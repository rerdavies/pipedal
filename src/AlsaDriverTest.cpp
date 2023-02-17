/*
 * MIT License
 * 
 * Copyright (c) 2022 Robin E. R. Davies
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "pch.h"
#include "catch.hpp"
#include <cmath>
#include <mutex>
#include <iostream>

#include "AlsaDriver.hpp"
#include "JackDriver.hpp"

using namespace pipedal;
using namespace std;


class AlsaTester: private AudioDriverHost {
public:
    enum class TestType { Oscillator, LatencyMonitor, NullTest};
private:
    AudioDriver *audioDriver = nullptr;
    TestType testType;

public:
    AlsaTester(TestType testType)
    :   testType(testType)
    {
        // audioDriver = CreateAlsaDriver(this);
        // audioDriver = CreateJackDriver(this);

    }
    ~AlsaTester()
    {
        delete audioDriver;
        delete[] inputBuffers;
        delete[] outputBuffers;
    }

    bool useJack = false;

    void Test()
    {

        AlsaFormatEncodeDecodeTest(this);

        JackServerSettings serverSettings("hw:M2",48000,32,3);

        JackConfiguration jackConfiguration;
        if (useJack)
        {
            jackConfiguration.JackInitialize();
        } else {
            jackConfiguration.AlsaInitialize(serverSettings);
        }

        JackChannelSelection channelSelection(
            jackConfiguration.GetInputAudioPorts(),
            jackConfiguration.GetOutputAudioPorts(),
            jackConfiguration.GetInputMidiDevices());

#if JACK_HOST
        if (useJack)
        {
            audioDriver = CreateJackDriver(this);

        } else {
            audioDriver = CreateAlsaDriver(this);
        }
#else
        audioDriver = CreateAlsaDriver(this);
#endif


        oscillator.Init(440,jackConfiguration.GetSampleRate());
        latencyMonitor.Init(jackConfiguration.GetSampleRate());
        audioDriver->Open(serverSettings,channelSelection);

        inputBuffers = new float*[channelSelection.GetInputAudioPorts().size()];
        outputBuffers = new float*[channelSelection.GetOutputAudioPorts().size()];

        audioDriver->Activate();

        for (int i = 0; i < 10; ++i)
        {
            sleep(1);
            if (testType == TestType::LatencyMonitor)
            {
                auto latency = this->latencyMonitor.GetLatency();
                double ms = 1000.0*latency/jackConfiguration.GetSampleRate();

                cout << "Latency: " << latency << " samples " << ms << "ms" << " xruns: " << GetXruns() << " Cpu: " << audioDriver->CpuUse() << "%" << endl;
            }
        }

        audioDriver->Deactivate();
        audioDriver->Close();

    }

    float**inputBuffers = nullptr;
    float**outputBuffers = nullptr;

    class Oscillator {
    private:
        double dx = 0;
        double x = 0;
        double dx2 = 0;
        double x2 = 0;
    public:

        void Init(float frequency, size_t sampleRate)
        {
            dx = frequency*3.141592736*2/sampleRate;
            dx2 = 0.5*3.141592736*2/sampleRate;
        }

        float Next() {
            float result = (float)std::cos(x);
            float env = (float)std::cos(x2);
            x += dx;
            x2 += dx2;
            return result*env;
        }
    };

    class LatencyMonitor {
        enum class State {
            Idle,
            Waiting,
        };
        State state = State::Idle;
        uint64_t t;
        uint64_t idle_samples;
        uint64_t waiting_samples;
        size_t current_latency = 0;
        size_t latency = 0;
        std::mutex sync;
    public:
        void Init(uint64_t sampleRate)
        {
            idle_samples = (uint64_t)sampleRate*2;
            waiting_samples = (uint64_t)sampleRate*2;

            state = State::Idle;
            t = idle_samples;
            latency = 0;
        }

        size_t GetLatency() {
            std::lock_guard lock { sync};
            return latency;
        }
        float Next(float input)
        {
            switch(state)
            {
                default:
                case State::Idle:
                {
                    if (t-- == 0) {
                        state = State::Waiting;
                        current_latency = 0;
                    }
                    return 0.01;
                }
                break;
                case State::Waiting:
                {
                    if (std::abs(input) > 0.1 || current_latency > 500) {
                        {
                            std::lock_guard lock { sync};
                            latency = current_latency;
                        }
                        state = State::Idle;
                        t = idle_samples;
                    } else {
                        ++current_latency;
                    }
                    return current_latency < 100 ? 0.25 : 0.0;
                }
                break;
            }
        }
    };


    Oscillator oscillator;
    LatencyMonitor latencyMonitor;


    virtual void OnAudioStopped() {
    }
    virtual void OnProcess(size_t nFrames) {
        if (testType == TestType::NullTest) return;


        size_t inputs = audioDriver->InputBufferCount();
        size_t outputs = audioDriver->OutputBufferCount();

        for (size_t i = 0; i < inputs; ++i)
        {
            inputBuffers[i] = audioDriver->GetInputBuffer(i,nFrames);
        }
        for (size_t i = 0; i < outputs; ++i)
        {
            outputBuffers[i] = audioDriver->GetOutputBuffer(i,nFrames);
        }
        if (this->testType == TestType::Oscillator)
        {
            for (size_t i = 0; i < nFrames; ++i)
            {
                float v = oscillator.Next()*0.25f;
                for (size_t c = 0; c < outputs; ++c)
                {
                    outputBuffers[c][i] = v;
                }
            }
        }
        else {
            for (size_t i = 0; i < nFrames; ++i)
            {
                float v = latencyMonitor.Next(inputBuffers[0][i]);
                for (size_t c = 0; c < outputs; ++c)
                {
                    outputBuffers[c][i] = v;
                }
            }

        }

    }
    std::mutex sync;
    uint64_t xruns;

    uint64_t GetXruns() {
        lock_guard lock { sync};
        return xruns;
    }
    virtual void OnUnderrun() {
        lock_guard lock { sync };
        ++xruns;

    }
};


TEST_CASE( "alsa_test", "[alsa_test]" ) {
    
    AlsaTester alsaDriver(AlsaTester::TestType::Oscillator);

    alsaDriver.Test();
}


TEST_CASE( "alsa_midi_test", "[alsa_midi_test]" ) {
    AlsaTester alsaDriver(AlsaTester::TestType::Oscillator);

    MidiDecoderTest();
}

