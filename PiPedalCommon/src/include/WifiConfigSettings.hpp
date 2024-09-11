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

#pragma once

#include "json.hpp"


#ifndef NEW_WIFI_CONFIG
// 0-> Old hostapd-based hotspot. Now dead code. Works with buster only.
// 1-> NetworkManager-based hotspot (supported on bookworm or later).
#define NEW_WIFI_CONFIG 1
#endif

// Wifi P2P broken completely on Debian 12 updates on raspberry pi.
// 
// Setting it to enable uses current-orphaned dead code as of PiPedal v1.2.49
//
// 0: disable.
// 1: enable.
#ifndef ENABLE_WIFI_P2P
#define ENABLE_WIFI_P2P 0
#endif

namespace pipedal {


    uint32_t ChannelToWifiFrequency(const std::string &channel);
    uint32_t ChannelToWifiFrequency(uint32_t channel);
    int32_t ChannelToChannelNumber(const std::string&channel);
    
    class WifiConfigSettings {
    public:
        using ssid_t = std::vector<uint8_t>;
        WifiConfigSettings();

        void Load();
        void Save();

        bool valid_ = false;
        bool wifiWarningGiven_ = false;
        bool rebootRequired_ = false;
        bool enable_ = false;
        std::string countryCode_ = "US"; // iso 3661
        std::string hotspotName_ = "pipedal";
        std::string mdnsName_ = "pipedal";
        std::vector<ssid_t> homeNetworks_;
        bool hasPassword_ = false;
        std::string password_;
        std::string channel_ = "";
        bool alwaysOn_ = false;

        void ParseArguments(const std::vector<std::string> &arguments);
        static bool ValidateCountryCode(const std::string&value);
        static bool ValidateChannel(const std::string&countryCode,const std::string&value);

        bool operator==(const WifiConfigSettings&other) const;
        bool ConfigurationChanged(const WifiConfigSettings&other) const;
        bool WantsHotspot(bool ethernetConnected,const std::vector<ssid_t> &availableNetworks);

    public:
        DECLARE_JSON_MAP(WifiConfigSettings);
    };
}