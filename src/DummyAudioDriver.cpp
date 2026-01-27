/*
 * MIT License
 *
 * Copyright (c) 2026 Robin E. R. Davies
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
#include "PiPedalCommon.hpp"
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
#include "ChannelRouterSettings.hpp"

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


        std::vector<std::vector<float>> allocatedBuffers;
        std::vector<float *> mainCaptureBuffers;
        std::vector<float *> mainPlaybackBuffers;

        std::vector<float *> auxCaptureBuffers;
        std::vector<float *> auxPlaybackBuffers;

        std::vector<float*> sendCaptureBuffers;
        std::vector<float*> sendPlaybackBuffers;

        std::vector<float *> deviceCaptureBuffers;
        std::vector<float *> devicePlaybackBuffers;


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

        PIPEDAL_NON_INLINE void terminateAudio(bool terminate)
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
        PIPEDAL_NON_INLINE void AllocateBuffers(std::vector<float *> &buffers, size_t n)
        {
            buffers.resize(n);
            for (size_t i = 0; i < n; ++i)
            {
                buffers[i] = AllocateAudioBuffer();
            }
        }

        PIPEDAL_NON_INLINE virtual size_t GetMidiInputEventCount() override
        {
            return midiEventCount;
        }
        PIPEDAL_NON_INLINE virtual MidiEvent *GetMidiEvents() override
        {
            return this->midiEvents.data();
        }


        ChannelSelection channelSelection;
        bool open = false;
        PIPEDAL_NON_INLINE virtual void Open(const JackServerSettings &jackServerSettings, const ChannelSelection &channelSelection)
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
        PIPEDAL_NON_INLINE virtual void SetAlsaSequencer(AlsaSequencer::ptr alsaSequencer) override
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
                << ", " << "device in: " << this->DeviceInputBufferCount() 
                << ", " << "device out: " << this->DeviceOutputBufferCount()
                << ", main in: " << this->MainInputBufferCount() 
                << ", main out: " << this->MainOutputBufferCount() 
                << ", aux in: " << this->AuxInputBufferCount() 
                << ", aux out: " << this->AuxOutputBufferCount()
                << ", send in: " << this->SendInputBufferCount() 
                << ", send out: " << this->SendOutputBufferCount()
            );
            return result;
        }

        void OpenAudio(const JackServerSettings &jackServerSettings, const ChannelSelection &channelSelection)
        {
            int err;

            this->numberOfBuffers = jackServerSettings.GetNumberOfBuffers();
            this->bufferSize = jackServerSettings.GetBufferSize();
     
        }

        std::unique_ptr<std::jthread> audioThread;
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
                pEvent->frame = audioFrame;
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
                    std::this_thread::sleep_for(std::chrono::milliseconds(5));


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


        bool activated = false;
        virtual void Activate()
        {
            if (activated)
            {
                throw PiPedalStateException("Already activated.");
            }
            activated = true;

            AllocateBuffers(deviceCaptureBuffers, channels);
            AllocateBuffers(devicePlaybackBuffers, channels);

            AllocateBuffers(mainCaptureBuffers, channelSelection.mainInputChannels().size());
            AllocateBuffers(mainPlaybackBuffers, channelSelection.mainOutputChannels().size());
            AllocateBuffers(auxCaptureBuffers, channelSelection.auxInputChannels().size());
            AllocateBuffers(auxPlaybackBuffers, channelSelection.auxOutputChannels().size());
            AllocateBuffers(sendCaptureBuffers, channelSelection.sendInputChannels().size());
            AllocateBuffers(sendPlaybackBuffers, channelSelection.sendOutputChannels().size());

            audioThread = std::make_unique<std::jthread>([this]()
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
                this->audioThread = nullptr;
            }
            Lv2Log::debug("Audio thread joined.");
        }


    public:


        virtual size_t DeviceInputBufferCount() const override { return deviceCaptureBuffers.size(); }
        virtual size_t DeviceOutputBufferCount() const override { return devicePlaybackBuffers.size(); }
   
        virtual size_t MainInputBufferCount() const override { return mainCaptureBuffers.size(); }
        virtual float *GetMainInputBuffer(size_t channel) override
        {
            return mainCaptureBuffers[channel];
        }

        virtual size_t MainOutputBufferCount() const { return mainPlaybackBuffers.size(); }
        virtual float *GetMainOutputBuffer(size_t channel) override
        {
            return mainPlaybackBuffers[channel];
        }

        virtual size_t AuxInputBufferCount() const override { return auxCaptureBuffers.size(); }
        virtual float *GetAuxInputBuffer(size_t channel) override
        {
            return auxCaptureBuffers[channel];
        }

        virtual size_t AuxOutputBufferCount() const override { return auxPlaybackBuffers.size(); }
        virtual float *GetAuxOutputBuffer(size_t channel) override
        {
            return auxPlaybackBuffers[channel];
        }

        virtual size_t SendInputBufferCount() const override { return sendCaptureBuffers.size(); }
        virtual float *GetSendInputBuffer(size_t channel) override
        {
            return sendCaptureBuffers[channel];
        }

        virtual size_t SendOutputBufferCount() const override { return sendPlaybackBuffers.size(); }
        virtual float *GetSendOutputBuffer(size_t channel) override
        {
            return sendPlaybackBuffers[channel];
        }



        

        PIPEDAL_NON_INLINE float*AllocateAudioBuffer() {
            allocatedBuffers.push_back(std::vector<float>(bufferSize));
            return allocatedBuffers.back().data();
        }
        PIPEDAL_NON_INLINE void DeleteBuffers()
        {
            mainCaptureBuffers.clear();
            mainPlaybackBuffers.clear();
            auxCaptureBuffers.clear();
            auxPlaybackBuffers.clear();
            sendCaptureBuffers.clear();
            sendPlaybackBuffers.clear();
            allocatedBuffers.clear();
        }
        PIPEDAL_NON_INLINE virtual void Close()
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

        PIPEDAL_NON_INLINE virtual float CpuUse()
        {
            return 0;
        }

        PIPEDAL_NON_INLINE virtual float CpuOverhead()
        {
            return 0.1;
        }
    };

    PIPEDAL_NON_INLINE AudioDriver *CreateDummyAudioDriver(AudioDriverHost *driverHost,const std::string&deviceName)
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

