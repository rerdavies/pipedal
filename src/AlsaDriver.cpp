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
#include <cmath>
#include "Finally.hpp"
#include <bit>
#include <memory>
#include "ss.hpp"
#include "AlsaDriver.hpp"
#include "JackServerSettings.hpp"
#include <thread>
#include "RtInversionGuard.hpp"
#include "PiPedalException.hpp"
#include "DummyAudioDriver.hpp"
#include "SchedulerPriority.hpp"
#include "CrashGuard.hpp"
#include <iostream>
#include <iomanip>

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

#define TRACE_BUFFER_POSITIONS 1
    static bool ShouldForceStereoChannels(snd_pcm_t *pcmHandle, snd_pcm_hw_params_t *hwParams, unsigned int channelsMin, unsigned int channelsMax)
    {
        // The problem: old IC2 drivers seem to return 1-8 channels, but 8 channels is non-functinal. The assumption is that legacy drivers
        // (I2C drivers, particularl, but also the Rpi headphones device, as an interesting example) that don't support channel maps do this.
        // Hypothetically, devices could allow slection of hardware-downmixed surround channels. So deal with this case defensively.
        // The approach: check the channel map and do our best to interpret what we find.
        // No channel map, or any part of the channel map is unknown? Probably the legalcy case we're interested in. Return TRUE
        // If the channel map is a surround format, return true in that case as well.
        // If the channel map is pairwise, return false! (legitimately multi-channel devices should not be forced to stereo).
        // If the channel map is all FL/FR/MONO return false (a hypothetical configuration for a multi-channel device)
        // If the channel map is not all FL/FR/MONO, assume that it's an upmixed/downmixed surround format, and return TRUE.
        // This is high-risk code, because it attempts to anticipate hypothetical device configurations with no actual testing.

        if (channelsMax <= 2)
            return false;
        if (channelsMin == channelsMax)
            return false;
        if (channelsMin > 2)
            return false; // can't imagine what sort of device this is.

        snd_pcm_hw_params_t *test_params;

        snd_pcm_hw_params_alloca(&test_params);
        snd_pcm_hw_params_copy(test_params, hwParams);

        // can we select 2 channels?
        if (snd_pcm_hw_params_set_channels(pcmHandle, test_params, (unsigned int)2) >= 0)
        {
            snd_pcm_chmap_query_t **chmaps = snd_pcm_query_chmaps(pcmHandle);

            if (chmaps == nullptr)
            {
                return true; // probably an old driver. Do it.
            }
            Finally ff([chmaps]()
                       { snd_pcm_free_chmaps(chmaps); });
            for (size_t i = 0; chmaps[i] != nullptr; ++i)
            {
                snd_pcm_chmap_query_t *chmap = chmaps[i];
                if (chmap->map.channels == channelsMax)
                {
                    switch (chmap->type)
                    {
                    case SND_CHMAP_TYPE_NONE:
                    default:
                        return true; // weird legacy case?  Do it.
                    case SND_CHMAP_TYPE_PAIRED:
                        return false; // A legitimate multi-channel device. definitely don't do it.

                    case SND_CHMAP_TYPE_VAR:
                    case SND_CHMAP_TYPE_FIXED:
                    {
                        // we should do it for surround formats. guard against other hypothetical mappings for legitimately multi-channel devices.

                        snd_pcm_chmap_position pos0 = (snd_pcm_chmap_position)(chmap->map.pos[0]);
                        if (pos0 == snd_pcm_chmap_position::SND_CHMAP_MONO) // hypothetical channel map of all mono channesl.
                        {
                            return false; // don't do it.
                        }
                        if (pos0 != snd_pcm_chmap_position::SND_CHMAP_FL && pos0 != snd_pcm_chmap_position::SND_CHMAP_FL) // surround formats always start with FL. Hypothetical quad formats could start with FC.
                        {
                            return false; // don't do it.
                        }
                        // accept a hypothetical channel map of mixed FL's and FR's, FC's and MONOs. (Multi-channel with mixed mono and stereo pairs).
                        // But otherwise assume it's a surround map, and use a stereo channel configuration instead.
                        for (size_t i = 0; i < chmap->map.channels; ++i)
                        {
                            snd_pcm_chmap_position pos = (snd_pcm_chmap_position)(chmap->map.pos[i]);
                            switch (pos)
                            {
                            case snd_pcm_chmap_position::SND_CHMAP_MONO:
                            case snd_pcm_chmap_position::SND_CHMAP_FL:
                            case snd_pcm_chmap_position::SND_CHMAP_FR:
                            case snd_pcm_chmap_position::SND_CHMAP_FC:
                                break; // keep going.
                            default:
                                return true; // probably a surround sound map.
                            }
                        }
                        return false;
                    };
                    }
                }
            }
            return true; // no matching channel map(!??). nonsensical case. may as well use the stereo config, which might be more sensible.
        }
        return false;
    }

    struct AudioFormat
    {
        char name[40];
        snd_pcm_format_t pcm_format;
    };

    bool SetPreferredAlsaFormat(
        const char *streamType,
        snd_pcm_t *handle,
        snd_pcm_hw_params_t *hwParams,
        AudioFormat *formats,
        size_t nItems)
    {
        snd_pcm_hw_params_t *test_params;
        snd_pcm_hw_params_alloca(&test_params);

        for (size_t i = 0; i < nItems; ++i)
        {
            snd_pcm_hw_params_copy(test_params, hwParams);

            int err = snd_pcm_hw_params_set_format(handle, test_params, formats[i].pcm_format);
            if (err == 0)
            {
                int err = snd_pcm_hw_params_set_format(handle, hwParams, formats[i].pcm_format);
                if (err == 0)
                {
                    return true;
                }
            }
        }
        return false;
    }

    static AudioFormat leFormats[]{
        {"32-bit float little-endian", SND_PCM_FORMAT_FLOAT_LE},
        {"32-bit integer little-endian", SND_PCM_FORMAT_S32_LE},
        {"24-bit little-endian", SND_PCM_FORMAT_S24_LE},
        {"24-bit little-endian in 3bytes format", SND_PCM_FORMAT_S24_3LE},
        {"16-bit little-endian", SND_PCM_FORMAT_S16_LE},

    };
    static AudioFormat beFormats[]{
        {"32-bit float big-endian", SND_PCM_FORMAT_FLOAT_BE},
        {"32-bit integer big-endian", SND_PCM_FORMAT_S32_BE},
        {"24-bit big-endian", SND_PCM_FORMAT_S24_BE},
        {"24-bit big-endian in 3bytes format", SND_PCM_FORMAT_S24_3BE},
        {"16-bit big-endian", SND_PCM_FORMAT_S16_BE},
    };
    [[noreturn]] static void AlsaError(const std::string &message)
    {
        throw PiPedalStateException(message);
    }

    std::string GetAlsaFormatDescription(snd_pcm_format_t format)
    {
        for (size_t i = 0; i < sizeof(beFormats) / sizeof(beFormats[0]); ++i)
        {
            if (beFormats[i].pcm_format == format)
            {
                return beFormats[i].name;
            }
        }
        for (size_t i = 0; i < sizeof(leFormats) / sizeof(leFormats[0]); ++i)
        {
            if (leFormats[i].pcm_format == format)
            {
                return leFormats[i].name;
            }
        }
        return "Unknown format.";
    }

    void SetPreferredAlsaFormat(
        const std::string &alsa_device_name,
        const char *streamType,
        snd_pcm_t *handle,
        snd_pcm_hw_params_t *hwParams)
    {
        int err;

        if (std::endian::native == std::endian::big)
        {
            if (SetPreferredAlsaFormat(streamType, handle, hwParams, beFormats, sizeof(beFormats) / sizeof(beFormats[0])))
                return;
            if (SetPreferredAlsaFormat(streamType, handle, hwParams, leFormats, sizeof(leFormats) / sizeof(leFormats[0])))
                return;
        }
        else
        {
            if (SetPreferredAlsaFormat(streamType, handle, hwParams, leFormats, sizeof(leFormats) / sizeof(leFormats[0])))
                return;
            if (SetPreferredAlsaFormat(streamType, handle, hwParams, beFormats, sizeof(beFormats) / sizeof(beFormats[0])))
                return;
        }
        AlsaError(SS("No supported audio formats (" << alsa_device_name << "/" << streamType << ")"));
    }

    class AlsaDriverImpl : public AudioDriver
    {
    private:
        struct BufferTrace
        {
            uint64_t time;
            snd_pcm_sframes_t inAvail;
            snd_pcm_sframes_t outAvail;
            snd_pcm_sframes_t buffered;
            snd_pcm_sframes_t total;
            char code;
        };

        std::vector<BufferTrace> bufferTraces{1000};
        size_t bufferTraceIndex = 0;

        virtual void DumpBufferTrace(size_t nEntries) override;

        inline void TraceBufferPositions(size_t framesInBuffer, char code = ' ')
        {
#if TRACE_BUFFER_POSITIONS
            uint64_t time = std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now().time_since_epoch()).count();
            auto inAvail = snd_pcm_avail_update(this->captureHandle);
            auto outAvail = snd_pcm_avail_update(this->playbackHandle);

            auto total = (inAvail >= 0? inAvail: 0) + (outAvail >= 0 ? outAvail: 0) + framesInBuffer;
            bufferTraces[bufferTraceIndex++] = {
                time,
                inAvail,
                outAvail,
                (snd_pcm_sframes_t)framesInBuffer,
                (snd_pcm_sframes_t)total,
                code};
            if (bufferTraceIndex == bufferTraces.size())
            {
                bufferTraceIndex = 0;
            }
#endif
        }

        std::recursive_mutex restartMutex;

        pipedal::CpuUse cpuUse;

#ifdef ALSADRIVER_CONFIG_DBG
        snd_output_t *snd_output = nullptr;
        snd_pcm_status_t *snd_status = nullptr;

#endif
        uint32_t sampleRate = 0;

        uint32_t bufferSize = 0;
        uint32_t numberOfBuffers = 0;

        unsigned int capturePeriods = 0;
        unsigned int playbackPeriods = 0;

        uint32_t captureHardwarePeriodSize = 0;
        uint32_t playbackHardwarePeriodSize = 0;
        ;

        int playbackChannels = 0;
        int captureChannels = 0;

        uint32_t user_threshold = 0;
        bool soft_mode = false;

        snd_pcm_format_t captureFormat = snd_pcm_format_t::SND_PCM_FORMAT_UNKNOWN;

        uint32_t playbackSampleSize = 0;
        uint32_t captureSampleSize = 0;
        uint32_t playbackFrameSize = 0;
        uint32_t captureFrameSize = 0;

        using CopyFunction = void (AlsaDriverImpl::*)(size_t frames);

        CopyFunction copyInputFn;
        CopyFunction copyOutputFn;

        bool inputSwapped = false;
        bool outputSwapped = false;

        std::vector<float *> activeCaptureBuffers;
        std::vector<float *> activePlaybackBuffers;

        std::vector<float *> captureBuffers;
        std::vector<float *> playbackBuffers;

        std::vector<uint8_t> rawCaptureBuffer;
        std::vector<uint8_t> rawPlaybackBuffer;

        AudioDriverHost *driverHost = nullptr;

        void validate_capture_handle()
        { // leftover debugging for a buffer overrun :-/
#ifdef DEBUG
            if (snd_pcm_type(captureHandle) != SND_PCM_TYPE_HW)
            {
                throw std::runtime_error("Capture handle has been overwritten");
            }
#endif
        }

    public:
        AlsaDriverImpl(AudioDriverHost *driverHost)
            : driverHost(driverHost)
        {
            midiEventMemoryIndex = 0;
            midiEventMemory.resize(MIDI_MEMORY_BUFFER_SIZE);
            midiEvents.resize(MAX_MIDI_EVENT);
        }
        virtual ~AlsaDriverImpl()
        {
            Close();
#ifdef ALSADRIVER_CONFIG_DBG
            if (snd_output)
            {
                snd_output_close(snd_output);
                snd_output = nullptr;
            }
            if (snd_status)
            {
                snd_pcm_status_free(snd_status);
                snd_status = nullptr;
            }
#endif
        }

    private:
        void OnShutdown()
        {
            Lv2Log::info("ALSA Audio Server has shut down.");
        }

        static void
        jack_shutdown_fn(void *arg)
        {
            ((AlsaDriverImpl *)arg)->OnShutdown();
        }

        static int xrun_callback_fn(void *arg)
        {
            ((AudioDriverHost *)arg)->OnUnderrun();
            return 0;
        }

        virtual uint32_t GetSampleRate()
        {
            return this->sampleRate;
        }

        JackServerSettings jackServerSettings;

        std::string alsa_device_name;

        snd_pcm_t *playbackHandle = nullptr;
        snd_pcm_t *captureHandle = nullptr;

        snd_pcm_hw_params_t *captureHwParams = nullptr;
        snd_pcm_sw_params_t *captureSwParams = nullptr;
        snd_pcm_hw_params_t *playbackHwParams = nullptr;
        snd_pcm_sw_params_t *playbackSwParams = nullptr;

        bool capture_and_playback_not_synced = false;

        std::mutex terminateSync;

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
        void AlsaCloseAudio()
        {
            std::lock_guard lock{restartMutex};

            if (captureHandle)
            {
                Lv2Log::debug("ALSA capture handle closed.");
                snd_pcm_drain(captureHandle);
                snd_pcm_close(captureHandle);
                captureHandle = nullptr;
            }
            if (playbackHandle)
            {
                Lv2Log::debug("ALSA playback handle closed.");
                snd_pcm_drain(playbackHandle);
                snd_pcm_close(playbackHandle);
                playbackHandle = nullptr;
            }
            if (captureHwParams)
            {
                snd_pcm_hw_params_free(captureHwParams);
                captureHwParams = nullptr;
            }
            if (captureSwParams)
            {
                snd_pcm_sw_params_free(captureSwParams);
                captureSwParams = nullptr;
            }
            if (playbackHwParams)
            {
                snd_pcm_hw_params_free(playbackHwParams);
                playbackHwParams = nullptr;
            }
            if (playbackSwParams)
            {
                snd_pcm_sw_params_free(playbackSwParams);
                playbackSwParams = nullptr;
            }
        }
        void AlsaCleanup()
        {
            AlsaCloseAudio();
        }

        std::string discover_alsa_using_apps()
        {
            return ""; // xxx fix me.
        }

        void AlsaConfigureStream(
            const std::string &alsa_device_name,
            const char *streamType,
            snd_pcm_t *handle,
            snd_pcm_hw_params_t *hwParams,
            snd_pcm_sw_params_t *swParams,
            int *channels,
            unsigned int *periods,
            unsigned int *hwPeriodSize)
        {
            int err;
            snd_pcm_uframes_t stop_th;

            bool isCaptureStream = strcmp(streamType, "capture") == 0;

            if ((err = snd_pcm_hw_params_any(handle, hwParams)) < 0)
            {
                AlsaError(SS("No playback configurations available (" << snd_strerror(err) << ")"));
            }

            err = snd_pcm_hw_params_set_access(handle, hwParams, SND_PCM_ACCESS_RW_INTERLEAVED);
            if (err < 0)
            {
                AlsaError("snd_pcm_hw_params_set_access failed.");
            }

            SetPreferredAlsaFormat(alsa_device_name, streamType, handle, hwParams);

            unsigned int sampleRate = (unsigned int)this->sampleRate;
            err = snd_pcm_hw_params_set_rate_near(handle, hwParams,
                                                  &sampleRate, NULL);
            this->sampleRate = sampleRate;
            if (err < 0)
            {
                AlsaError(SS("Can't set sample rate to " << this->sampleRate << " (" << alsa_device_name << "/" << streamType << ")"));
            }
            if (!*channels)
            {
                /*if not user-specified, try to find the maximum
                 * number of channels */
                unsigned int channels_max = 0;
                unsigned int channels_min = 0;
                err = snd_pcm_hw_params_get_channels_max(hwParams,
                                                         &channels_max);
                if (err < 0)
                {
                    AlsaError(SS("Can't get channels_max."));
                }

                err = snd_pcm_hw_params_get_channels_min(hwParams,
                                                         &channels_min);
                if (err < 0)
                {
                    AlsaError(SS("Can't get channels_min."));
                }

                *channels = channels_max;

                if (ShouldForceStereoChannels(handle, hwParams, channels_min, channels_max))
                {
                    *channels = 2;
                }

                if (*channels >= 1024)
                {
                    // The default PCM device has unlimited channels.
                    // report 2 channels
                    *channels = 2;
                }
            }

            if ((err = snd_pcm_hw_params_set_channels(handle, hwParams,
                                                      *channels)) < 0)
            {
                AlsaError(SS("Can't set channel count to " << *channels << " (" << alsa_device_name << "/" << streamType << ")"));
            }

            snd_pcm_uframes_t effectivePeriodSize = this->bufferSize;

            int dir = 0;
            if ((err = snd_pcm_hw_params_set_period_size_near(handle, hwParams,
                                                              &effectivePeriodSize,
                                                              &dir)) < 0)
            {
                AlsaError(SS("Can't set period size to " << this->bufferSize << " (" << alsa_device_name << "/" << streamType << ")"));
            }
            *hwPeriodSize = effectivePeriodSize;

            *periods = this->numberOfBuffers;
            dir = 0;
            snd_pcm_hw_params_set_periods_min(handle, hwParams, periods, &dir);
            if (*periods < this->numberOfBuffers)
                *periods = this->numberOfBuffers;
            if (snd_pcm_hw_params_set_periods_near(handle, hwParams,
                                                   periods, NULL) < 0)
            {
                AlsaError(SS("Can't set number of periods to " << (*periods) << " (" << alsa_device_name << "/" << streamType << ")"));
            }

            if (*periods < this->numberOfBuffers)
            {
                AlsaError(SS("Got smaller periods " << *periods << " than " << this->numberOfBuffers));
            }

            snd_pcm_uframes_t bSize;

            // if ((err = snd_pcm_hw_params_set_buffer_size(handle, hwParams,
            //                                              *periods *
            //                                                  this->bufferSize)) < 0)
            // {
            //     AlsaError(SS("Can't set buffer length to " << (*periods * this->bufferSize)));
            // }

            if ((err = snd_pcm_hw_params(handle, hwParams)) < 0)
            {
                AlsaError(SS("Cannot set hardware parameters for " << alsa_device_name));
            }

            snd_pcm_sw_params_current(handle, swParams);

            if (isCaptureStream)
            {
                if ((err = snd_pcm_sw_params_set_start_threshold(handle, swParams,
                                                                 0)) < 0)
                {
                    AlsaError(SS("Cannot set start mode for " << alsa_device_name));
                }
            }
            else
            {
                if ((err = snd_pcm_sw_params_set_start_threshold(handle, swParams,
                                                                 0x7fffffff)) < 0)
                {
                    AlsaError(SS("Cannot set start mode for " << alsa_device_name));
                }
            }

            stop_th = *periods * *hwPeriodSize;
            if (this->soft_mode)
            {
                stop_th = (snd_pcm_uframes_t)-1;
            }

            if ((err = snd_pcm_sw_params_set_stop_threshold(
                     handle, swParams, stop_th)) < 0)
            {
                AlsaError(SS("ALSA: cannot set stop mode for " << alsa_device_name));
            }

            if ((err = snd_pcm_sw_params_set_silence_threshold(
                     handle, swParams, 0)) < 0)
            {
                AlsaError(SS("Cannot set silence threshold for " << alsa_device_name));
            }

            if (!isCaptureStream)
            {
                // For playback, set avail_min to one buffer size to minimize latency
                // while ensuring we have enough buffered data to prevent underruns
                snd_pcm_uframes_t playback_avail_min = this->bufferSize;
                err = snd_pcm_sw_params_set_avail_min(
                    handle, swParams, playback_avail_min);
            }
            else
            {
                err = snd_pcm_sw_params_set_avail_min(
                    handle, swParams, this->bufferSize);
            }

            if (err < 0)
            {
                AlsaError(SS("Cannot set avail min for " << alsa_device_name));
            }

            // err = snd_pcm_sw_params_set_tstamp_mode(handle, swParams, SND_PCM_TSTAMP_ENABLE);
            // if (err < 0)
            // {
            //     Lv2Log::info(SS(
            //         "Could not enable ALSA time stamp mode for " << alsa_device_name << " (err " << err << ")"));
            // }

#if SND_LIB_MAJOR >= 1 && SND_LIB_MINOR >= 1
            err = snd_pcm_sw_params_set_tstamp_type(handle, swParams, SND_PCM_TSTAMP_TYPE_MONOTONIC);
            if (err < 0)
            {
                Lv2Log::info(SS(
                    "Could not use monotonic ALSA time stamps for " << alsa_device_name << "(err " << err << ")"));
            }
#endif

            if ((err = snd_pcm_sw_params(handle, swParams)) < 0)
            {
                AlsaError(SS("Cannot set software parameters for " << alsa_device_name));
            }
            err = snd_pcm_prepare(handle);
            if (err < 0)
            {
                AlsaError(SS("ALSA prepare failed. " << snd_strerror(err)));
            }
        }
        void SetAlsaParameters(uint32_t bufferSize, uint32_t numberOfBuffers, uint32_t sampleRate)
        {
            this->bufferSize = bufferSize;
            this->numberOfBuffers = numberOfBuffers;
            this->sampleRate = sampleRate;

            if (this->captureHandle)
            {
                this->alsa_device_name = this->jackServerSettings.GetAlsaInputDevice();
                AlsaConfigureStream(
                    this->alsa_device_name,
                    "capture",
                    captureHandle,
                    captureHwParams,
                    captureSwParams,
                    &captureChannels,
                    &this->capturePeriods,
                    &this->captureHardwarePeriodSize);
            }
            if (this->playbackHandle)
            {
                this->alsa_device_name = this->jackServerSettings.GetAlsaOutputDevice();
                AlsaConfigureStream(
                    this->alsa_device_name,
                    "playback",
                    playbackHandle,
                    playbackHwParams,
                    playbackSwParams,
                    &playbackChannels,
                    &this->playbackPeriods,
                    &this->playbackHardwarePeriodSize);
            }

#ifdef ALSADRIVER_CONFIG_DBG
            snd_pcm_dump(captureHandle, snd_output);
            snd_pcm_dump(playbackHandle, snd_output);
#endif
        }

        int32_t EndianSwap(int32_t v)
        {
            int32_t b0 = v & 0xFF;
            int32_t b1 = (v >> 8) & 0xFF;
            int32_t b2 = (v >> 16) & 0xFF;
            int32_t b3 = (v >> 24) & 0xFF;

            return (b0 << 24) | (b1 << 16) | (b2 << 8) | (b3);
        }
        int16_t EndianSwap(int16_t v)
        {
            int16_t b0 = v & 0xFF;
            int16_t b1 = (v >> 8) & 0xFF;

            return (b0 << 8) | (b1);
        }
        void EndianSwap(float *p, float v_)
        {
            int32_t v = EndianSwap(*(int32_t *)&v_);
            *(int32_t *)p = v;
        }
        template <typename T>
        static T *getCaptureBuffer(std::vector<uint8_t> &buffer) { return (T *)(buffer.data()); }

        void CopyCaptureFloatBe(size_t frames)
        {
            int32_t *p = getCaptureBuffer<int32_t>(rawCaptureBuffer);

            std::vector<float *> &buffers = this->captureBuffers;
            int channels = this->captureChannels;
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    int32_t v = EndianSwap(*p);
                    ++p;

                    *(int32_t *)(buffers[channel] + frame) = v;
                }
            }
        }

        void CopyCaptureFloatLe(size_t frames)
        {
            float *p = getCaptureBuffer<float>(rawCaptureBuffer);

            std::vector<float *> &buffers = this->captureBuffers;
            int channels = this->captureChannels;
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    float v = *p++;
                    buffers[channel][frame] = v;
                }
            }
        }

        void CopyCaptureS16Le(size_t frames)
        {
            int16_t *p = getCaptureBuffer<int16_t>(rawCaptureBuffer);

            std::vector<float *> &buffers = this->captureBuffers;
            int channels = this->captureChannels;
            constexpr double scale = 1.0f / (std::numeric_limits<int16_t>::max() + 1L);
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    int16_t v = *p++;
                    buffers[channel][frame] = scale * v;
                }
            }
        }
        void CopyCaptureS16Be(size_t frames)
        {
            int16_t *p = getCaptureBuffer<int16_t>(rawCaptureBuffer);

            std::vector<float *> &buffers = this->captureBuffers;
            int channels = this->captureChannels;
            constexpr float scale = 1.0f / (std::numeric_limits<int16_t>::max() + 1L);
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    int16_t v = EndianSwap(*p++);
                    buffers[channel][frame] = scale * v;
                }
            }
        }

        void CopyCaptureS32Le(size_t frames)
        {
            int32_t *p = getCaptureBuffer<int32_t>(rawCaptureBuffer);

            std::vector<float *> &buffers = this->captureBuffers;
            int channels = this->captureChannels;
            constexpr float scale = 1.0f / (std::numeric_limits<int32_t>::max() + 1L);
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    int32_t v = *p++;
                    buffers[channel][frame] = scale * v;
                }
            }
        }
        void CopyCaptureS24_3Le(size_t frames)
        {
            uint8_t *p = getCaptureBuffer<uint8_t>(rawCaptureBuffer);

            std::vector<float *> &buffers = this->captureBuffers;
            int channels = this->captureChannels;
            constexpr float scale = 1.0f / (std::numeric_limits<int32_t>::max() + 1LL);
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    int32_t v = (p[0] << 8) + (p[1] << 16) | (p[2] << 24);
                    p += 3;
                    buffers[channel][frame] = scale * v;
                }
            }
        }
        void CopyCaptureS24_3Be(size_t frames)
        {
            uint8_t *p = (uint8_t *)rawCaptureBuffer.data();

            std::vector<float *> &buffers = this->captureBuffers;
            int channels = this->captureChannels;
            constexpr float scale = 1.0f / (std::numeric_limits<int32_t>::max() + 1LL);
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    int32_t v = (p[2] << 8) + (p[1] << 16) | (p[0] << 24);
                    p += 3;
                    buffers[channel][frame] = scale * v;
                }
            }
        }
        void CopyCaptureS24Le(size_t frames)
        {
            int32_t *p = (int32_t *)rawCaptureBuffer.data();

            std::vector<float *> &buffers = this->captureBuffers;
            int channels = this->captureChannels;
            constexpr float scale = 1.0f / (0x00FFFFFFL + 1L);
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    int32_t v = *p++;
                    buffers[channel][frame] = scale * v;
                }
            }
        }
        void CopyCaptureS24Be(size_t frames)
        {
            int32_t *p = (int32_t *)rawCaptureBuffer.data();

            std::vector<float *> &buffers = this->captureBuffers;
            int channels = this->captureChannels;
            constexpr float scale = 1.0f / (0x00FFFFFFL + 1L);
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    int32_t v = EndianSwap(*p++);
                    buffers[channel][frame] = scale * v;
                }
            }
        }
        void CopyCaptureS32Be(size_t frames)
        {
            int32_t *p = (int32_t *)rawCaptureBuffer.data();

            std::vector<float *> &buffers = this->captureBuffers;
            int channels = this->captureChannels;
            constexpr float scale = 1.0f / (std::numeric_limits<int32_t>::max() + 1L);
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    int32_t v = EndianSwap(*p++);
                    buffers[channel][frame] = scale * v;
                }
            }
        }
        void CopyPlaybackS16Le(size_t frames)
        {
            int16_t *p = (int16_t *)rawPlaybackBuffer.data();

            std::vector<float *> &buffers = this->playbackBuffers;
            int channels = this->playbackChannels;
            constexpr float scale = std::numeric_limits<int16_t>::max();
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    float v = buffers[channel][frame];
                    if (v > 1.0f)
                        v = 1.0f;
                    else if (v < -1.0f)
                        v = -1.0f;
                    *p++ = (int16_t)(scale * v);
                }
            }
        }
        void CopyPlaybackS16Be(size_t frames)
        {
            int16_t *p = (int16_t *)rawPlaybackBuffer.data();

            std::vector<float *> &buffers = this->playbackBuffers;
            int channels = this->playbackChannels;
            constexpr float scale = std::numeric_limits<int16_t>::max();
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    float v = buffers[channel][frame];
                    if (v > 1.0f)
                        v = 1.0f;
                    else if (v < -1.0f)
                        v = -1.0f;
                    *p++ = EndianSwap((int16_t)(scale * v));
                }
            }
        }
        void CopyPlaybackS32Le(size_t frames)
        {
            int32_t *p = (int32_t *)rawPlaybackBuffer.data();

            std::vector<float *> &buffers = this->playbackBuffers;
            int channels = this->playbackChannels;
            constexpr double scale = std::numeric_limits<int32_t>::max();
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    float v = buffers[channel][frame];
                    if (v > 1.0f)
                        v = 1.0f;
                    else if (v < -1.0f)
                        v = -1.0f;
                    *p++ = (int32_t)(scale * v);
                }
            }
        }
        void CopyPlaybackS24Le(size_t frames)
        {
            // 24 bits in low bits of an int32_t.

            int32_t *p = (int32_t *)rawPlaybackBuffer.data();

            std::vector<float *> &buffers = this->playbackBuffers;
            int channels = this->playbackChannels;
            constexpr double scale = 0x00FFFFFF;
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    float v = buffers[channel][frame];
                    if (v > 1.0f)
                        v = 1.0f;
                    else if (v < -1.0f)
                        v = -1.0f;
                    *p++ = (int32_t)(scale * v);
                }
            }
        }
        void CopyPlaybackS24Be(size_t frames)
        {
            // 24 bits in low bits of an int32_t.

            int32_t *p = (int32_t *)rawPlaybackBuffer.data();

            std::vector<float *> &buffers = this->playbackBuffers;
            int channels = this->playbackChannels;
            constexpr double scale = 0x00FFFFFF;
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    float v = buffers[channel][frame];
                    if (v > 1.0f)
                        v = 1.0f;
                    else if (v < -1.0f)
                        v = -1.0f;
                    *p++ = EndianSwap((int32_t)(scale * v));
                }
            }
        }
        void CopyPlaybackS32Be(size_t frames)
        {
            int32_t *p = (int32_t *)rawPlaybackBuffer.data();

            std::vector<float *> &buffers = this->playbackBuffers;
            int channels = this->playbackChannels;
            constexpr double scale = std::numeric_limits<int32_t>::max();
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    float v = buffers[channel][frame];
                    if (v > 1.0f)
                        v = 1.0f;
                    else if (v < -1.0f)
                        v = -1.0f;
                    *p++ = EndianSwap((int32_t)(scale * v));
                }
            }
        }
        void CopyPlaybackS24_3Be(size_t frames)
        {
            uint8_t *p = (uint8_t *)rawPlaybackBuffer.data();

            std::vector<float *> &buffers = this->playbackBuffers;
            int channels = this->playbackChannels;
            constexpr double scale = std::numeric_limits<int32_t>::max();
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    float v = buffers[channel][frame];
                    if (v > 1.0f)
                        v = 1.0f;
                    else if (v < -1.0f)
                        v = -1.0f;
                    int32_t iValue = (int32_t)(scale * v);
                    p[0] = (uint8_t)(iValue >> 24);
                    p[1] = (uint8_t)(iValue >> 16);
                    p[2] = (uint8_t)(iValue >> 8);

                    p += 3;
                }
            }
        }
        void CopyPlaybackS24_3Le(size_t frames)
        {
            uint8_t *p = (uint8_t *)rawPlaybackBuffer.data();

            std::vector<float *> &buffers = this->playbackBuffers;
            int channels = this->playbackChannels;
            constexpr double scale = std::numeric_limits<int32_t>::max();
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    float v = buffers[channel][frame];
                    if (v > 1.0f)
                        v = 1.0f;
                    else if (v < -1.0f)
                        v = -1.0f;
                    int32_t iValue = (int32_t)(scale * v);
                    p[0] = (uint8_t)(iValue >> 8);
                    p[1] = (uint8_t)(iValue >> 16);
                    p[2] = (uint8_t)(iValue >> 24);

                    p += 3;
                }
            }
        }

        void CopyPlaybackFloatLe(size_t frames)
        {
            float *p = (float *)rawPlaybackBuffer.data();

            std::vector<float *> &buffers = this->playbackBuffers;
            int channels = this->playbackChannels;
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    float v = buffers[channel][frame];
                    *p++ = v;
                }
            }
        }
        void CopyPlaybackFloatBe(size_t frames)
        {
            float *p = (float *)rawPlaybackBuffer.data();

            std::vector<float *> &buffers = this->playbackBuffers;
            int channels = this->playbackChannels;
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    float v = buffers[channel][frame];
                    EndianSwap(p, v);
                    p++;
                }
            }
        }

    public:
        void TestFormatEncodeDecode(snd_pcm_format_t captureFormat);

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

            open = true;
            try
            {
                OpenAudio(jackServerSettings, channelSelection);
                std::atomic_thread_fence(std::memory_order::release);
            }
            catch (const std::exception &e)
            {
                std::atomic_thread_fence(std::memory_order::release);

                Close();
                throw;
            }
        }

        void RestartAlsa()
        {
            std::lock_guard lock{restartMutex};
            Lv2Log::debug("Restarting ALSA devices.");

            try
            {
                AlsaCloseAudio();
            }
            catch (const std::exception &e)
            {
                Lv2Log::error(SS("Error cleaning up ALSA: " << e.what()));
                throw std::runtime_error("Unable to restart the audio stream.");
            }
            try
            {
                OpenAudio(this->jackServerSettings, this->channelSelection);
                validate_capture_handle();
                FillOutputBuffer();
            }
            catch (const std::exception &e)
            {
                Lv2Log::error(SS("Error opening ALSA: " << e.what()));
                throw std::runtime_error("Unable to restart the audio stream.");
            }
            int err;
            
            if ((err = snd_pcm_start(captureHandle)) < 0)
            {
                Lv2Log::error(SS("Unable to restart ALSA capture: " << snd_strerror(err)));
                throw PiPedalStateException("Unable to restart ALSA capture.");
            }
            TraceBufferPositions(0,'+');
            audioRunning = true;
        }

        void PrepareCaptureFunctions(snd_pcm_format_t captureFormat)
        {
            this->captureFormat = captureFormat;

            switch (captureFormat)
            {
            case SND_PCM_FORMAT_FLOAT_LE:
                captureSampleSize = 4;
                copyInputFn = &AlsaDriverImpl::CopyCaptureFloatLe;
                break;
            case SND_PCM_FORMAT_S24_3LE:
                copyInputFn = &AlsaDriverImpl::CopyCaptureS24_3Le;
                captureSampleSize = 3;
                break;
            case SND_PCM_FORMAT_S32_LE:
                captureSampleSize = 4;
                copyInputFn = &AlsaDriverImpl::CopyCaptureS32Le;
                break;
            case SND_PCM_FORMAT_S24_LE:
                captureSampleSize = 4;
                copyInputFn = &AlsaDriverImpl::CopyCaptureS24Le;
                break;
            case SND_PCM_FORMAT_S16_LE:
                captureSampleSize = 2;
                copyInputFn = &AlsaDriverImpl::CopyCaptureS16Le;
                break;
            case SND_PCM_FORMAT_FLOAT_BE:
                captureSampleSize = 4;
                copyInputFn = &AlsaDriverImpl::CopyCaptureFloatBe;
                captureSampleSize = 4;
                break;
            case SND_PCM_FORMAT_S24_3BE:
                captureSampleSize = 3;
                copyInputFn = &AlsaDriverImpl::CopyCaptureS24_3Be;
                break;
            case SND_PCM_FORMAT_S32_BE:
                copyInputFn = &AlsaDriverImpl::CopyCaptureS32Be;
                captureSampleSize = 4;
                break;
            case SND_PCM_FORMAT_S24_BE:
                copyInputFn = &AlsaDriverImpl::CopyCaptureS24Be;
                captureSampleSize = 4;
                break;
            case SND_PCM_FORMAT_S16_BE:
                copyInputFn = &AlsaDriverImpl::CopyCaptureS16Be;
                captureSampleSize = 2;
                break;
            default:
                break;
            }
            if (copyInputFn == nullptr)
            {
                throw PiPedalStateException(SS("Audio input format not supported. (" << captureFormat << ")"));
            }

            captureFrameSize = captureSampleSize * captureChannels;
            rawCaptureBuffer.resize(captureFrameSize * bufferSize * 2);
            memset(rawCaptureBuffer.data(), 0, rawCaptureBuffer.size());

            AllocateBuffers(captureBuffers, captureChannels);
        }

        virtual std::string GetConfigurationDescription()
        {
            std::string result = SS(
                "ALSA, "
                << this->alsa_device_name
                << ", " << GetAlsaFormatDescription(this->captureFormat)
                << ", " << this->sampleRate
                << ", " << this->bufferSize << "x" << this->numberOfBuffers
                << ", in: " << this->InputBufferCount() << "/" << this->captureChannels
                << ", out: " << this->OutputBufferCount() << "/" << this->playbackChannels);
            return result;
        }
        void PreparePlaybackFunctions(snd_pcm_format_t playbackFormat)
        {
            copyOutputFn = nullptr;
            switch (playbackFormat)
            {
            case SND_PCM_FORMAT_FLOAT_LE:
                playbackSampleSize = 4;
                copyOutputFn = &AlsaDriverImpl::CopyPlaybackFloatLe;
                break;
            case SND_PCM_FORMAT_S24_3LE:
                copyOutputFn = &AlsaDriverImpl::CopyPlaybackS24_3Le;
                playbackSampleSize = 3;
                break;
            case SND_PCM_FORMAT_S32_LE:
                copyOutputFn = &AlsaDriverImpl::CopyPlaybackS32Le;
                playbackSampleSize = 4;
                break;
            case SND_PCM_FORMAT_S24_LE:
                copyOutputFn = &AlsaDriverImpl::CopyPlaybackS24Le;
                playbackSampleSize = 4;
                break;
            case SND_PCM_FORMAT_S16_LE:
                copyOutputFn = &AlsaDriverImpl::CopyPlaybackS16Le;
                playbackSampleSize = 2;
                break;
            case SND_PCM_FORMAT_FLOAT_BE:
                copyOutputFn = &AlsaDriverImpl::CopyPlaybackFloatBe;
                playbackSampleSize = 4;
                break;
            case SND_PCM_FORMAT_S24_3BE:
                copyOutputFn = &AlsaDriverImpl::CopyPlaybackS24_3Be;
                playbackSampleSize = 3;
                break;
            case SND_PCM_FORMAT_S32_BE:
                copyOutputFn = &AlsaDriverImpl::CopyPlaybackS32Be;
                playbackSampleSize = 4;
                break;
            case SND_PCM_FORMAT_S24_BE:
                copyOutputFn = &AlsaDriverImpl::CopyPlaybackS24Be;
                playbackSampleSize = 4;
                break;
            case SND_PCM_FORMAT_S16_BE:
                copyOutputFn = &AlsaDriverImpl::CopyPlaybackS16Be;
                playbackSampleSize = 2;
                break;
            default:
                break;
            }
            if (copyOutputFn == nullptr)
            {
                throw PiPedalStateException(SS("Unsupported audio output format. (" << playbackFormat << ")"));
            }

            playbackFrameSize = playbackSampleSize * playbackChannels;
            rawPlaybackBuffer.resize(playbackFrameSize * bufferSize);
            memset(rawPlaybackBuffer.data(), 0, playbackFrameSize * bufferSize);

            AllocateBuffers(playbackBuffers, playbackChannels);
        }

        void OpenAudio(const JackServerSettings &jackServerSettings, const JackChannelSelection &channelSelection)
        {
            std::lock_guard lock{restartMutex};

            int err;

            std::string inputName  = jackServerSettings.GetAlsaInputDevice();
            std::string outputName = jackServerSettings.GetAlsaOutputDevice();


            this->numberOfBuffers = jackServerSettings.GetNumberOfBuffers();
            this->bufferSize = jackServerSettings.GetBufferSize();
            this->user_threshold = jackServerSettings.GetBufferSize();

            try
            {

                this->alsa_device_name = outputName;
                err = snd_pcm_open(&playbackHandle, outputName.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
                if (err < 0)
                {
                    switch (errno)
                    {
                    case EBUSY:
                    {
                        std::string apps = discover_alsa_using_apps();
                        std::string message;
                        if (apps.size() != 0)
                        {
                            message =
                                SS("Device " << alsa_device_name << " in use. The following applications are using your soundcard: " << apps
                                             << ". Stop them as neccesary before trying to  start pipedald.");
                        }
                        else
                        {
                            message =
                                SS("Device " << alsa_device_name << " in use. Stop the application using it before trying to restart pipedald. ");
                        }
                        Lv2Log::error(message);
                        throw PiPedalStateException(std::move(message));
                    }
                    break;
                    case EPERM:
                        throw PiPedalStateException(SS("Permission denied opening device '" << alsa_device_name << "'"));
                    default:
                        throw PiPedalStateException(SS("Unexepected error (" << errno << ") opening device '" << alsa_device_name << "'"));
                    }
                }
                if (this->playbackHandle)
                {
                    snd_pcm_nonblock(playbackHandle, 0);
                }
                
                this->alsa_device_name = inputName;
                err = snd_pcm_open(&captureHandle, inputName.c_str(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);

                if (err < 0)
                {
                    switch (errno)
                    {
                    case EBUSY:
                    {
                        std::string apps = discover_alsa_using_apps();
                        std::string message;
                        if (apps.size() != 0)
                        {
                            message =
                                SS("Device " << alsa_device_name << " in use. The following applications are using your soundcard: " << apps
                                             << ". Stop them as neccesary before trying to restart pipedald.");
                        }
                        else
                        {
                            message =
                                SS("Device " << alsa_device_name << " in use. Stop the application using it before trying to restart pipedald. ");
                        }
                        Lv2Log::error(message);
                        throw PiPedalStateException(std::move(message));
                    }
                    break;
                    case EPERM:
                        throw PiPedalStateException(SS("Permission denied opening device '" << alsa_device_name << "'"));
                    default:
                        throw PiPedalStateException(SS("Unexepected error (" << errno << ") opening device '" << alsa_device_name << "'"));
                    }
                }
                if (this->captureHandle)
                {
                    snd_pcm_nonblock(captureHandle, 0);
                }

                if ((err = snd_pcm_hw_params_malloc(&captureHwParams)) < 0)
                {
                    throw PiPedalStateException("Failed to allocate captureHwParams");
                }
                if ((err = snd_pcm_sw_params_malloc(&captureSwParams)) < 0)
                {
                    throw PiPedalStateException("Failed to allocate captureSwParams");
                }
                if ((err = snd_pcm_hw_params_malloc(&playbackHwParams)) < 0)
                {
                    throw PiPedalStateException("Failed to allocate playbackHwParams");
                }
                if ((err = snd_pcm_sw_params_malloc(&playbackSwParams)) < 0)
                {
                    throw PiPedalStateException("Failed to allocate playbackSwParams");
                }

                SetAlsaParameters(jackServerSettings.GetBufferSize(), jackServerSettings.GetNumberOfBuffers(), jackServerSettings.GetSampleRate());
                capture_and_playback_not_synced = false;

                if (captureHandle && playbackHandle)
                {
                    if (snd_pcm_link(playbackHandle,
                                     captureHandle) != 0)
                    {
                        capture_and_playback_not_synced = true;
                    }
                }

                snd_pcm_format_t captureFormat;
                snd_pcm_hw_params_get_format(captureHwParams, &captureFormat);
                copyInputFn = nullptr;

                PrepareCaptureFunctions(captureFormat);

                snd_pcm_format_t playbackFormat;
                snd_pcm_hw_params_get_format(playbackHwParams, &playbackFormat);

                PreparePlaybackFunctions(playbackFormat);
            }
            catch (const std::exception &e)
            {
                AlsaCleanup();
                throw;
            }
        }

        void FillOutputBuffer()
        {
            validate_capture_handle();

            memset(rawPlaybackBuffer.data(), 0, rawPlaybackBuffer.size());
            int retry = 0;
            while (true)
            {
                auto avail = snd_pcm_avail(this->playbackHandle);
                if (avail < 0)
                {
                    if (avail == -EAGAIN)
                    {
                        return;
                    }
                    if (++retry >= 5) // kinda sus code. let's make sure we don't spin forever.
                    {
                        throw std::runtime_error("Timed out trying to fill the audio output buffer.");
                    }

                    int err = snd_pcm_prepare(playbackHandle);
                    if (err < 0)
                    {
                        throw PiPedalStateException(SS("Audio playback failed. " << snd_strerror(err)));
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(10));
                    continue;
                }
                if (avail == 0)
                    break;

                if (avail * playbackFrameSize > this->rawPlaybackBuffer.size())
                    this->rawPlaybackBuffer.resize(avail * playbackFrameSize);

                ssize_t err = WriteBuffer(playbackHandle, rawPlaybackBuffer.data(), avail);
                if (err < 0)
                {
                    throw PiPedalStateException(SS("Audio playback failed. " << snd_strerror(err)));
                }
            }
            validate_capture_handle();
        }
        void recover_from_output_underrun(snd_pcm_t *capture_handle, snd_pcm_t *playback_handle, int err, size_t framesRead)
        {
            validate_capture_handle();
            try
            {

                TraceBufferPositions(framesRead, 'w');
                if (err == -EPIPE)
                {
                    err = snd_pcm_prepare(playback_handle);
                    if (err < 0)
                    {
                        Lv2Log::error(SS("Can't recover from ALSA output underrun. (" << snd_strerror(err) << ")"));
                        throw PiPedalStateException(SS("Can't recover from ALSA output underrun. (" << snd_strerror(err) << ")"));
                    }
                    snd_pcm_drain(capture_handle);
                    FillOutputBuffer();
                    TraceBufferPositions(framesRead, 'x');
                }
                else
                {
                    TraceBufferPositions(framesRead, 'z');
                    Lv2Log::error(SS("Can't recover from ALSA output underrun. (" << snd_strerror(err) << ")"));
                    throw PiPedalStateException(SS("Can't recover from ALSA output error. (" << snd_strerror(err) << ")"));
                }
            }
            catch (const std::exception &e)
            {
                RestartAlsa();
                audioRunning = true;
            }
            validate_capture_handle();
        }
        void recover_from_input_underrun(snd_pcm_t *capture_handle, snd_pcm_t *playback_handle, int err, size_t bufferedFrames)
        {
            validate_capture_handle();

            try
            {
                TraceBufferPositions(bufferedFrames, 'r');
                if (err == -EPIPE)
                {

                    // Unlink the streams before recovery
                    snd_pcm_unlink(capture_handle);

                    // Prepare both streams
                    if ((err = snd_pcm_prepare(playback_handle)) < 0)
                    {
                        throw std::runtime_error(SS("Cannot prepare playback stream: " << snd_strerror(err)));
                    }
                    if ((err = snd_pcm_prepare(capture_handle)) < 0)
                    {
                        throw std::runtime_error(SS("Cannot prepare capture stream: " << snd_strerror(err)));
                    }

                    // Resynchronize the streams
                    if ((err = snd_pcm_link(capture_handle, playback_handle)) < 0)
                    {
                        throw std::runtime_error(SS("Cannot relink streams: " << snd_strerror(err)));
                    }

                    // Start the streams
                    FillOutputBuffer();
                    if ((err = snd_pcm_start(capture_handle)) < 0)
                    {
                        throw std::runtime_error(SS("Cannot restart capture stream: " << snd_strerror(err)));
                    }

                    validate_capture_handle();
                }
                else if (err == ESTRPIPE)
                {
                    audioRunning = false;
                    validate_capture_handle();

                    while ((err = snd_pcm_resume(capture_handle)) == -EAGAIN)
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    }
                    if (err < 0)
                    {
                        err = snd_pcm_prepare(capture_handle);
                        if (err < 0)
                        {
                            throw PiPedalStateException(SS("Can't recover from ALSA suspend. (" << snd_strerror(err) << ")"));
                        }
                    }
                    audioRunning = true;
                    validate_capture_handle();
                }
                else
                {
                    throw PiPedalStateException(SS("Can't recover from ALSA input error. (" << snd_strerror(err) << ")"));
                }
            }
            catch (const std::exception &e)
            {
                RestartAlsa();
                audioRunning = true;
            }
        }

        void DumpStatus(snd_pcm_t *handle)
        {
#ifdef ALSADRIVER_CONFIG_DBG
            snd_pcm_status(handle, snd_status);
            snd_pcm_status_dump(snd_status, snd_output);
#endif
        }

        std::unique_ptr<std::jthread> audioThread;
        bool audioRunning;

        bool block = false;

        snd_pcm_sframes_t ReadBuffer(snd_pcm_t *handle, uint8_t *buffer, snd_pcm_uframes_t frames)
        {
            // transcode to jack format.
            // expand running status if neccessary.
            // deal with regular and sysex messages split across
            // buffer boundaries (but discard them)
            snd_pcm_sframes_t framesRead = 0;

            auto state = snd_pcm_state(handle);
            auto frame_bytes = this->captureFrameSize;
            do
            {
                TraceBufferPositions(framesRead,'1');

                framesRead = snd_pcm_readi(handle, buffer, frames);
                if (framesRead < 0)
                {
                    return framesRead;
                }
                if (framesRead > 0)
                {
                    buffer += framesRead * frame_bytes;
                    frames -= framesRead;
                }
                if (framesRead == 0)
                {
                    snd_pcm_wait(handle, frames);
                }
            } while (frames > 0);

            TraceBufferPositions(framesRead,'2');

            return framesRead;
        }

    protected:
        void ReadMidiData(uint32_t audioFrame)
        {
            AlsaMidiMessage message;

            midiEventCount = 0;
            auto alsaSequener = this->alsaSequencer; // take an addref
            if (!alsaSequener)
            {
                return;
            }
            while (alsaSequencer->ReadMessage(message, 0))
            {
                size_t messageSize = message.size;
                if (messageSize == 0)
                {
                    continue;
                }
                if (midiEventMemoryIndex + messageSize >= this->midiEventMemory.size())
                {
                    continue;
                }
                if (midiEventCount >= this->midiEvents.size())
                {
                    midiEvents.resize(midiEventCount * 2);
                }
                // for now, prevent META event messages from propagating.
                if (message.data[0] == 0xFF && message.size > 1)
                {
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

    private:
        long WriteBuffer(snd_pcm_t *handle, uint8_t *buf, size_t frames)
        {
            long framesRead;
            auto frame_bytes = this->playbackFrameSize;

            while (frames > 0)
            {
                framesRead = snd_pcm_writei(handle, buf, frames);
                if (framesRead == -EAGAIN)
                    continue;
                if (framesRead < 0)
                    return framesRead;
                buf += framesRead * frame_bytes;
                frames -= framesRead;
            }
            return 0;
        }
        void AudioThread()
        {
            SetThreadName("alsaDriver");

            try
            {
                SetThreadPriority(SchedulerPriority::RealtimeAudio);

                bool ok = true;

                auto playbackState = snd_pcm_state(playbackHandle);

                FillOutputBuffer();

                int err;
                if ((err = snd_pcm_start(captureHandle)) < 0)
                {
                    throw PiPedalStateException(SS("Unable to start ALSA capture. " << snd_strerror(err)));
                }

                CrashGuardLock crashGuardLock;

                cpuUse.SetStartTime(cpuUse.Now());
                while (true)
                {
                    validate_capture_handle();
                    cpuUse.UpdateCpuUse();

                    if (terminateAudio())
                    {
                        break;
                    }
                    this->midiEventCount = 0;

                    // snd_pcm_wait(captureHandle, 1);
                    ssize_t framesToRead = bufferSize;
                    ssize_t framesRead = 0;
                    bool xrun = false;
                    validate_capture_handle();

                    while (framesToRead != 0)
                    {
                        ReadMidiData((uint32_t)framesRead);

                        ssize_t thisTime = framesToRead;
                        ssize_t nFrames;
                        if ((nFrames = ReadBuffer(
                                 captureHandle,
                                 this->rawCaptureBuffer.data() + this->captureFrameSize * framesRead,
                                 framesToRead)) < 0)
                        {
                            this->driverHost->OnUnderrun();
                            recover_from_input_underrun(captureHandle, playbackHandle, nFrames, framesRead);
                            xrun = true;
                            break;
                        }
                        framesRead += nFrames;
                        framesToRead -= nFrames;
                    }
                    validate_capture_handle();

                    if (xrun)
                    {
                        continue;
                    }
                    cpuUse.AddSample(ProfileCategory::Read);
                    if (framesRead == 0)
                        continue;
                    if (framesRead != bufferSize)
                    {
                        throw PiPedalStateException("Invalid read.");
                    }

                    (this->*copyInputFn)(framesRead);
                    cpuUse.AddSample(ProfileCategory::Driver);

                    this->driverHost->OnProcess(framesRead);

                    cpuUse.AddSample(ProfileCategory::Execute);

                    (this->*copyOutputFn)(framesRead);
                    cpuUse.AddSample(ProfileCategory::Driver);
                    // process.

                    ssize_t err = WriteBuffer(playbackHandle, rawPlaybackBuffer.data(), framesRead);

                    if (err < 0)
                    {
                        this->driverHost->OnUnderrun();

                        recover_from_output_underrun(captureHandle, playbackHandle, err, framesRead);
                        framesRead = 0;
                    }
                    cpuUse.AddSample(ProfileCategory::Write);
                }
            }
            catch (const std::exception &e)
            {
                Lv2Log::error(e.what());
                Lv2Log::error("ALSA audio thread terminated abnormally.");
            }

            // if we terminated abnormally, pump messages until we have been terminated.
            if (!terminateAudio())
            {
                this->driverHost->OnAlsaDriverStopped();
                // zero out input buffers.
                for (size_t i = 0; i < this->captureBuffers.size(); ++i)
                {
                    float *pBuffer = captureBuffers[i];
                    for (size_t j = 0; j < this->bufferSize; ++j)
                    {
                        pBuffer[j] = 0;
                    }
                }
                try
                {
                    while (!terminateAudio())
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(10));
                        this->driverHost->OnProcess(this->bufferSize);
                    }
                }
                catch (const std::exception &e)
                {
                }
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
                this->audioThread = 0; // jthread joins.
            }
            Lv2Log::debug("Audio thread joined.");
        }

        static constexpr size_t MIDI_MEMORY_BUFFER_SIZE = 32 * 1024;
        static constexpr size_t MAX_MIDI_EVENT = 4 * 1024;

        size_t midiEventCount = 0;
        std::vector<MidiEvent> midiEvents;
        size_t midiEventMemoryIndex = 0;
        std::vector<uint8_t> midiEventMemory;
        AlsaSequencer::ptr alsaSequencer;

    public:
        virtual void SetAlsaSequencer(AlsaSequencer::ptr alsaSequencer) override
        {
            this->alsaSequencer = alsaSequencer;
        }

        virtual size_t InputBufferCount() const { return activeCaptureBuffers.size(); }
        virtual float *GetInputBuffer(size_t channel) override
        {
            return activeCaptureBuffers[channel];
        }

        virtual size_t GetMidiInputEventCount() override
        {
            return midiEventCount;
        }
        virtual MidiEvent *GetMidiEvents() override
        {
            return this->midiEvents.data();
        }

        virtual size_t OutputBufferCount() const { return activePlaybackBuffers.size(); }
        virtual float *GetOutputBuffer(size_t channel) override
        {
            return activePlaybackBuffers[channel];
        }

        void FreeBuffers(std::vector<float *> &buffer)
        {
            for (size_t i = 0; i < buffer.size(); ++i)
            {
                delete[] buffer[i];
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
        }
        virtual void Close()
        {
            std::atomic_thread_fence(std::memory_order::acquire);

            if (!open)
            {
                return;
            }
            open = false;
            Deactivate();
            AlsaCleanup();
            DeleteBuffers();
            this->alsaSequencer = nullptr;

            std::atomic_thread_fence(std::memory_order::release);
        }

        virtual float CpuUse()
        {
            return cpuUse.GetCpuUse();
        }

        virtual float CpuOverhead()
        {
            return cpuUse.GetCpuOverhead();
        }
    };

    AudioDriver *CreateAlsaDriver(AudioDriverHost *driverHost)
    {
        return new AlsaDriverImpl(driverHost);
    }

    bool GetAlsaChannels(const JackServerSettings &jackServerSettings,
                         std::vector<std::string> &inputAudioPorts,
                         std::vector<std::string> &outputAudioPorts)
    {
        if (jackServerSettings.IsDummyAudioDevice())
        {
            auto nChannels = GetDummyAudioChannels(jackServerSettings.GetAlsaInputDevice());

            inputAudioPorts.clear();
            outputAudioPorts.clear();
            for (uint32_t i = 0; i < nChannels; ++i)
            {
                inputAudioPorts.push_back(std::string(SS("system::capture_" << i)));
                outputAudioPorts.push_back(std::string(SS("system::playback_" << i)));
            }
            return true;
        }

        snd_pcm_t *playbackHandle = nullptr;
        snd_pcm_t *captureHandle = nullptr;
        snd_pcm_hw_params_t *playbackHwParams = nullptr;
        snd_pcm_hw_params_t *captureHwParams = nullptr;

        Finally ff_playbackHandle{
            [&playbackHandle]()
            {
                if (playbackHandle)
                {
                    int rc = snd_pcm_close(playbackHandle);
                    if (rc < 0)
                    {
                        throw std::runtime_error("snd_pcm_close failed.");
                    }
                    playbackHandle = nullptr;
                }
            }};
        Finally ff_captureHandle{
            [&captureHandle]()
            {
                if (captureHandle)
                {
                    int rc = snd_pcm_close(captureHandle);
                    if (rc < 0)
                    {
                        throw std::runtime_error("snd_pcm_close failed.");
                    }

                    captureHandle = nullptr;
                }
            }};
        Finally ff_playbackHwParams{
            [&playbackHwParams]()
            {
                if (playbackHwParams)
                {
                    snd_pcm_hw_params_free(playbackHwParams);
                }
            }};
        Finally ff_captureHwParams{
            [&captureHwParams]()
            {
                if (captureHwParams)
                {
                    snd_pcm_hw_params_free(captureHwParams);
                }
            }};

        std::string alsaDeviceName = jackServerSettings.GetAlsaInputDevice();
        bool result = false;

        try
        {
            int err;
            for (int retry = 0; retry < 4; ++retry)
            {
                err = snd_pcm_open(&playbackHandle, alsaDeviceName.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
                if (err < 0) // field report of a device that is present, but won't immediately open.
                {
                    sleep(1);
                    continue;
                }
                break;
            }
            if (err < 0)
            {
                throw PiPedalStateException(SS(alsaDeviceName << " playback device not found. "
                                                              << "(" << snd_strerror(err) << ")"));
            }

            for (int retry = 0; retry < 15; ++retry)
            {
                err = snd_pcm_open(&captureHandle, alsaDeviceName.c_str(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
                if (err == -EBUSY)
                {
                    sleep(1);
                    continue;
                }
                break;
            }
            if (err < 0)
                throw PiPedalStateException(SS(alsaDeviceName << " capture device not found."));

            if (snd_pcm_hw_params_malloc(&playbackHwParams) < 0)
            {
                throw PiPedalLogicException("Out of memory.");
            }
            if (snd_pcm_hw_params_malloc(&captureHwParams) < 0)
            {
                throw PiPedalLogicException("Out of memory.");
            }

            snd_pcm_hw_params_any(playbackHandle, playbackHwParams);
            snd_pcm_hw_params_any(captureHandle, captureHwParams);

            SetPreferredAlsaFormat(alsaDeviceName, "capture", captureHandle, captureHwParams);
            SetPreferredAlsaFormat(alsaDeviceName, "output", playbackHandle, playbackHwParams);

            unsigned int sampleRate = jackServerSettings.GetSampleRate();
            err = snd_pcm_hw_params_set_rate_near(playbackHandle, playbackHwParams, &sampleRate, 0);
            if (err < 0)
            {
                throw PiPedalLogicException("Sample rate not supported.");
            }
            sampleRate = jackServerSettings.GetSampleRate();
            err = snd_pcm_hw_params_set_rate_near(captureHandle, captureHwParams, &sampleRate, 0);
            if (err < 0)
            {
                throw PiPedalLogicException("Sample rate not supported.");
            }

            unsigned int playbackChannels, captureChannels;

            err = snd_pcm_hw_params_get_channels_max(playbackHwParams, &playbackChannels);
            if (err < 0)
            {
                throw PiPedalLogicException("No outut channels.");
            }
            unsigned int channelsMin;
            err = snd_pcm_hw_params_get_channels_min(playbackHwParams, &channelsMin);
            if (err < 0)
            {
                throw PiPedalLogicException("No outut channels.");
            }
            if (ShouldForceStereoChannels(playbackHandle, playbackHwParams, channelsMin, playbackChannels))
            {
                playbackChannels = 2;
            }

            err = snd_pcm_hw_params_get_channels_max(captureHwParams, &captureChannels);
            if (err < 0)
            {
                throw PiPedalLogicException("No input channels.");
            }
            err = snd_pcm_hw_params_get_channels_min(captureHwParams, &channelsMin);
            if (err >= 0)
            {
                if (ShouldForceStereoChannels(captureHandle, captureHwParams, channelsMin, captureChannels))
                {
                    captureChannels = 2;
                }
            }

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
            result = false;
            throw;
        }
        return result;
    }

    static void AlsaAssert(bool value)
    {
        if (!value)
            throw PiPedalStateException("Assert failed.");
    }

#ifdef JUNK
    static void ExpectEvent(AlsaDriverImpl::AlsaMidiDeviceImpl &m, int event, const std::vector<uint8_t> message)
    {
        MidiEvent e;
        m.GetMidiInputEvent(&e, event);
        AlsaAssert(e.size == message.size());
        for (size_t i = 0; i < message.size(); ++i)
        {
            AlsaAssert(message[i] == e.buffer[i]);
        }
    }
#endif

    void AlsaDriverImpl::TestFormatEncodeDecode(snd_pcm_format_t captureFormat)
    {
        this->alsa_device_name = "Test";
        this->numberOfBuffers = 3;
        this->bufferSize = 64;
        this->user_threshold = this->bufferSize;
        this->sampleRate = 44100;
        this->captureChannels = 2;
        this->playbackChannels = 2;

        PrepareCaptureFunctions(captureFormat);
        PreparePlaybackFunctions(captureFormat);

        // make sure encode decode round-trips with reasonable accuracy.

        for (size_t i = 0; i < bufferSize; ++i)
        {
            for (size_t c = 0; c < captureChannels; ++c)
            {
                // provide a rich set of approximately readable bits in the output.
                float value = 1.0f * i / bufferSize + 1.0f * (i) / (128.0 * 256.0);

                // only 16-bits of precision in data for 16-bit formats
                if (captureFormat != snd_pcm_format_t::SND_PCM_FORMAT_S16_BE && captureFormat != snd_pcm_format_t::SND_PCM_FORMAT_S16_LE)
                {
                    value += 1.0f * (c) / (128.0 * 256.0 * 256.0);
                }
                this->playbackBuffers[c][i] = value;
            }
        }

        (this->*copyOutputFn)(bufferSize);

        assert(captureFrameSize == playbackFrameSize);
        memcpy(this->rawCaptureBuffer.data(), this->rawPlaybackBuffer.data(), captureFrameSize * bufferSize);

        (this->*copyInputFn)(bufferSize);

        for (size_t i = 0; i < bufferSize; ++i)
        {
            for (size_t c = 0; c < captureChannels; ++c)
            {
                float error =
                    this->captureBuffers[c][i] - this->playbackBuffers[c][i];

                assert(std::abs(error) < 4e-5);
            }
        }
    }

    void AlsaFormatEncodeDecodeTest(AudioDriverHost *testDriverHost)
    {
        static snd_pcm_format_t formats[] = {
            snd_pcm_format_t::SND_PCM_FORMAT_S16_LE,
            snd_pcm_format_t::SND_PCM_FORMAT_S16_BE,
            snd_pcm_format_t::SND_PCM_FORMAT_S32_LE,
            snd_pcm_format_t::SND_PCM_FORMAT_S32_BE,
            snd_pcm_format_t::SND_PCM_FORMAT_S24_3BE,
            snd_pcm_format_t::SND_PCM_FORMAT_S24_3LE,
            snd_pcm_format_t::SND_PCM_FORMAT_FLOAT_BE,
            snd_pcm_format_t::SND_PCM_FORMAT_FLOAT_LE,
        };

        for (auto format : formats)
        {
            // Check audio encode/decode.
            std::unique_ptr<AlsaDriverImpl> alsaDriver{
                (AlsaDriverImpl *)new AlsaDriverImpl(testDriverHost)};

            alsaDriver->TestFormatEncodeDecode(format);
        }
    }
    void MidiDecoderTest()
    {
#ifdef JUNK
        AlsaDriverImpl::AlsaMidiDeviceImpl midiState;

        MidiEvent event;

        // Running status decoding.
        {
            static uint8_t m0[] = {0x80, 0x1, 0x2, 0x3, 0x4, 0x5};
            midiState.NextEventBuffer();
            midiState.ProcessInputBuffer(m0, sizeof(m0));
            AlsaAssert(midiState.GetMidiInputEventCount() == 2);
            AlsaAssert(midiState.GetMidiInputEvent(&event, 0));

            ExpectEvent(midiState, 0, {0x80, 0x1, 0x2});
            ExpectEvent(midiState, 1, {0x80, 0x3, 0x4});

            static uint8_t m1[] = {0x06, 0xC0, 0x1, 0x2};
            midiState.NextEventBuffer();
            midiState.ProcessInputBuffer(m1, sizeof(m1));
            AlsaAssert(midiState.GetMidiInputEventCount() == 3);
            ExpectEvent(midiState, 0, {0x80, 0x05, 0x06});
            ExpectEvent(midiState, 1, {0xC0, 0x1});
            ExpectEvent(midiState, 2, {0xC0, 0x2});
        }

        // SYSEX.
        {
            static uint8_t m0[] = {0xF0, 0x76, 0xF7, 0xA};
            midiState.NextEventBuffer();
            midiState.ProcessInputBuffer(m0, 4);
            AlsaAssert(midiState.GetMidiInputEventCount() == 2);
            AlsaAssert(midiState.GetMidiInputEvent(&event, 0));
            AlsaAssert(event.size == 2);
            AlsaAssert(event.buffer[0] == 0xF0);
            AlsaAssert(event.buffer[1] == 0x76);
        }

        // SPLIT SYSEX
        {
            static uint8_t m0[] = {0xF0, 0x76, 0x3B};
            midiState.NextEventBuffer();
            midiState.ProcessInputBuffer(m0, sizeof(m0));
            AlsaAssert(midiState.GetMidiInputEventCount() == 0);
            static uint8_t m1[] = {0x77, 0xF7};
            midiState.NextEventBuffer();
            midiState.ProcessInputBuffer(m1, sizeof(m1));
            AlsaAssert(midiState.GetMidiInputEventCount() == 2);

            AlsaAssert(midiState.GetMidiInputEvent(&event, 0));
            AlsaAssert(event.size == 0x4);
            AlsaAssert(event.buffer[0] == 0xF0);
            AlsaAssert(event.buffer[1] == 0x76);
            AlsaAssert(event.buffer[2] == 0x3B);
            AlsaAssert(event.buffer[3] == 0x77);
        }
#endif
    }

    void FreeAlsaGlobals()
    {
        snd_config_update_free_global(); // to get a clean Valgrind report.
    }

    void AlsaDriverImpl::DumpBufferTrace(size_t nEntries)
    {
        using namespace std;
        int savedPrecision = cout.precision();
        auto  savedFlags = cout.flags();

        size_t ix = bufferTraceIndex;
        if (ix < nEntries)
        {
            ix = ix + bufferTraces.size()-nEntries;
        } else {
            ix -= nEntries;
        }
        uint64_t t0;
        if (bufferTraceIndex == 0) {
            t0 = bufferTraces[bufferTraces.size()-1].time;
        } else {
            t0 = bufferTraces[bufferTraceIndex-1].time;
        }
        while (ix != bufferTraceIndex)
        {
            auto&bufferTrace = bufferTraces[ix];

            if (bufferTrace.time != 0)
            {
                int64_t dt = (int64_t)bufferTrace.time - (int64_t)t0;


                cout << bufferTrace.code << " " 
                    << fixed << setprecision(3) << dt*0.001
                    << " " << "inAvail: " << bufferTrace.inAvail
                    << " " << "outAvail: " << bufferTrace.outAvail
                    << " " << "buffered: " << bufferTrace.buffered
                    << " " << "total: " << bufferTrace.total
                    << endl;
            }

            ++ix;
            if (ix == bufferTraces.size())
            {
                ix = 0;
            }
        }

        cout.precision(savedPrecision);
        cout.flags(savedFlags);
    }

} // namespace
