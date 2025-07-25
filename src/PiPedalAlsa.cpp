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
#include <algorithm>

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

                snd_pcm_t *captureDevice = nullptr;
                snd_pcm_t *playbackDevice = nullptr;
                bool captureOk = snd_pcm_open(&captureDevice, cardId.c_str(), SND_PCM_STREAM_CAPTURE, 0) == 0;
                bool playbackOk = snd_pcm_open(&playbackDevice, cardId.c_str(), SND_PCM_STREAM_PLAYBACK, 0) == 0;
                info.supportsCapture_ = captureOk;
                info.supportsPlayback_ = playbackOk;

                if (captureOk || playbackOk)
                {
                    snd_pcm_t *hDevice = captureOk ? captureDevice : playbackDevice;
                    snd_pcm_hw_params_t *params = nullptr;
                    err = snd_pcm_hw_params_malloc(&params);
                    if (err == 0)
                    {
                        err = snd_pcm_hw_params_any(hDevice, params);
                        if (err == 0)
                        {
                            unsigned int minRate = 0, maxRate = 0;
                            snd_pcm_uframes_t minBufferSize = 0, maxBufferSize = 0;
                            int dir;
                            
                            err = snd_pcm_hw_params_get_rate_min(params, &minRate, &dir);
                            if (err == 0)
                            {
                               int err2 = snd_pcm_hw_params_get_rate_max(params, &maxRate, &dir);
                                if (err2 == 0)
                                {
                                    for (size_t i = 0; i < sizeof(RATES) / sizeof(RATES[0]); ++i)
                                    {
                                        uint32_t rate = RATES[i];
                                        if (rate >= minRate && rate <= maxRate)
                                        {
                                            info.sampleRates_.push_back(rate);
                                        }
                                    }                                   
                            }
                            else
                                {
                                    Lv2Log::warning(SS("Failed to get maximum sample rate for device '" << info.name_ << "'."));
                                }
                            }
                            else
                            {
                                Lv2Log::warning(SS("Failed to get minimum sample rate for device '" << info.name_ << "'."));
                                err = 0; // continue using fallback rate for other parameters
                            }
                            
                            err = snd_pcm_hw_params_get_buffer_size_min(params, &minBufferSize);
                            if (err == 0)
                            {
                                err = snd_pcm_hw_params_get_buffer_size_max(params, &maxBufferSize);
                            }
                            if (err == 0)
                            {
                                if (minBufferSize < 16)
                                {
                                    minBufferSize = 16;
                                }

                                info.minBufferSize_ = (uint32_t)minBufferSize;
                                info.maxBufferSize_ = (uint32_t)maxBufferSize;
                            }
                        }
                    }
                    if (params != nullptr)
                        snd_pcm_hw_params_free(params);
                   if (captureOk)
                        snd_pcm_close(captureDevice);
                    if (playbackOk && playbackDevice != captureDevice)
                        snd_pcm_close(playbackDevice);
                    if (err == 0)
                    {
                        cacheDevice(info.name_, info);
                        result.push_back(info);
                    }
                    else if (getCachedDevice(info.name_, &info))
                    {
                        result.push_back(info);
                    }
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

    auto isFiltered = [](const AlsaDeviceInfo &d) {
        std::string name = d.name_ + " " + d.longName_;
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c){return std::tolower(c);});
        return name.find("hdmi") != std::string::npos || name.find("bcm2835") != std::string::npos;
    };

    std::vector<AlsaDeviceInfo> filtered;
    for (auto &d : result)
    {
        if (!isFiltered(d)) filtered.push_back(d);
    }

    Lv2Log::debug("GetAlsaDevices --");
    for (auto &device : filtered)
    {
        Lv2Log::debug(SS("   " << device.name_ << " " << device.longName_ << " " << device.cardId_));
    }
     return filtered;
}

static void AddMidiCardDevicesToList(snd_ctl_t *ctl, int card, int device, AlsaMidiDeviceInfo::Direction direction, std::vector<AlsaMidiDeviceInfo> *result)
{
    snd_rawmidi_info_t *info = nullptr;
    const char *name = nullptr;
    const char *sub_name = nullptr;
    int subs = 0, subs_in = 0, subs_out = 0;
    int sub = 0;
    int err = 0;
    ;

    snd_rawmidi_info_alloca(&info);
    snd_rawmidi_info_set_device(info, device);

    snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_INPUT);
    err = snd_ctl_rawmidi_info(ctl, info);
    if (err >= 0)
        subs_in = snd_rawmidi_info_get_subdevices_count(info);
    else
        subs_in = 0;

    snd_rawmidi_info_set_stream(info, SND_RAWMIDI_STREAM_OUTPUT);
    err = snd_ctl_rawmidi_info(ctl, info);
    if (err >= 0)
        subs_out = snd_rawmidi_info_get_subdevices_count(info);
    else
        subs_out = 0;

    subs = subs_in > subs_out ? subs_in : subs_out;
    if (!subs)
        return;

    switch (direction)
    {
    case AlsaMidiDeviceInfo::In:
        if (subs_out == 0) // out for the device, in for us.
        {
            return; // no input devices to add
        }
        break;
    case AlsaMidiDeviceInfo::Out: // in for the device, out for us.
        if (subs_in == 0)
        {
            return; // no output devices to add
        }
        break;
    case AlsaMidiDeviceInfo::InOut:
        if (subs_in == 0 || subs_out == 0)
        {
            return; // no input or output devices to add
        }
        break;
    case AlsaMidiDeviceInfo::None:
        return; // no devices to add
    }

    for (sub = 0; sub < subs; ++sub)
    {
        snd_rawmidi_info_set_stream(info, direction == AlsaMidiDeviceInfo::Direction::Out ? SND_RAWMIDI_STREAM_INPUT : SND_RAWMIDI_STREAM_OUTPUT);
        snd_rawmidi_info_set_subdevice(info, sub);
        err = snd_ctl_rawmidi_info(ctl, info);
        if (err < 0)
        {
            throw std::runtime_error(
                SS("snd_ctl_rawmidi_info failed  for card " << card << ", device " << device << ", subdevice " << sub << ": " << snd_strerror(err)));
        }
        name = snd_rawmidi_info_get_name(info);
        sub_name = snd_rawmidi_info_get_subdevice_name(info);
        // get card name.
        
        std::string cardName;
        snd_ctl_card_info_t *card_info = nullptr;
        snd_ctl_card_info_malloc(&card_info);
        if (snd_ctl_card_info(ctl, card_info) == 0) {
            cardName = snd_ctl_card_info_get_name(card_info);
        }
        snd_ctl_card_info_free(card_info);
        if (cardName.length() == 0)
        {
            return;
        }
        
        if (sub == 0 && sub_name[0] == '\0')
        {
            AlsaMidiDeviceInfo info;
            info.name_ = SS("hw:CARD=" << cardName << ",DEV=" << device);
            info.description_ = SS("Virtual MIDI " << card << "-" << device);
            info.card_ = card;
            info.device_ = device;
            info.subdevice_ = 0;
            info.isVirtual_ = true;
            info.subDevices_ = subs;
            if (subs_in > 0 && subs_out > 0)
            {
                info.direction_ = AlsaMidiDeviceInfo::InOut;
            }
            else if (subs_in > 0)
            {
                info.direction_ = AlsaMidiDeviceInfo::In;
            }
            else if (subs_out > 0)
            {
                info.direction_ = AlsaMidiDeviceInfo::Out;
            }
            else
            {
                info.direction_ = AlsaMidiDeviceInfo::None;
            }
            result->push_back(info);
            break;
        }
        else
        {
            AlsaMidiDeviceInfo info;
            info.name_ = SS("hw:CARD=" << cardName << ",DEV=" << device);
            if (sub != 0) {
                info.name_ = SS(info.name_ << "," << sub);
            }
            info.description_ = sub_name;
            info.card_ = card;
            info.device_ = device;
            info.subdevice_ = sub;
            info.isVirtual_ = false;
            info.subDevices_ = 1;
            result->push_back(info);
        }
    }
}
static void AddMidiCardToList(int card, std::vector<AlsaMidiDeviceInfo> *result, AlsaMidiDeviceInfo::Direction direction)
{
    snd_ctl_t *ctl = nullptr;
    char name[32];
    int device;
    int err;

    sprintf(name, "hw:%d", card);
    if ((err = snd_ctl_open(&ctl, name, 0)) < 0)
    {
        throw std::runtime_error(SS("cannot open control for card " << card << ": " << snd_strerror(err)));
    }
    device = -1;
    for (;;)
    {
        if ((err = snd_ctl_rawmidi_next_device(ctl, &device)) < 0)
        {
            snd_ctl_close(ctl);
            throw std::runtime_error(SS("cannot get next rawmidi device for card " << card << ": " << snd_strerror(err)));
        }
        if (device < 0)
            break;
        AddMidiCardDevicesToList(ctl, card, device, direction, result);
    }
    snd_ctl_close(ctl);
}

static std::vector<AlsaMidiDeviceInfo> GetAlsaDevices(const char *devname, const char *direction_)
{
    std::vector<AlsaMidiDeviceInfo> result;

    int card, err;

    AlsaMidiDeviceInfo::Direction direction;
    if (strcmp(direction_, "Input") == 0)
    {
        direction = AlsaMidiDeviceInfo::In;
    }
    else if (strcmp(direction_, "Output") == 0)
    {
        direction = AlsaMidiDeviceInfo::Out;
    }
    else if (strcmp(direction_, "InOut") == 0)
    {
        direction = AlsaMidiDeviceInfo::InOut;
    }
    else
    {
        direction = AlsaMidiDeviceInfo::None;
        return result;
    }

    card = -1;
    if ((err = snd_card_next(&card)) < 0)
    {
        throw std::runtime_error(SS("snd_card_next failed.: " << snd_strerror(err)));
    }
    if (card < 0)
    {
        // no devices!
        return result;
    }
    do
    {
        AddMidiCardToList(card, &result, direction);
        if ((err = snd_card_next(&card)) < 0)
        {
            throw std::runtime_error(SS("snd_card_next failed: " << snd_strerror(err)));
        }
    } while (card >= 0);

    return result;
}
static std::vector<AlsaMidiDeviceInfo> OldGetAlsaDevices(const char *devname, const char *direction)
{
    std::vector<AlsaMidiDeviceInfo> result;
    {

        char **hints;
        int err;
        char **n;
        char *name;
        char *desc;
        char *ioid;

        /* Enumerate sound devices */
        err = snd_device_name_hint(-1, devname, (void ***)&hints);
        if (err != 0)
        {
            return result;
        }

        n = hints;
        while (*n != NULL)
        {

            name = snd_device_name_get_hint(*n, "NAME");
            desc = snd_device_name_get_hint(*n, "DESC");
            ioid = snd_device_name_get_hint(*n, "IOID");

            if (desc != nullptr) // skip virtual device
            {
                if (ioid == nullptr || strcmp(ioid, direction) == 0)
                {
                    result.push_back(AlsaMidiDeviceInfo(name, desc));
                }
            }
            // translate rawmidi device name to device number.

            if (name && strcmp("null", name) != 0)
                free(name);
            if (desc && strcmp("null", desc) != 0)
                free(desc);
            if (ioid && strcmp("null", ioid) != 0)
                free(ioid);
            n++;
        }

        // Free hint buffer too
        snd_device_name_free_hint((void **)hints);
    }
    return result;
}

std::vector<AlsaMidiDeviceInfo> pipedal::LegacyGetAlsaMidiInputDevices()
{
    return GetAlsaDevices("rawmidi", "Input");
}
std::vector<AlsaMidiDeviceInfo> pipedal::LegacyGetAlsaMidiOutputDevices()
{
    return GetAlsaDevices("rawmidi", "Output");
}

AlsaMidiDeviceInfo::AlsaMidiDeviceInfo(const char *name, const char *description)
    : name_(name)
{
    // extract just the display name from description.
    // undocumented but e.g.:   M2, M2\nM2 Raw Midi
    const char *p = description;
    const char *pEnd = p;
    // undocumented but e.g.:   M2, M2\nM2 Raw Midi
    while (pEnd != nullptr && *pEnd != 0 && *pEnd != ',' && *pEnd != '\n')
    {
        ++pEnd;
    }
    if (p != pEnd)
    {
        description_ = std::string(p, pEnd);
    }
    else
    {
        description = name;
    }
}

AlsaMidiDeviceInfo::AlsaMidiDeviceInfo(const char *name, const char *description, int card, int device, int subdevice)
    : AlsaMidiDeviceInfo(name, description)
{
    this->card_ = card;
    this->device_ = device;
    this->subdevice_ = subdevice;
}

JSON_MAP_BEGIN(AlsaDeviceInfo)
JSON_MAP_REFERENCE(AlsaDeviceInfo, cardId)
JSON_MAP_REFERENCE(AlsaDeviceInfo, id)
JSON_MAP_REFERENCE(AlsaDeviceInfo, name)
JSON_MAP_REFERENCE(AlsaDeviceInfo, longName)
JSON_MAP_REFERENCE(AlsaDeviceInfo, sampleRates)
JSON_MAP_REFERENCE(AlsaDeviceInfo, minBufferSize)
JSON_MAP_REFERENCE(AlsaDeviceInfo, maxBufferSize)
JSON_MAP_REFERENCE(AlsaDeviceInfo, supportsCapture)
JSON_MAP_REFERENCE(AlsaDeviceInfo, supportsPlayback)
JSON_MAP_END()

JSON_MAP_BEGIN(AlsaMidiDeviceInfo)
JSON_MAP_REFERENCE(AlsaMidiDeviceInfo, name)
JSON_MAP_REFERENCE(AlsaMidiDeviceInfo, description)
JSON_MAP_END()
