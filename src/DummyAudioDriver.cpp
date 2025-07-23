/*
 * MIT License
 *
 * Copyright (c) 2024 Robin E. R. Davies
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
#include "util.hpp"
#include <bit>
#include <memory>
#include "ss.hpp"
#include "DummyAudioDriver.hpp"
#include "JackServerSettings.hpp"
#include <thread>
#include "RtInversionGuard.hpp"
#include "PiPedalException.hpp"
#include <atomic>
#include <chrono>
#include <thread>
#include <stdexcept>
#include "ss.hpp"
#include "SchedulerPriority.hpp"
#include "CrashGuard.hpp"

#include "CpuUse.hpp"

#include <alsa/asoundlib.h>

#include "Lv2Log.hpp"
#include <limits>
#include "ss.hpp"

#undef ALSADRIVER_CONFIG_DBG

#ifdef ALSADRIVER_CONFIG_DBG
#include <stdio.h>
#endif

using namespace pipedal;

namespace pipedal
{

    [[noreturn]] static void DummyError(const std::string &message)
    {
        throw PiPedalStateException(message);
    }


    class DummyDriverImpl : public AudioDriver
    {
    private:
        pipedal::CpuUse cpuUse;

        uint32_t sampleRate = 0;

        uint32_t bufferSize;
        uint32_t numberOfBuffers;

        int playbackChannels = 2;
        int captureChannels = 2;


        uint32_t playbackSampleSize = 0;
        uint32_t captureSampleSize = 0;
        uint32_t playbackFrameSize = 0;
        uint32_t captureFrameSize = 0;


        std::vector<float *> activeCaptureBuffers;
        std::vector<float *> activePlaybackBuffers;

        std::vector<float *> captureBuffers;
        std::vector<float *> playbackBuffers;

        uint8_t *rawCaptureBuffer = nullptr;
        uint8_t *rawPlaybackBuffer = nullptr;

        AudioDriverHost *driverHost = nullptr;
        uint32_t channels = 2;

    public:
        DummyDriverImpl(AudioDriverHost *driverHost,const std::string&deviceName)
            : driverHost(driverHost)
            , channels(GetDummyAudioChannels(deviceName))
        {
            captureChannels = channels;
            playbackChannels = channels;

            midiEventMemoryIndex = 0;
            midiEventMemory.resize(MIDI_MEMORY_BUFFER_SIZE);
            midiEvents.resize(MAX_MIDI_EVENT);


        }
        virtual ~DummyDriverImpl()
        {
            Close();
        }

    private:
        void OnShutdown()
        {
            Lv2Log::info("Dummy Audio Server has shut down.");
        }

        virtual uint32_t GetSampleRate()
        {
            return this->sampleRate;
        }

        JackServerSettings jackServerSettings;
        AlsaSequencer::ptr alsaSequencer;

        static constexpr size_t MIDI_MEMORY_BUFFER_SIZE = 32 * 1024;
        static constexpr size_t MAX_MIDI_EVENT = 4 * 1024;

        size_t midiEventCount = 0;
        std::vector<MidiEvent> midiEvents;
        size_t midiEventMemoryIndex = 0;
        std::vector<uint8_t> midiEventMemory;


        unsigned int periods = 0;



        std::atomic<bool> terminateAudio_ = false;

        void terminateAudio(bool terminate)
        {
            this->terminateAudio_ = terminate;
        }

        bool terminateAudio()
        {
           return this->terminateAudio_;
        }

    private:
        void DummyCleanup()
        {
        }

    private:
        void AllocateBuffers(std::vector<float *> &buffers, size_t n)
        {
            buffers.resize(n);
            for (size_t i = 0; i < n; ++i)
            {
                buffers[i] = new float[this->bufferSize];
                for (size_t j = 0; j < this->bufferSize; ++j)
                {
                    buffers[i][j] = 0;
                }
            }
        }

        virtual size_t GetMidiInputEventCount() override
        {
            return midiEventCount;
        }
        virtual MidiEvent *GetMidiEvents() override
        {
            return this->midiEvents.data();
        }


        JackChannelSelection channelSelection;
        bool open = false;
        virtual void Open(const JackServerSettings &jackServerSettings, const JackChannelSelection &channelSelection)
        {
            terminateAudio_ = false;
            if (open)
            {
                throw PiPedalStateException("Already open.");
            }
            this->jackServerSettings = jackServerSettings;
            this->channelSelection = channelSelection;
            this->sampleRate = jackServerSettings.GetSampleRate();

            open = true;
            try
            {
                OpenAudio(jackServerSettings, channelSelection);
            }
            catch (const std::exception &e)
            {
                Close();
                throw;
            }
        }
        virtual void SetAlsaSequencer(AlsaSequencer::ptr alsaSequencer) override
        {
            this->alsaSequencer = alsaSequencer;
        }
        virtual std::string GetConfigurationDescription()
        {
            std::string result = SS(
                "DUMMY, " 
                << "n/a"
                << ", " << "Native float"
                << ", " << this->sampleRate
                << ", " << this->bufferSize << "x" << this->numberOfBuffers
                << ", in: " << this->InputBufferCount() << "/" << this->captureChannels
                << ", out: " << this->OutputBufferCount() << "/" << this->playbackChannels);
            return result;
        }

        void OpenAudio(const JackServerSettings &jackServerSettings, const JackChannelSelection &channelSelection)
        {
            int err;

            this->numberOfBuffers = jackServerSettings.GetNumberOfBuffers();
            this->bufferSize = jackServerSettings.GetBufferSize();
            AllocateBuffers(captureBuffers, channels);
            AllocateBuffers(playbackBuffers, channels);
     
        }

        std::jthread *audioThread;
        bool audioRunning;

        bool block = false;

        void ReadMidiData(uint32_t audioFrame)
        {
            AlsaMidiMessage message;

            midiEventCount = 0;
            while(alsaSequencer->ReadMessage(message,0))
            {
                size_t messageSize = message.size;
                if (messageSize == 0) 
                {
                    continue;
                }
                if (midiEventMemoryIndex + messageSize  >= this->midiEventMemory.size()) {
                    continue;
                }
                if (midiEventCount >= this->midiEvents.size()) {
                    midiEvents.resize(midiEventCount*2);
                }
                // for now, prevent META event messages from propagating. 
                if (message.data[0] == 0xFF && message.size > 1) {
                    continue;
                }
                MidiEvent *pEvent = midiEvents.data() + midiEventCount++;
                pEvent->time = audioFrame;
                pEvent->size = messageSize; 
                pEvent->buffer = midiEventMemory.data() + midiEventMemoryIndex;

                memcpy(
                    midiEventMemory.data() + midiEventMemoryIndex,
                    message.data,
                    message.size);
                midiEventMemoryIndex += messageSize;
                
            }
        }


        void AudioThread()
        {
            SetThreadName("dummyAudioDriver");
            try
            {

                SetThreadPriority(SchedulerPriority::RealtimeAudio);

                bool ok = true;

                CrashGuardLock crashGuardLock;
                while (true)
                {

                    if (terminateAudio())
                    {
                        break;
                    }

                    ReadMidiData((uint32_t)0);

                    ssize_t framesRead = this->bufferSize;
                    this->driverHost->OnProcess(framesRead);

                    /// no attempt at realtime. Just as long as we run occasionally.
                    std::this_thread::sleep_for(std::chrono::milliseconds(20));


                }
            }
            catch (const std::exception &e)
            {
                Lv2Log::error(e.what());
                Lv2Log::error("Dummy audio thread terminated abnormally.");
            }
            this->driverHost->OnAudioTerminated();
        }

        bool alsaActive = false;

        static int IndexFromPortName(const std::string &s)
        {
            auto pos = s.find_last_of('_');
            if (pos == std::string::npos)
            {
                throw std::invalid_argument("Bad port name.");
            }
            const char *p = s.c_str() + (pos + 1);

            int v = atoi(p);
            if (v < 0)
            {
                throw std::invalid_argument("Bad port name.");
            }
            return v;
        }

        bool activated = false;
        virtual void Activate()
        {
            if (activated)
            {
                throw PiPedalStateException("Already activated.");
            }
            activated = true;

            this->activeCaptureBuffers.resize(channelSelection.GetInputAudioPorts().size());

            playbackBuffers.resize(channels);

            int ix = 0;
            for (auto &x : channelSelection.GetInputAudioPorts())
            {
                int sourceIndex = IndexFromPortName(x);
                if (sourceIndex >= captureBuffers.size())
                {
                    Lv2Log::error(SS("Invalid audio input port: " << x));
                }
                else
                {
                    this->activeCaptureBuffers[ix++] = this->captureBuffers[sourceIndex];
                }
            }

            this->activePlaybackBuffers.resize(channelSelection.GetOutputAudioPorts().size());

            ix = 0;
            for (auto &x : channelSelection.GetOutputAudioPorts())
            {
                int sourceIndex = IndexFromPortName(x);
                if (sourceIndex >= playbackBuffers.size())
                {
                    Lv2Log::error(SS("Invalid audio output port: " << x));
                }
                else
                {
                    this->activePlaybackBuffers[ix++] = this->playbackBuffers[sourceIndex];
                }
            }

            audioThread = new std::jthread([this]()
                                           { AudioThread(); });
        }

        virtual void Deactivate()
        {
            if (!activated)
            {
                return;
            }
            activated = false;
            terminateAudio(true);
            if (audioThread)
            {
                this->audioThread->join();
                this->audioThread = 0;
            }
            Lv2Log::debug("Audio thread joined.");
        }


    public:


   
        virtual size_t InputBufferCount() const { return activeCaptureBuffers.size(); }
        virtual float *GetInputBuffer(size_t channel)
        {
            return activeCaptureBuffers[channel];
        }

        virtual size_t OutputBufferCount() const { return activePlaybackBuffers.size(); }
        virtual float *GetOutputBuffer(size_t channel)
        {
            return activePlaybackBuffers[channel];
        }

        void FreeBuffers(std::vector<float *> &buffer)
        {
            for (size_t i = 0; i < buffer.size(); ++i)
            {
                // delete[] buffer[i];
                buffer[i] = 0;
            }
            buffer.clear();
        }
        void DeleteBuffers()
        {
            activeCaptureBuffers.clear();
            activePlaybackBuffers.clear();
            FreeBuffers(this->playbackBuffers);
            FreeBuffers(this->captureBuffers);
            if (rawCaptureBuffer)
            {
                delete[] rawCaptureBuffer;
                rawCaptureBuffer = nullptr;
            }
            if (rawPlaybackBuffer)
            {
                delete[] rawPlaybackBuffer;
                rawPlaybackBuffer = nullptr;
            }
        }
        virtual void Close()
        {
            if (!open)
            {
                return;
            }
            open = false;
            Deactivate();
            DummyCleanup();
            DeleteBuffers();
        }

        virtual float CpuUse()
        {
            return 0;
        }

        virtual float CpuOverhead()
        {
            return 0.1;
        }
    };

    AudioDriver *CreateDummyAudioDriver(AudioDriverHost *driverHost,const std::string&deviceName)
    {
        return new DummyDriverImpl(driverHost,deviceName);
    }

    bool GetDummyChannels(const JackServerSettings &jackServerSettings,
                         std::vector<std::string> &inputAudioPorts,
                         std::vector<std::string> &outputAudioPorts,
                         uint32_t channels)
    {

        bool result = false;

        try
        {

            uint32_t playbackChannels = channels, captureChannels = channels;

            inputAudioPorts.clear();
            for (unsigned int i = 0; i < captureChannels; ++i)
            {
                inputAudioPorts.push_back(SS("system::capture_" << i));
            }

            outputAudioPorts.clear();
            for (unsigned int i = 0; i < playbackChannels; ++i)
            {
                outputAudioPorts.push_back(SS("system::playback_" << i));
            }

            result = true;
        }
        catch (const std::exception &e)
        {
            throw;
        }
        return result;
    }



    AlsaDeviceInfo MakeDummyDeviceInfo(uint32_t channels)
    {
        AlsaDeviceInfo result;
        constexpr int DUMMY_DEVICE_ID_OFFSET = 100974;
        result.cardId_ = DUMMY_DEVICE_ID_OFFSET+channels;
        result.id_ = SS("dummy:channels_" << channels);
        result.name_ = SS("Dummy Device (" << channels << " channels)");
        result.longName_ = result.name_;
        result.sampleRates_.push_back(44100);
        result.sampleRates_.push_back(48000);
        result.minBufferSize_ = 16;
        result.maxBufferSize_ = 1024;
		result.supportsCapture_ = true;
        result.supportsPlayback_ = true;
        return result;
    }
    
} // namespace

uint32_t pipedal::GetDummyAudioChannels(const std::string &deviceName)
{
    uint32_t channels;
    int pos = deviceName.find_last_of('_');
    if (pos == std::string::npos)
    {
        throw std::runtime_error("Invalid dummy device name");
    }
    std::istringstream ss(deviceName.substr(pos+1));
    ss >> channels;
    return channels;

}

