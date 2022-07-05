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
#include <bit>
#include <memory>
#include "ss.hpp"
#include "AlsaDriver.hpp"
#include "JackServerSettings.hpp"
#include <thread>

#include "CpuUse.hpp"

#include <alsa/asoundlib.h>

#include "Lv2Log.hpp"
#include <limits>

#undef ALSADRIVER_CONFIG_DBG

#ifdef ALSADRIVER_CONFIG_DBG
#include <stdio.h>
#endif

using namespace pipedal;

namespace pipedal
{

    struct AudioFormat
    {
        char name[40];
        snd_pcm_format_t pcm_format;
    };
    [[noreturn]] static void AlsaError(const std::string &message)
    {
        throw PiPedalStateException(message);
    }

    static bool SetPreferredAlsaFormat(
        const char *streamType,
        snd_pcm_t *handle,
        snd_pcm_hw_params_t *hwParams,
        AudioFormat *formats,
        size_t nItems)
    {
        for (size_t i = 0; i < nItems; ++i)
        {
            int err = snd_pcm_hw_params_set_format(handle, hwParams, formats[i].pcm_format);
            if (err == 0)
            {
                return true;
            }
        }
        return false;
    }

    static void SetPreferredAlsaFormat(
        const std::string &alsa_device_name,
        const char *streamType,
        snd_pcm_t *handle,
        snd_pcm_hw_params_t *hwParams)
    {
        int err;

        static AudioFormat leFormats[]{
            {"32-bit float little-endian", SND_PCM_FORMAT_FLOAT_LE},
            {"24-bit little-endian in 3bytes format", SND_PCM_FORMAT_S24_3LE},
            {"32-bit integer little-endian", SND_PCM_FORMAT_S32_LE},
            {"24-bit little-endian", SND_PCM_FORMAT_S24_LE},
            {"16-bit little-endian", SND_PCM_FORMAT_S16_LE},

        };
        static AudioFormat beFormats[]{
            {"32-bit float big-endian", SND_PCM_FORMAT_FLOAT_BE},
            {"24-bit big-endian in 3bytes format", SND_PCM_FORMAT_S24_3BE},
            {"32-bit integer big-endian", SND_PCM_FORMAT_S32_BE},
            {"24-bit big-endian", SND_PCM_FORMAT_S24_BE},
            {"16-bit big-endian", SND_PCM_FORMAT_S16_BE},
        };

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
        pipedal::CpuUse cpuUse;

#ifdef ALSADRIVER_CONFIG_DBG
        snd_output_t *snd_output = nullptr;
        snd_pcm_status_t *snd_status = nullptr;

#endif
        uint32_t sampleRate = 0;

        uint32_t bufferSize;
        uint32_t numberOfBuffers;

        int playbackChannels = 0;
        int captureChannels = 0;

        uint32_t user_threshold = 0;
        bool soft_mode = false;

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

        uint8_t *rawCaptureBuffer = nullptr;
        uint8_t *rawPlaybackBuffer = nullptr;

        AudioDriverHost *driverHost = nullptr;

    public:
        AlsaDriverImpl(AudioDriverHost *driverHost)
            : driverHost(driverHost)
        {
#ifdef ALSADRIVER_CONFIG_DBG
            snd_output_stdio_attach(&snd_output, stdout, 0);
            snd_pcm_status_malloc(&snd_status);
#endif
        }
        virtual ~AlsaDriverImpl()
        {
            Close();
#ifdef ALSADRIVER_CONFIG_DBG
            snd_output_close(snd_output);
            snd_pcm_status_free(snd_status);
#endif
        }

    private:
        void OnShutdown()
        {
            Lv2Log::info("Jack Audio Server has shut down.");
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

        unsigned int periods = 0;

        snd_pcm_hw_params_t *captureHwParams;
        snd_pcm_sw_params_t *captureSwParams;
        snd_pcm_hw_params_t *playbackHwParams;
        snd_pcm_sw_params_t *playbackSwParams;

        bool capture_and_playback_not_synced = false;

        std::mutex terminateSync;

        bool terminateAudio_ = false;

        void terminateAudio(bool terminate)
        {
            std::lock_guard lock{terminateSync};
            this->terminateAudio_ = terminate;
        }

        bool terminateAudio()
        {
            std::lock_guard lock{terminateSync};
            return this->terminateAudio_;
        }

    private:
        void AlsaCleanup()
        {

            if (captureHandle)
            {
                snd_pcm_close(captureHandle);
                captureHandle = nullptr;
            }
            if (playbackHandle)
            {
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
            unsigned int *periods)
        {
            int err;
            snd_pcm_uframes_t stop_th;

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
                unsigned int channels_max;
                err = snd_pcm_hw_params_get_channels_max(hwParams,
                                                         &channels_max);
                *channels = channels_max;

                if (*channels > 1024)
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

            if ((err = snd_pcm_hw_params_set_period_size_near(handle, hwParams,
                                                              &effectivePeriodSize,
                                                              0)) < 0)
            {
                AlsaError(SS("Can't set period size to " << this->bufferSize << " (" << alsa_device_name << "/" << streamType << ")"));
            }
            this->bufferSize = effectivePeriodSize;

            *periods = this->numberOfBuffers;
            snd_pcm_hw_params_set_periods_min(handle, hwParams, periods, NULL);
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

            if (handle == this->captureHandle)
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

            stop_th = *periods * this->bufferSize;
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

            if (handle == this->playbackHandle)
                err = snd_pcm_sw_params_set_avail_min(
                    handle, swParams,
                    this->bufferSize * (*periods - this->numberOfBuffers + 1));
            else
                err = snd_pcm_sw_params_set_avail_min(
                    handle, swParams, this->bufferSize);

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
                AlsaConfigureStream(
                    this->alsa_device_name,
                    "capture",
                    captureHandle,
                    captureHwParams,
                    captureSwParams,
                    &captureChannels,
                    &this->periods);
            }
            if (this->playbackHandle)
            {
                AlsaConfigureStream(
                    this->alsa_device_name,
                    "playback",
                    playbackHandle,
                    playbackHwParams,
                    playbackSwParams,
                    &playbackChannels,
                    &this->periods);
            }

#ifdef ALSADRIVER_CONFIG_DBG
            snd_pcm_dump(captureHandle, snd_output);
            snd_pcm_dump(playbackHandle, snd_output);
#endif
        }
        void CopyCaptureFloatLe(size_t frames)
        {
            float *p = (float *)rawCaptureBuffer;

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

        void CopyCaptureS32Le(size_t frames)
        {
            int32_t *p = (int32_t *)rawCaptureBuffer;

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
        void CopyPlaybackS32Le(size_t frames)
        {
            int32_t *p = (int32_t *)rawPlaybackBuffer;

            std::vector<float *> &buffers = this->playbackBuffers;
            int channels = this->playbackChannels;
            constexpr float scale = std::numeric_limits<int32_t>::max();
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    float v = buffers[channel][frame];
                    if (v > 1.0f)
                        v = 1.0f;
                    if (v < -1.0f)
                        v = -1.0f;
                    *p++ = (int32_t)(scale * v);
                }
            }
        }
        void CopyPlaybackFloatLe(size_t frames)
        {
            float *p = (float *)rawPlaybackBuffer;

            std::vector<float *> &buffers = this->playbackBuffers;
            int channels = this->captureChannels;
            for (size_t frame = 0; frame < frames; ++frame)
            {
                for (int channel = 0; channel < channels; ++channel)
                {
                    float v = buffers[channel][frame];
                    *p++ = v;
                }
            }
        }
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

            OpenMidi(jackServerSettings, channelSelection);
            OpenAudio(jackServerSettings, channelSelection);
        }

        void OpenAudio(const JackServerSettings &jackServerSettings, const JackChannelSelection &channelSelection)
        {
            int err;

            alsa_device_name = jackServerSettings.GetAlsaInputDevice();

            this->numberOfBuffers = jackServerSettings.GetNumberOfBuffers();
            this->bufferSize = jackServerSettings.GetBufferSize();
            this->user_threshold = jackServerSettings.GetBufferSize();

            try
            {

                err = snd_pcm_open(&playbackHandle, alsa_device_name.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
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
                if (this->playbackHandle)
                {
                    snd_pcm_nonblock(playbackHandle, 0);
                }

                err = snd_pcm_open(&captureHandle, alsa_device_name.c_str(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);

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

                switch (captureFormat)
                {
                case SND_PCM_FORMAT_FLOAT_LE:
                    captureSampleSize = 4;
                    copyInputFn = &AlsaDriverImpl::CopyCaptureFloatLe;
                    break;
                case SND_PCM_FORMAT_S24_3LE:
                    captureSampleSize = 3;
                    break;
                case SND_PCM_FORMAT_S32_LE:
                    captureSampleSize = 4;
                    copyInputFn = &AlsaDriverImpl::CopyCaptureS32Le;
                    break;
                case SND_PCM_FORMAT_S24_LE:
                    captureSampleSize = 4;
                    break;
                case SND_PCM_FORMAT_S16_LE:
                    captureSampleSize = 2;
                    break;
                case SND_PCM_FORMAT_FLOAT_BE:
                    captureSampleSize = 4;
                    break;
                case SND_PCM_FORMAT_S24_3BE:
                    captureSampleSize = 3;
                    break;
                case SND_PCM_FORMAT_S32_BE:
                    captureSampleSize = 4;
                    break;
                case SND_PCM_FORMAT_S24_BE:
                    captureSampleSize = 4;
                    break;
                case SND_PCM_FORMAT_S16_BE:
                    captureSampleSize = 2;
                    break;
                default:
                    throw PiPedalStateException("Invalid format.");
                }
                captureFrameSize = captureSampleSize * captureChannels;
                rawCaptureBuffer = new uint8_t[captureFrameSize * bufferSize];
                memset(rawCaptureBuffer, 0, captureFrameSize * bufferSize);

                AllocateBuffers(captureBuffers, captureChannels);

                snd_pcm_format_t playbackFormat;
                snd_pcm_hw_params_get_format(playbackHwParams, &playbackFormat);

                switch (playbackFormat)
                {
                case SND_PCM_FORMAT_FLOAT_LE:
                    playbackSampleSize = 4;
                    copyOutputFn = &AlsaDriverImpl::CopyPlaybackFloatLe;
                    break;
                case SND_PCM_FORMAT_S24_3LE:
                    playbackSampleSize = 3;
                    break;
                case SND_PCM_FORMAT_S32_LE:
                    copyOutputFn = &AlsaDriverImpl::CopyPlaybackS32Le;
                    playbackSampleSize = 4;
                    break;
                case SND_PCM_FORMAT_S24_LE:
                    playbackSampleSize = 4;
                    break;
                case SND_PCM_FORMAT_S16_LE:
                    playbackSampleSize = 2;
                    break;
                case SND_PCM_FORMAT_FLOAT_BE:
                    playbackSampleSize = 4;
                    break;
                case SND_PCM_FORMAT_S24_3BE:
                    playbackSampleSize = 3;
                    break;
                case SND_PCM_FORMAT_S32_BE:
                    playbackSampleSize = 4;
                    break;
                case SND_PCM_FORMAT_S24_BE:
                    playbackSampleSize = 4;
                    break;
                case SND_PCM_FORMAT_S16_BE:
                    playbackSampleSize = 2;
                    break;
                default:
                    throw PiPedalStateException("Invalid format.");
                }
                playbackFrameSize = playbackSampleSize * playbackChannels;
                rawPlaybackBuffer = new uint8_t[playbackFrameSize * bufferSize];
                memset(rawPlaybackBuffer, 0, playbackFrameSize * bufferSize);

                AllocateBuffers(playbackBuffers, playbackChannels);
            }
            catch (const std::exception &e)
            {
                AlsaCleanup();
                throw;
            }
        }

        void FillOutputBuffer()
        {
            memset(rawPlaybackBuffer, 0, playbackFrameSize * bufferSize);
            while (true)
            {
                auto avail = snd_pcm_avail(this->playbackHandle);
                if (avail < 0)
                {
                    int err = snd_pcm_prepare(playbackHandle);
                    if (err < 0)
                    {
                        throw PiPedalStateException(SS("Audio playback failed. " << snd_strerror(err)));
                    }
                    continue;
                }
                if (avail == 0)
                    break;
                if (avail > this->bufferSize)
                    avail = this->bufferSize;

                ssize_t err = WriteBuffer(playbackHandle, rawPlaybackBuffer, avail);
                if (err < 0)
                {
                    throw PiPedalStateException(SS("Audio playback failed. " << snd_strerror(err)));
                }
            }
        }
        void XrunRecoverOutputOverrun(snd_pcm_t *handle, int err)
        {
            if (err == -EPIPE)
            {
                err = snd_pcm_prepare(handle);
                if (err < 0)
                {
                    throw PiPedalStateException(SS("Can't recover from ALSA underrun. (" << snd_strerror(err) << ")"));
                }
            }
            FillOutputBuffer();
        }

        void DumpStatus(snd_pcm_t *handle)
        {
#ifdef ALSADRIVER_CONFIG_DBG
            snd_pcm_status(handle, snd_status);
            snd_pcm_status_dump(snd_status, snd_output);
#endif
        }

        void XrunRecoverInputUnderrun(snd_pcm_t *handle, int err)
        {
            if (err == -EPIPE)
            { /* underrun */

                this->driverHost->OnUnderrun();

                // DumpStatus(handle);
                err = snd_pcm_drop(handle);
                if (err < 0)
                {
                    throw PiPedalStateException(SS("Can't recover from ALSA underrun. (" << snd_strerror(err) << ")"));
                }
                err = snd_pcm_prepare(handle);
                if (err < 0)
                {
                    throw PiPedalStateException(SS("Can't recover from ALSA underrun. (" << snd_strerror(err) << ")"));
                }
                FillOutputBuffer();

                err = snd_pcm_start(handle);
                // if (err < 0)
                // {
                //     throw PiPedalStateException(SS("Can't recover from ALSA underrun. (" << snd_strerror(err) << ")"));
                // }
                return;
            }
            else if (err == -ESTRPIPE)
            {
                audioRunning = false;
                while ((err = snd_pcm_resume(handle)) == -EAGAIN)
                {
                    sleep(1);
                }
                if (err < 0)
                {
                    err = snd_pcm_prepare(handle);
                    if (err < 0)
                    {
                        throw PiPedalStateException(SS("Can't recover from ALSA suspend. (" << snd_strerror(err) << ")"));
                    }
                }
                return;
            }
            throw PiPedalStateException(SS("ALSA error:" << snd_strerror(err)));
        }

        std::jthread *audioThread;
        bool audioRunning;

        bool block = false;

        snd_pcm_sframes_t ReadBuffer(snd_pcm_t *handle, uint8_t *buffer, snd_pcm_uframes_t frames)
        {
            // transcode to jack format.
            // expand running status if neccessary.
            // deal with regular and sysex messages split across
            // buffer boundaries.
            snd_pcm_sframes_t framesRead;

            auto state = snd_pcm_state(handle);
            auto frame_bytes = this->captureFrameSize;
            do
            {
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
                    snd_pcm_wait(captureHandle, 1);
                }
            } while (frames > 0);
            return framesRead;
        }

        long WriteBuffer(snd_pcm_t *handle, uint8_t *buf, size_t frames)
        {
            long framesRead;
            auto frame_bytes = this->captureFrameSize;

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
                param.sched_priority = 79;

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
                xxx; // TODO!
#endif

                bool ok = true;

                auto playbackState = snd_pcm_state(playbackHandle);

                FillOutputBuffer();

                int err;
                if ((err = snd_pcm_start(captureHandle)) < 0)
                {
                    throw PiPedalStateException("Unable to start ALSA capture.");
                }

                cpuUse.SetStartTime(cpuUse.Now());
                while (true)
                {
                    cpuUse.UpdateCpuUse();

                    if (terminateAudio())
                    {
                        break;
                    }

                    // snd_pcm_wait(captureHandle, 1);

                    ssize_t framesRead;
                    if ((framesRead = ReadBuffer(captureHandle, this->rawCaptureBuffer, bufferSize)) < 0)
                    {
                        this->driverHost->OnUnderrun();
                        auto state = snd_pcm_state(playbackHandle);
                        XrunRecoverInputUnderrun(captureHandle, framesRead);
                        continue;
                    }
                    else
                    {
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

                        ssize_t err = WriteBuffer(playbackHandle, rawPlaybackBuffer, framesRead);

                        if (err < 0)
                        {
                            this->driverHost->OnUnderrun();
                            XrunRecoverOutputOverrun(playbackHandle, err);
                        }
                        cpuUse.AddSample(ProfileCategory::Write);
                    }
                }
            }
            catch (const std::exception &e)
            {
                Lv2Log::error(e.what());
                Lv2Log::error("ALSA audio thread terminated abnormally.");
            }
            this->driverHost->OnAudioStopped();
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
                    throw PiPedalArgumentException("Invalid input port.");
                }
                this->activeCaptureBuffers[ix++] = this->captureBuffers[sourceIndex];
            }

            this->activePlaybackBuffers.resize(channelSelection.GetOutputAudioPorts().size());

            ix = 0;
            for (auto &x : channelSelection.GetOutputAudioPorts())
            {
                int sourceIndex = IndexFromPortName(x);
                if (sourceIndex >= playbackBuffers.size())
                {
                    throw PiPedalArgumentException("Invalid output port.");
                }
                this->activePlaybackBuffers[ix++] = this->playbackBuffers[sourceIndex];
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

                int err = snd_rawmidi_open(&hIn, nullptr, device.name_.c_str(), SND_RAWMIDI_NONBLOCK);
                if (err < 0)
                {
                    throw PiPedalStateException(SS("Can't open midi device " << deviceName << ". (" << snd_strerror(err)));
                }

                err = snd_rawmidi_params_malloc(&hInParams);
                checkError(err, "snd_rawmidi_params_malloc failed.");

                err = snd_rawmidi_params_set_buffer_size(hIn, hInParams, 2048);
                checkError(err, "snd_rawmidi_params_set_buffer_size failed.");

                err = snd_rawmidi_params_set_no_active_sensing(hIn, hInParams, 1);
                checkError(err, "snd_rawmidi_params_set_no_active_sensing failed.");
            }
            void Close()
            {
                if (hIn)
                {
                    snd_rawmidi_close(hIn);
                    hIn = nullptr;
                }
                if (hInParams)
                {
                    snd_rawmidi_params_free(hInParams);
                    hInParams = 0;
                }
            }

            int GetDataLength(uint8_t cc)
            {
                static int sDataLength[] = {0, 0, 0, 0, 0, 0, 0, 0, 2, 2, 2, 2, 1, 1, 1, -1};
                return sDataLength[cc >> 4];
            }

            size_t GetMidiInputEventCount()
            {
                return eventCount;
            }

            void NextEventBuffer()
            {
                // xxx preserve unflushed sysex data.
                if (sysexStartIndex != -1)
                {
                    int end = bufferCount;
                    bufferCount = 0;
                    eventCount = 0;
                    for (int i = sysexStartIndex; i < end; ++i)
                    {
                        buffer[bufferCount++] = buffer[i];
                    }
                    sysexStartIndex = 0;
                }
                else
                {
                    bufferCount = 0;
                    eventCount = 0;
                }
            }
            bool GetMidiInputEvent(MidiEvent *event, size_t nFrame)
            {
                if (nFrame >= eventCount)
                    return false;
                *event = this->events[nFrame];
                return true;
            }

            void MidiPut(uint8_t cc, uint8_t d0, uint8_t d1)
            {
                if (cc == 0)
                    return;

                // check for overrun.
                if (bufferCount + 1 + dataLength >= sizeof(buffer))
                {
                    bufferCount = sizeof(buffer);
                    return;
                }
                if (eventCount >= MAX_MIDI_EVENT)
                {
                    return;
                }

                auto *event = &(this->events[eventCount++]);

                event->time = 0;
                event->buffer = &buffer[bufferCount];
                event->size = dataLength + 1;

                buffer[bufferCount++] = cc;
                if (dataLength >= 1)
                {
                    buffer[bufferCount++] = d0;
                    if (dataLength >= 2)
                    {
                        buffer[bufferCount++] = d1;
                    }
                }
            }

            void FillBuffer()
            {
                while (true)
                {
                    ssize_t nRead = snd_rawmidi_read(hIn, readBuffer, sizeof(readBuffer));
                    if (nRead == -EAGAIN)
                        return;
                    if (nRead < 0)
                    {
                        checkError(nRead, SS(this->deviceName << "MIDI event read failed. (" << snd_strerror(nRead)).c_str());
                    }
                    WriteBuffer(readBuffer, nRead); // expose write to test code.
                }
            }

            void FlushSysex()
            {
                if (sysexStartIndex != -1)
                {
                    if (this->eventCount != MAX_MIDI_EVENT)
                    {
                        auto *event = &(events[eventCount++]);
                        event->size = this->bufferCount - sysexStartIndex;
                        event->buffer = &(this->buffer[this->sysexStartIndex]);
                        event->time = 0;
                    }
                    sysexStartIndex = -1;
                }
            }

            int GetSystemCommonLength(uint8_t cc)
            {
                static int sizes[] = {-1, 1, 2, 1, -1, -1, 0, 0};
                return sizes[(cc >> 4) & 0x07];
            }
            void WriteBuffer(uint8_t *readBuffer, size_t nRead)
            {
                for (ssize_t i = 0; i < nRead; ++i)
                {
                    uint8_t v = readBuffer[i];

                    if (v >= 0x80)
                    {
                        if (v >= 0xF0)
                        {
                            if (v == 0xF0)
                            {
                                if (bufferCount == sizeof(buffer))
                                {
                                    break;
                                }
                                sysexStartIndex = bufferCount;

                                buffer[bufferCount++] = 0xF0;

                                runningStatus = 0; // discard subsequent data.
                                dataLength = -2;   // indefinitely.
                                dataIndex = -1;
                            }
                            else if (v >= 0xF8)
                            {
                                // don't overwrite running status.
                                // don't break sysexes on a running status message.
                                // LV2 standard is ambiguous how realtime messages are handled,  so just discard them.
                                continue;
                            }
                            else
                            {
                                FlushSysex();
                                int length = GetSystemCommonLength(v);
                                if (length == -1)
                                    break; // ignore illegal messages.
                                runningStatus = v;
                                dataLength = length;
                                dataIndex = 0;
                            }
                        }
                        else
                        {
                            FlushSysex();
                            int dataLength = GetDataLength(v);
                            runningStatus = v;
                            if (dataLength == -1)
                            {
                                this->dataLength = dataLength;
                                dataIndex = -1;
                            }
                            else
                            {
                                this->dataLength = dataLength;
                                dataIndex = 0;
                            }
                        }
                    }
                    else
                    {
                        if (sysexStartIndex != -1)
                        {
                            if (bufferCount != sizeof(buffer))
                            {
                                buffer[bufferCount++] = v;
                            }
                        }
                        else
                        {
                            switch (dataIndex)
                            {
                            default:
                                // discard.
                                break;
                            case 0:
                                data0 = v;
                                dataIndex = 1;
                                break;
                            case 1:
                                data1 = v;
                                dataIndex = 2;
                                break;
                            }
                        }
                    }
                    if (dataIndex == dataLength)
                    {
                        MidiPut(runningStatus, data0, data1);
                        dataIndex = 0;
                    }
                }
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
            for (size_t i = 0; i < this->midiStates.size(); ++i)
            {
                auto *state = midiStates[i];
                state->NextEventBuffer();
                state->FillBuffer();
            }
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
        }
        virtual void Close()
        {
            if (!open)
            {
                return;
            }
            open = false;
            Deactivate();
            AlsaCleanup();
            DeleteBuffers();
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

        snd_pcm_t *playbackHandle = nullptr;
        snd_pcm_t *captureHandle = nullptr;
        snd_pcm_hw_params_t *playbackHwParams = nullptr;
        snd_pcm_hw_params_t *captureHwParams = nullptr;
        std::string alsaDeviceName = jackServerSettings.GetAlsaInputDevice();
        bool result = false;

        try
        {
            int err;
            for (int retry = 0; retry < 5; ++retry)
            {
                err = snd_pcm_open(&playbackHandle, alsaDeviceName.c_str(), SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK);
                if (err == -EBUSY)
                {
                    sleep(1);
                    continue;
                }
                break;
            }
            if (err < 0)
            {
                throw PiPedalStateException(SS(alsaDeviceName << " not found. "
                                                              << "(" << snd_strerror(err) << ")"));
            }

            err = snd_pcm_open(&captureHandle, alsaDeviceName.c_str(), SND_PCM_STREAM_CAPTURE, SND_PCM_NONBLOCK);
            if (err < 0)
                throw PiPedalStateException(SS(alsaDeviceName << " not found."));

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
            err = snd_pcm_hw_params_get_channels_max(captureHwParams, &captureChannels);
            if (err < 0)
            {
                throw PiPedalLogicException("No input channels.");
            }

            inputAudioPorts.clear();
            for (unsigned int i = 0; i < playbackChannels; ++i)
            {
                inputAudioPorts.push_back(SS("system::playback_" << i));
            }

            outputAudioPorts.clear();
            for (unsigned int i = 0; i < captureChannels; ++i)
            {
                outputAudioPorts.push_back(SS("system::capture_" << i));
            }

            result = true;
        }
        catch (const std::exception &e)
        {
            Lv2Log::warning(SS("Unable to read ALSA configuration for " << alsaDeviceName << ". " << e.what() << "."));
            result = false;
            throw;
        }
        if (playbackHwParams)
            snd_pcm_hw_params_free(playbackHwParams);
        if (captureHwParams)
            snd_pcm_hw_params_free(captureHwParams);

        if (playbackHandle)
        {
            snd_pcm_close(playbackHandle);
        }
        if (captureHandle)
            snd_pcm_close(captureHandle);
        return result;
    }

    static void AlsaAssert(bool value)
    {
        if (!value)
            throw PiPedalStateException("Assert failed.");
    }

    static void ExpectEvent(AlsaDriverImpl::MidiState &m, int event, const std::vector<uint8_t> message)
    {
        MidiEvent e;
        m.GetMidiInputEvent(&e,event);
        AlsaAssert(e.size == message.size());
        for (size_t i = 0; i < message.size(); ++i)
        {
            AlsaAssert(message[i] == e.buffer[i]);
        }
    }

    void MidiDecoderTest()
    {

        AlsaDriverImpl::MidiState midiState;

        MidiEvent event;

        // Running status decoding.
        {
            static uint8_t m0[] = {0x80, 0x1, 0x2, 0x3,0x4,0x5};
            midiState.NextEventBuffer();
            midiState.WriteBuffer(m0, sizeof(m0));
            AlsaAssert(midiState.GetMidiInputEventCount() == 2);
            AlsaAssert(midiState.GetMidiInputEvent(&event, 0));

            ExpectEvent(midiState,0, {0x80,0x1,0x2});
            ExpectEvent(midiState,1, {0x80,0x3,0x4});

            static uint8_t m1[] = {0x06,0xC0,0x1,0x2};
            midiState.NextEventBuffer();
            midiState.WriteBuffer(m1,sizeof(m1));
            AlsaAssert(midiState.GetMidiInputEventCount() == 3);
            ExpectEvent(midiState,0,{0x80,0x05,0x06});
            ExpectEvent(midiState,1,{0xC0,0x1});
            ExpectEvent(midiState,2,{0xC0,0x2});
        }


        // SYSEX.
        {
            static uint8_t m0[] = {0xF0, 0x76, 0xF7, 0xA};
            midiState.NextEventBuffer();
            midiState.WriteBuffer(m0, 4);
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
            midiState.WriteBuffer(m0, sizeof(m0));
            AlsaAssert(midiState.GetMidiInputEventCount() == 0);
            static uint8_t m1[] = {0x77, 0xF7};
            midiState.NextEventBuffer();
            midiState.WriteBuffer(m1, sizeof(m1));
            AlsaAssert(midiState.GetMidiInputEventCount() == 2);

            AlsaAssert(midiState.GetMidiInputEvent(&event, 0));
            AlsaAssert(event.size == 0x4);
            AlsaAssert(event.buffer[0] == 0xF0);
            AlsaAssert(event.buffer[1] == 0x76);
            AlsaAssert(event.buffer[2] == 0x3B);
            AlsaAssert(event.buffer[3] == 0x77);
        }
    }
} // namespace
