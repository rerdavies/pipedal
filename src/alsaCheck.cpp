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

#include "alsaCheck.hpp"

#include <alsa/asoundlib.h>
#include <iostream>
#include <vector>
#include <string>
#include "ss.hpp"
#include <stdexcept>
#include "Finally.hpp"



void pipedal::check_alsa_channel_configs(const char* device_name) {
    snd_pcm_t* handle;
    int err;
    
    // Open PCM device for playback
    if ((err = snd_pcm_open(&handle, device_name, SND_PCM_STREAM_PLAYBACK, 0)) < 0) {
        throw std::runtime_error(SS("Cannot open audio device " << device_name << ": " << snd_strerror(err)));
        return;
    }

    Finally ff([handle] {
        snd_pcm_close(handle);
    });
    // Get hardware parameters
    snd_pcm_hw_params_t* hw_params;
    snd_pcm_hw_params_alloca(&hw_params);
    
    if ((err = snd_pcm_hw_params_any(handle, hw_params)) < 0) {
        const std::string errorMessage = snd_strerror(err);
        throw std::runtime_error(SS("Cannot initialize hardware parameter structure: " << errorMessage));
    }

    // Get channel count range
    unsigned int min_channels, max_channels;
    if ((err = snd_pcm_hw_params_get_channels_min(hw_params, &min_channels)) < 0) {
        const std::string errorMessage = snd_strerror(err);
        throw std::runtime_error(SS("Cannot get minimum channels: " << errorMessage));
    }
    
    if ((err = snd_pcm_hw_params_get_channels_max(hw_params, &max_channels)) < 0) {
        std::cerr << "Cannot get maximum channels: " << snd_strerror(err) << std::endl;
        return;
    }

    std::cout << "Device: " << device_name << std::endl;
    std::cout << "Channel range: " << min_channels << " to " << max_channels << std::endl;
    std::cout << "\nSupported channel configurations:" << std::endl;

    // Test specific channel counts
    std::vector<unsigned int> common_configs = {1, 2, 4, 6, 8};
    for (unsigned int channels : common_configs) {
        if (channels >= min_channels && channels <= max_channels) {
            snd_pcm_hw_params_t* test_params;
            snd_pcm_hw_params_alloca(&test_params);
            snd_pcm_hw_params_copy(test_params, hw_params);

            if (snd_pcm_hw_params_set_channels(handle, test_params, channels) == 0) {
                std::string config_name;
                switch (channels) {
                    case 1: config_name = "Mono"; break;
                    case 2: config_name = "Stereo"; break;
                    case 4: config_name = "Quadraphonic"; break;
                    case 6: config_name = "5.1 Surround"; break;
                    case 8: config_name = "7.1 Surround"; break;
                    default: config_name = std::to_string(channels) + " channels";
                }
                std::cout << "  " << channels << " channels (" << config_name << ")" << std::endl;
            }
        }
    }

}


static void print_device_info(snd_ctl_t* handle, int card, int device) {
    snd_pcm_info_t* pcminfo;
    snd_pcm_info_alloca(&pcminfo);
    snd_pcm_info_set_device(pcminfo, device);
    snd_pcm_info_set_subdevice(pcminfo, 0);

    // Check playback
    snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_PLAYBACK);
    bool has_playback = snd_ctl_pcm_info(handle, pcminfo) >= 0;

    // Check capture
    snd_pcm_info_set_stream(pcminfo, SND_PCM_STREAM_CAPTURE);
    bool has_capture = snd_ctl_pcm_info(handle, pcminfo) >= 0;

    if (has_playback || has_capture) {
        char name[32];
        snprintf(name, sizeof(name), "hw:%d,%d", card, device);

        // Get device name
        snd_pcm_t* pcm;
        std::string device_name = "Unknown";
        if (snd_pcm_open(&pcm, name, SND_PCM_STREAM_PLAYBACK, SND_PCM_NONBLOCK) >= 0) {
            device_name = snd_pcm_name(pcm);
            snd_pcm_close(pcm);
        }

        // Print basic info
        std::cout << "\n  Subdevice: " << name << " - " << device_name << std::endl;
        std::cout << "  ID: " << snd_pcm_info_get_id(pcminfo) << std::endl;
        std::cout << "  Capabilities:";
        if (has_playback) std::cout << " PLAYBACK";
        if (has_capture) std::cout << " CAPTURE";
        std::cout << std::endl;

        // Get hardware parameters for more detailed info
        if (has_playback) {
            snd_pcm_t* pcm_handle;
            if (snd_pcm_open(&pcm_handle, name, SND_PCM_STREAM_PLAYBACK, 0) >= 0) {
                snd_pcm_hw_params_t* hw_params;
                snd_pcm_hw_params_alloca(&hw_params);
                
                if (snd_pcm_hw_params_any(pcm_handle, hw_params) >= 0) {
                    unsigned int min_channels, max_channels;
                    unsigned int min_rate, max_rate;
                    
                    snd_pcm_hw_params_get_channels_min(hw_params, &min_channels);
                    snd_pcm_hw_params_get_channels_max(hw_params, &max_channels);
                    snd_pcm_hw_params_get_rate_min(hw_params, &min_rate, 0);
                    snd_pcm_hw_params_get_rate_max(hw_params, &max_rate, 0);

                    std::cout << "  Channels: " << min_channels << " - " << max_channels << std::endl;
                    std::cout << "  Sample rates: " << min_rate << " - " << max_rate << " Hz" << std::endl;

                    // Get supported formats
                    std::cout << "  Supported formats:";
                    for (snd_pcm_format_t fmt = SND_PCM_FORMAT_S8; fmt <= SND_PCM_FORMAT_LAST; fmt = snd_pcm_format_t(fmt + 1)) {
                        if (snd_pcm_hw_params_test_format(pcm_handle, hw_params, fmt) == 0) {
                            std::cout << " " << snd_pcm_format_name(fmt);
                        }
                    }
                    std::cout << std::endl;
                }
                snd_pcm_close(pcm_handle);
            } else {
                std::cout << "  (device in use)" << std::endl;
            }
        }
    }
}

void pipedal::list_alsa_devices() {
    int card = -1;
    int err;
    
    std::cout << "=== ALSA Hardware Devices ===" << std::endl;

    // Iterate through cards
    while ((err = snd_card_next(&card)) >= 0 && card >= 0) {
        snd_ctl_t* handle;
        char name[32];
        snprintf(name, sizeof(name), "hw:%d", card);
        
        if ((err = snd_ctl_open(&handle, name, 0)) < 0) {
            std::cerr << "Cannot open control for card " << card << ": " << snd_strerror(err) << std::endl;
            continue;
        }

        // Get card info
        snd_ctl_card_info_t* info;
        snd_ctl_card_info_alloca(&info);
        if ((err = snd_ctl_card_info(handle, info)) < 0) {
            std::cerr << "Cannot get card info: " << snd_strerror(err) << std::endl;
            snd_ctl_close(handle);
            continue;
        }

        std::cout << "\nSound Card hw:" << snd_ctl_card_info_get_id(info)  << std::endl;
        std::cout << "  Name: " << snd_ctl_card_info_get_name(info) << std::endl;
        std::cout << "  Driver: " << snd_ctl_card_info_get_driver(info) << std::endl;

        // Iterate through devices on this card
        int device = -1;
        while ((err = snd_ctl_pcm_next_device(handle, &device)) >= 0 && device >= 0) {
            print_device_info(handle, card, device);
        }

        snd_ctl_close(handle);
    }
}
