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

    public:
        DummyDriverImpl(AudioDriverHost *driverHost)
            : driverHost(driverHost)
        {
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
                OpenMidi(jackServerSettings, channelSelection);
                OpenAudio(jackServerSettings, channelSelection);
            }
            catch (const std::exception &e)
            {
                Close();
                throw;
            }
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
            AllocateBuffers(captureBuffers, 2);
            AllocateBuffers(playbackBuffers, 2);
     
        }

        std::jthread *audioThread;
        bool audioRunning;

        bool block = false;

        void AudioThread()
        {
            SetThreadName("dummyAudioDriver");
            try
            {
#if defined(__WIN32)
                // bump thread prioriy two levels to
                // ensure that the service thread doesn't
                // get bogged down by UIwork. Doesn't have to be realtime, but it
                // MUST run at higher priority than UI threads.
                xxx; // TO DO.
#elif defined(__linux__)
                int min = sched_get_priority_min(SCHED_RR);
                int max = sched_get_priority_max(SCHED_RR);

                struct sched_param param;
                memset(&param, 0, sizeof(param));
                param.sched_priority = RT_THREAD_PRIORITY;

                int result = sched_setscheduler(0, SCHED_RR, &param);
                if (result == 0)
                {
                    Lv2Log::debug("Service thread priority successfully boosted.");
                }
                else
                {
                    Lv2Log::error(SS("Failed to set ALSA AudioThread priority. (" << strerror(result) << ")"));
                }
#else
                xxx; // TODO for your platform.
#endif

                bool ok = true;

                while (true)
                {
                    if (terminateAudio())
                    {
                        break;
                    }

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

            playbackBuffers.resize(2);

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

        static constexpr size_t MIDI_BUFFER_SIZE = 16 * 1024;
        static constexpr size_t MAX_MIDI_EVENT = 4 * 1024;

    public:
        class MidiState
        {
        private:
            snd_rawmidi_t *hIn = nullptr;
            snd_rawmidi_params_t *hInParams = nullptr;
            std::string deviceName;

            // running status state.
            uint8_t runningStatus = 0;
            int dataLength = 0;
            int dataIndex = 0;
            size_t statusBytesRemaining = 0;
            size_t data0;
            size_t data1;

            size_t eventCount = 0;
            MidiEvent events[MAX_MIDI_EVENT];
            size_t bufferCount = 0;
            uint8_t buffer[MIDI_BUFFER_SIZE];

            uint8_t readBuffer[1024];

            ssize_t sysexStartIndex = -1;

            void checkError(int result, const char *message)
            {
                if (result < 0)
                {
                    throw PiPedalStateException(SS("Unexpected error: " << message << " (" << this->deviceName));
                }
            }

        public:
            void Open(const AlsaMidiDeviceInfo &device)
            {
                bufferCount = 0;
                eventCount = 0;
                sysexStartIndex = -1;
                runningStatus = 0;
                dataIndex = 0;
                dataLength = 0;

                this->deviceName = device.description_;

            }
            void Close()
            {
            }


            size_t GetMidiInputEventCount()
            {
                return 0;
            }

            bool GetMidiInputEvent(MidiEvent *event, size_t nFrame)
            {
                return false;
            }

            void MidiPut(uint8_t cc, uint8_t d0, uint8_t d1)
            {
            }

            void FillBuffer()
            {
            }

            void WriteBuffer(uint8_t *readBuffer, size_t nRead)
            {
            }
        };

        std::vector<MidiState *> midiStates;

        void OpenMidi(const JackServerSettings &jackServerSettings, const JackChannelSelection &channelSelection)
        {
            const auto &devices = channelSelection.GetInputMidiDevices();

            midiStates.resize(devices.size());

            for (size_t i = 0; i < devices.size(); ++i)
            {
                const auto &device = devices[i];
                MidiState *state = new MidiState();
                midiStates[i] = state;
                state->Open(device);
            }
        }

        virtual size_t MidiInputBufferCount() const
        {
            return this->midiStates.size();
        }
        virtual void *GetMidiInputBuffer(size_t channel, size_t nFrames)
        {
            return (void *)midiStates[channel];
        }

        virtual size_t GetMidiInputEventCount(void *portBuffer)
        {
            MidiState *state = (MidiState *)portBuffer;
            return state->GetMidiInputEventCount();
        }
        virtual bool GetMidiInputEvent(MidiEvent *event, void *portBuf, size_t nFrame)
        {
            MidiState *state = (MidiState *)portBuf;
            return state->GetMidiInputEvent(event, nFrame);
        }

        virtual void FillMidiBuffers()
        {
        }

        virtual size_t InputBufferCount() const { return activeCaptureBuffers.size(); }
        virtual float *GetInputBuffer(size_t channel, size_t nFrames)
        {
            return activeCaptureBuffers[channel];
        }

        virtual size_t OutputBufferCount() const { return activePlaybackBuffers.size(); }
        virtual float *GetOutputBuffer(size_t channel, size_t nFrames)
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

    AudioDriver *CreateDummyAudioDriver(AudioDriverHost *driverHost)
    {
        return new DummyDriverImpl(driverHost);
    }

    bool GetDummyChannels(const JackServerSettings &jackServerSettings,
                         std::vector<std::string> &inputAudioPorts,
                         std::vector<std::string> &outputAudioPorts)
    {

        bool result = false;

        try
        {

            unsigned int playbackChannels = 2, captureChannels = 2;

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

} // namespace
