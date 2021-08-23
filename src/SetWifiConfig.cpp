// Copyright (c) 2021 Robin Davies
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

#include "SetWifiConfig.hpp"
#include "PiPedalException.hpp"
#include "SystemConfigFile.hpp"
#include "SysExec.hpp"

using namespace pipedal;


void pipedal::SetWifiConfig(const WifiConfigSettings&settings)
{
    char band;
    if (!settings.enable_)
    {
        SysExec("/usr/bin/systemctl stop hostapd");
        SysExec("/usr/bin/systemctl disable hostapd");

        SysExec("/usr/bin/systemctl enable wpa_supplicant");
        SysExec("/usr/bin/systemctl start wpa_supplicant");
    } else {
        boost::filesystem::path path("/etc/hostapd/hostapd.conf");
        SystemConfigFile apdConfig(path);
        apdConfig.Set("ieee80211n","1",false);
        apdConfig.Set("ieee80211d","1",false);
        apdConfig.Set("ieee80211ac","1",false);
        apdConfig.Set("ieee80211d","1",false);
        apdConfig.Set("ht_capab","[HT40][SHORT-GI-20][DSSS_CCK-40]",false);
        apdConfig.Set("rsn_pairwise","CCMP",false);

        apdConfig.Set("wmm_enabled","1");
        apdConfig.Set("auth_algs","1");
        apdConfig.Set("wpa","2");


        std::string hwMode = "g";
        if (settings.channel_.length() > 0 && settings.channel_[0] == 'a') {
            hwMode = "a"; // 5G
        } else {
            hwMode = "g"; // 2.4G
        }
        std::string channel;
        if (settings.channel_[0] > '9' || settings.channel_[0] < '0')
        {
            channel = settings.channel_.substr(1);
        } else {
            channel = settings.channel_;
            int iChannel = std::stoi(channel);
            if (iChannel <= 14) {
                hwMode = "g";
            } else {
                hwMode = "a";
            }
        }

        if (settings.password_.length() != 0)
        {
            apdConfig.Set("wpa_passphrase",settings.password_);
        }
        apdConfig.Set("ssid",settings.hotspotName_);
        apdConfig.Set("country_code",settings.countryCode_);
        apdConfig.Set("hw_mode",hwMode);
        apdConfig.Set("channel",channel);
        apdConfig.Save();

        SysExec("/usr/bin/systemctl daemon-reload");

        SysExec("/usr/bin/systemctl mask wpa_supplicant");
        SysExec("/usr/bin/systemctl stop wpa_supplicant");

        if (SysExec("/usr/bin/systemctl restart hostapd") != 0)
        {
            throw PiPedalException("Unable to start the access point.");

        }
        if (SysExec("systemctl is-active --quiet hostapd") != 0)
        {
            throw PiPedalException("Unable to start the access point.");
        }
        SysExec("/usr/bin/systemctl enable hostapd");

    }

}
