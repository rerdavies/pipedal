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

#include "SetWifiConfig.hpp"
#include "PiPedalException.hpp"
#include "SystemConfigFile.hpp"
#include "SysExec.hpp"

using namespace pipedal;

#define IP_RANGE "172.22.0"
#define PIPEDAL_IP IP_RANGE ".1"

#define PIPEDAL_NETWORK PIPEDAL_IP "/24"

#define SYSTEMCTL_BIN "/usr/bin/systemctl"
void pipedal::SetWifiConfig(const WifiConfigSettings&settings)
{
    char band;
    if (!settings.enable_)
    {
        sysExec(SYSTEMCTL_BIN " stop hostapd");
        sysExec(SYSTEMCTL_BIN " disable hostapd");

        sysExec(SYSTEMCTL_BIN " enable wpa_supplicant");
        sysExec(SYSTEMCTL_BIN " start wpa_supplicant");
    } else {
        std::filesystem::path path("/etc/hostapd/hostapd.conf");
        SystemConfigFile apdConfig;
        if (std::filesystem::exists(path))
        {
            apdConfig.Load(path);
        }
        apdConfig.Set("interface","wlan0");
        apdConfig.Set("driver","nl80211");
        apdConfig.Set("country_code",settings.countryCode_);

        apdConfig.Set("ieee80211n","1","Wi-Fi features");
        apdConfig.SetDefault("ieee80211d","1");
        apdConfig.SetDefault("ieee80211ac","1");
        apdConfig.SetDefault("ieee80211d","1");
        apdConfig.Set("ht_capab","[HT40][SHORT-GI-20][DSSS_CCK-40]");


        apdConfig.Set("wmm_enabled","1","Authentication options");
        apdConfig.Set("auth_algs","1");
        apdConfig.Set("wpa","2");
        apdConfig.SetDefault("wpa_pairwise","TKIP CCMP");
        apdConfig.SetDefault("wpa_ptk_rekey","600");
        apdConfig.SetDefault("rsn_pairwise","CCMP");

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

        apdConfig.Set("ssid",settings.hotspotName_,"Access point configuration");
        if (settings.password_.length() != 0)
        {
            apdConfig.Set("wpa_passphrase",settings.password_);
        }
        apdConfig.Set("hw_mode",hwMode);
        apdConfig.Set("channel",channel);

        // ******************** dsnmasq ******
        std::filesystem::path dnsMasqPath("/etc/dnsmasq.conf");
        SystemConfigFile dnsMasq;
        
        try {
            dnsMasq.Load(dnsMasqPath);
        } catch (const std::exception &)
        {
            // ignore.
        }

        
        dnsMasq.Set("interface","wlan0","Name of the Wi-Fi interface");
        dnsMasq.Set("listen-address",PIPEDAL_IP);

        dnsMasq.SetDefault("dhcp-range", IP_RANGE ".3," IP_RANGE ".127,12h","dhcp configuration");
        dnsMasq.SetDefault("domain","local");
        std::stringstream sAddress;
        sAddress << "/" << settings.mdnsName_ << ".local/" IP_RANGE << ".1";
        dnsMasq.Set("address",sAddress.str());

        // ****** dhcpd.conf ***/
        std::filesystem::path dhcpcdConfig("/etc/dhcpcd.conf");
        if (std::filesystem::exists(dhcpcdConfig))
        {
            SystemConfigFile dhcpcd(dhcpcdConfig);

            int line = dhcpcd.GetLineThatStartsWith("hostname");
            std::string hostNameLine;
            if (settings.mdnsName_ != "")
            {
                hostNameLine = "hostname " + settings.mdnsName_;
            } else {
                hostNameLine = "hostname"; // the default value.
            }
            if (line != -1) {
                dhcpcd.SetLineValue(line,hostNameLine);
            } else {
                dhcpcd.AppendLine(hostNameLine);
            }
            line = dhcpcd.GetLineNumber("interface wlan0");
            if (line != -1) {
                dhcpcd.EraseLine(line);
                while (line < dhcpcd.GetLineCount())
                {
                    const std::string &lineValue = dhcpcd.GetLineValue(line);
                    if (lineValue.length() > 0 && (lineValue[0] == ' ' || lineValue[0] == '\t'))
                    {
                        dhcpcd.EraseLine(line);
                    } else {
                        break;
                    }
                }
            }
            if (line == -1) {
                dhcpcd.AppendLine("");
                line = dhcpcd.GetLineCount();
            }
            dhcpcd.InsertLine(line++,"interface wlan0");
            dhcpcd.InsertLine(line++,"   static ip_address=" PIPEDAL_NETWORK);
            dhcpcd.InsertLine(line++,"   nohook wpa_supplicant");
            dhcpcd.Save(dhcpcdConfig); 
        }

        // ***** save the config files ***

        apdConfig.Save(path);
        dnsMasq.Save(dnsMasqPath);



        // **************** start services ************
        sysExec("rfkill unblock wlan");
        sysExec(SYSTEMCTL_BIN " daemon-reload");

        sysExec(SYSTEMCTL_BIN " mask wpa_supplicant");
        sysExec(SYSTEMCTL_BIN " stop wpa_supplicant");

        sysExec(SYSTEMCTL_BIN " unmask hostapd");
        if (sysExec(SYSTEMCTL_BIN " restart hostapd") != 0)
        {
            throw PiPedalException("Unable to start the access point.");

        }
        if (sysExec("systemctl is-active --quiet hostapd") != 0)
        {
            throw PiPedalException("Unable to start the access point.");
        }
        sysExec(SYSTEMCTL_BIN " enable hostapd");
        
        sysExec(SYSTEMCTL_BIN " stop wpa_supplicant");
        sysExec(SYSTEMCTL_BIN " mask wpa_supplicant");

        sysExec(SYSTEMCTL_BIN " restart dnsmasq");
        sysExec(SYSTEMCTL_BIN " enable dnsmasq");

    }

}

