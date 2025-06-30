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
#include <vector>
#include <string>


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
    std::string ssidToString(const std::vector<uint8_t> &ssid);
    std::vector<std::string> ssidToStringVector(const std::vector<std::vector<uint8_t>> &ssids);


    uint32_t ChannelToWifiFrequency(const std::string &channel);
    uint32_t ChannelToWifiFrequency(uint32_t channel);
    int32_t ChannelToChannelNumber(const std::string&channel);
    

    enum class HotspotAutoStartMode {
        // saved to file. Do not remove or renumber enums.
        Never = 0, 
        NoEthernetConnection = 1,
        NotAtHome = 2,
        NoRememberedWifiConections = 3,
        Always = 4
    };
    class WifiConfigSettings {
    public:
        using ssid_t = std::vector<uint8_t>;
        WifiConfigSettings();

        void Load();
        void Save();

        int16_t autoStartMode_ = 0; // see HotspotAutoStartMode
        bool hasSavedPassword_ = false;
        std::string homeNetwork_;

        std::string countryCode_ = "US"; // iso 3661
        std::string hotspotName_ = "pipedal";
        bool hasPassword_ = false;
        std::string password_;
        std::string channel_ = "";
        std::string mdnsName_ = "pipedal";

        bool wifiWarningGiven_ = false;  // Do not use. Present only for backward compatibility.
        bool valid_ = false; // Do not use. Present only for backward compatibility.
    private:
        bool rebootRequired_ = false;  // Do not use. Present only for backward compatibility.
        bool enable_ = false; // Do not use. Present only for backward compatibility.
    public:
        bool IsEnabled() const { return autoStartMode_ != 0; }
        // Initialize from commandline arguments (see ConfigMain.cpp)
        void ParseArguments(
            const std::vector<std::string> &argv,
            HotspotAutoStartMode startMode,
            const std::string homeNetworkSsid
            );
        static bool ValidateCountryCode(const std::string&value);
        static bool ValidateChannel(const std::string&countryCode,const std::string&value);

        bool operator==(const WifiConfigSettings&other) const;
        bool ConfigurationChanged(const WifiConfigSettings&other) const;
        bool WantsHotspot(bool ethernetConnected) 
        {
            return WantsHotspot(ethernetConnected, std::vector<std::string>{}, std::vector<std::string>{});
        }
        bool WantsHotspot(
            bool ethernetConnected, 
            const std::vector<std::string> &availableRememberedNetworks, // remembered networks that are currently visible
            const std::vector<std::string> &availableNetworks // all visible networks.
        );
        bool WantsHotspot(
            bool ethernetConnected, 
            const std::vector<std::vector<uint8_t>> &availableRememberedNetworks, // remembered networks that are currently visible
            const std::vector<std::vector<uint8_t>> &availableNetworks // all visible networks.
            );
        bool NeedsScan() const;
        bool NeedsWifi() const;
    public:
        DECLARE_JSON_MAP(WifiConfigSettings);
    };
}