// Copyright (c) 2022 Robin Davies
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "pch.h"
#include "PiPedalAlsa.hpp"
#include "alsa/asoundlib.h"
#include "Lv2Log.hpp"
#include <mutex>

using namespace pipedal;

static uint32_t RATES[] = {
    22050, 24000, 44100, 48000, 44100 * 2, 48000 * 2, 44100 * 4, 48000 * 4};

std::mutex alsaMutex;

bool PiPedalAlsaDevices::getCachedDevice(const std::string &name, AlsaDeviceInfo *pResult)
{
    auto it = cachedDevices.find(name);
    if (it != cachedDevices.end())
    {
        *pResult = it->second;
        return true;
    }
    return false;
}
void PiPedalAlsaDevices::cacheDevice(const std::string &name, const AlsaDeviceInfo &deviceInfo)
{
    cachedDevices[name] = deviceInfo;
}

std::vector<AlsaDeviceInfo> PiPedalAlsaDevices::GetAlsaDevices()
{
    std::lock_guard guard{alsaMutex};

    std::vector<AlsaDeviceInfo> result;

    int cardNum = -1; // Start with first card
    int err;

    for (;;)
    {
        if ((err = snd_card_next(&cardNum)) < 0)
        {
            Lv2Log::error("Unexpected error enumerating ALSA devices.");
            break;
        }
        if (cardNum < 0)
            // No more cards
            break;

        {
            std::stringstream ss;
            ss << "hw:" << cardNum;
            std::string cardId = ss.str();

            snd_ctl_t *hDevice = nullptr;

            if ((err = snd_ctl_open(&hDevice, cardId.c_str(), 0)) < 0)
            {
                continue;
            }

            snd_ctl_card_info_t *alsaInfo;
            if (snd_ctl_card_info_malloc(&alsaInfo) != 0)
            {
                Lv2Log::error("Failed to allocate ALSA card info");
                snd_ctl_close(hDevice);
                continue;
            }

            err = snd_ctl_card_info(hDevice, alsaInfo);
            if (err == 0)
            {
                AlsaDeviceInfo info;
                info.cardId_ = cardNum;
                info.id_ = std::string("hw:") + snd_ctl_card_info_get_id(alsaInfo);
                const char *driver = snd_ctl_card_info_get_driver(alsaInfo);

                info.name_ = snd_ctl_card_info_get_name(alsaInfo);
                info.longName_ = snd_ctl_card_info_get_longname(alsaInfo);

                snd_pcm_t *hDevice = nullptr;

                // must support capture AND playback
                err = snd_pcm_open(&hDevice, cardId.c_str(), SND_PCM_STREAM_CAPTURE, 0);
                if (err == 0)
                {
                    snd_pcm_close(hDevice);
                }
                if (err == 0)
                {
                    err = snd_pcm_open(&hDevice, cardId.c_str(), SND_PCM_STREAM_PLAYBACK, 0);
                }
                if (err == 0)
                {
                    snd_pcm_hw_params_t *params = nullptr;
                    err = snd_pcm_hw_params_malloc(&params);
                    if (err == 0)
                    {
                        err = snd_pcm_hw_params_any(hDevice, params);
                        if (err == 0)
                        {
                            unsigned int minRate = 0, maxRate = 0;
                            snd_pcm_uframes_t minBufferSize, maxBufferSize;
                            int dir;
                            err = snd_pcm_hw_params_get_rate_min(params, &minRate, &dir);
                            if (err == 0)
                            {
                                err = snd_pcm_hw_params_get_rate_max(params, &maxRate, &dir);
                            }
                            if (err == 0)
                            {
                                err = snd_pcm_hw_params_get_buffer_size_min(params, &minBufferSize);
                            }
                            if (err == 0)
                            {
                                err = snd_pcm_hw_params_get_buffer_size_max(params, &maxBufferSize);
                            }
                            if (err == 0)
                            {
                                for (size_t i = 0; i < sizeof(RATES) / sizeof(RATES[0]); ++i)
                                {
                                    uint32_t rate = RATES[i];
                                    if (rate >= minRate && rate <= maxRate)
                                    {
                                        info.sampleRates_.push_back(rate);
                                    }
                                }
                                info.minBufferSize_ = (uint32_t)minBufferSize;
                                info.maxBufferSize_ = (uint32_t)maxBufferSize;
                                cacheDevice(info.name_, info);
                                result.push_back(info);
                            }
                        }
                    }
                    if (params != nullptr)
                        snd_pcm_hw_params_free(params);
                    snd_pcm_close(hDevice);
                }
                else
                {
                    if (getCachedDevice(info.name_, &info))
                    {
                        result.push_back(info);
                    }
                }
            }
            snd_ctl_card_info_free(alsaInfo);
            snd_ctl_close(hDevice);
        }
    }
    snd_config_update_free_global();
    return result;
}

JSON_MAP_BEGIN(AlsaDeviceInfo)
JSON_MAP_REFERENCE(AlsaDeviceInfo, cardId)
JSON_MAP_REFERENCE(AlsaDeviceInfo, id)
JSON_MAP_REFERENCE(AlsaDeviceInfo, name)
JSON_MAP_REFERENCE(AlsaDeviceInfo, longName)
JSON_MAP_REFERENCE(AlsaDeviceInfo, sampleRates)
JSON_MAP_REFERENCE(AlsaDeviceInfo, minBufferSize)
JSON_MAP_REFERENCE(AlsaDeviceInfo, maxBufferSize)
JSON_MAP_END()
