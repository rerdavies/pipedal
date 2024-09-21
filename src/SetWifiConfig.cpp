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

#include "ss.hpp"

#include "P2pConfigFiles.hpp"
#include "ServiceConfiguration.hpp"
#include "SetWifiConfig.hpp"
#include "PiPedalException.hpp"
#include "SystemConfigFile.hpp"
#include "SysExec.hpp"
#include "WriteTemplateFile.hpp"
#include <unistd.h>

using namespace pipedal;
using namespace std;

#define APD_IP_PREFIX "172.22.0"
#define APD_IP_ADDRESS APD_IP_PREFIX ".1"
#define APD_INTERFACE_ADDR APD_IP_ADDRESS "/24"

#define P2P_IP_PREFIX "172.23.0"
#define P2P_IP_ADDRESS P2P_IP_PREFIX ".2" // no gateway!
#define P2P_INTERFACE_ADDR P2P_IP_ADDRESS "/24"

#define SYSTEMCTL_BIN "/usr/bin/systemctl"

static char DNSMASQ_APD_PATH[] = "/etc/dnsmasq.d/30-pipedal-apd.conf";

static char DNSMASQ_P2P_SOURCE_PATH[] = "/etc/pipedal/config/templates/30-pipedal-p2p.conf";
static char DNSMASQ_P2P_PATH[] = "/etc/dnsmasq.d/30-pipedal-p2p.conf";

static char NETWORK_MANAGER_DNSMASQ_P2P_SOURCE_PATH[] = "/etc/pipedal/config/templates/30-pipedal-p2p-dnsmasq-nm.conf";
static char NETWORK_MANAGER_DNSMASQ_P2P_PATH[] = "/etc/dnsmasq.d/30-pipedal-p2p-dnsmasq-nm.conf";

static char NETWORK_MANAGER_SERVICE_PATH[] = "/usr/lib/systemd/system/NetworkManager.service";
static char NETWORK_MANAGER_CONFIG_PATH[] = "/etc/NetworkManager/conf.d/50-pipedal-nm.conf";
static char NETWORK_MANAGER_CONFIG_SOURCE_PATH[] = "/etc/pipedal/config/templates/50-pipedal-nm.conf";

//forward declarations
static void ConfigDhcpcdForP2p();
static void applyNetworkManagerP2pConfig();
static void applyDnsmasqP2pConfig();


static std::string gWlanAddress = "wlan0";

void pipedal::SetWifiConfigWLanAddress(const std::string &wLanAddress)
{
    gWlanAddress = wLanAddress;
}
const std::string &pipedal::GetWifiConfigWlanAddress()
{
    return gWlanAddress;
}

static bool gNetworkManagerTestExecuted = false;
static bool gUsingNetworkManager = false;

void pipedal::PretendNetworkManagerIsInstalled()
{
    gNetworkManagerTestExecuted = true;
    gUsingNetworkManager = true;
}
bool pipedal::UsingNetworkManager()
{
    if (gNetworkManagerTestExecuted)
    {
        return gUsingNetworkManager;
    }
    bool bResult = false;
    auto result = sysExecForOutput("systemctl","is-active NetworkManager");
    if (result.exitCode == EXIT_SUCCESS)
    {
        std::string text = result.output.erase(result.output.find_last_not_of(" \n\r\t")+1);
        if (text == "active")
        {
            bResult = true;
        } else if (text == "inactive")
        {
            bResult = false;
        } else {
            throw std::runtime_error(SS("UsingNetworkManager: unexpected result (" << text << ")"));
        }
        gNetworkManagerTestExecuted = true;
        gUsingNetworkManager = bResult;
        return bResult;
    }

    // does the neworkManager service path exists?
    return std::filesystem::exists(NETWORK_MANAGER_SERVICE_PATH);
}

static bool IsP2pInstalled()
{
    return std::filesystem::exists(DNSMASQ_P2P_PATH) | std::filesystem::exists(NETWORK_MANAGER_DNSMASQ_P2P_PATH);
}

static bool IsApdInstalled()
{

    return std::filesystem::exists(DNSMASQ_APD_PATH);
}


#if !NEW_WIFI_CONFIG
static void restoreApdDhcpdConfFile()
{
    // remove the interface wlan0 section.
    std::filesystem::path dhcpcdConfig("/etc/dhcpcd.conf");
    if (std::filesystem::exists(dhcpcdConfig))
    {
        SystemConfigFile dhcpcd(dhcpcdConfig);

        int line = dhcpcd.GetLineThatStartsWith("hostname");
        std::string hostNameLine;
        hostNameLine = "hostname"; // the default value.

        if (line != -1)
        {
            dhcpcd.SetLineValue(line, hostNameLine);
        }

        line = dhcpcd.GetLineNumber("interface wlan0");
        if (line != -1)
        {
            dhcpcd.EraseLine(line);
            while (line < dhcpcd.GetLineCount())
            {
                const std::string &lineValue = dhcpcd.GetLineValue(line);
                if (lineValue.length() > 0 && (lineValue[0] == ' ' || lineValue[0] == '\t'))
                {
                    dhcpcd.EraseLine(line);
                }
                else
                {
                    break;
                }
            }
        }
        dhcpcd.Save(dhcpcdConfig);
    }
}
#endif

static void restoreP2pDhcpdConfFile()
{
    // remove the interface wlan0 section.
    std::filesystem::path dhcpcdConfig("/etc/dhcpcd.conf");
    if (std::filesystem::exists(dhcpcdConfig))
    {
        SystemConfigFile dhcpcd(dhcpcdConfig);

        int line = dhcpcd.GetLineThatStartsWith("hostname");
        std::string hostNameLine;
        hostNameLine = "hostname"; // the default value.

        if (line != -1)
        {
            dhcpcd.SetLineValue(line, hostNameLine);
        }

        line = dhcpcd.GetLineNumber("interface p2p-wlan0-0"); // pre bookworm.
        if (line != -1)
        {
            dhcpcd.EraseLine(line);
            while (line < dhcpcd.GetLineCount())
            {
                const std::string &lineValue = dhcpcd.GetLineValue(line);
                if (lineValue.length() > 0 && (lineValue[0] == ' ' || lineValue[0] == '\t'))
                {
                    dhcpcd.EraseLine(line);
                }
                else
                {
                    break;
                }
            }
        }
        line = dhcpcd.GetLineNumberStartingWith("allowinterfaces p2p-"); // bookworm
        if (line != -1)
        {
            while (line < dhcpcd.GetLineCount())
            {
                    dhcpcd.EraseLine(line);
            }
        }
        dhcpcd.Save(dhcpcdConfig);
    }
}

#if !NEW_WIFI_CONFIG
static void restoreApdDnsmasqConfFile()
{
    std::filesystem::path path(DNSMASQ_APD_PATH);
    if (std::filesystem::exists(path))
    {
        std::filesystem::remove(path);
    }
}
#endif
static void restoreP2pDnsmasqConfFile()
{
    {
        std::filesystem::path path(DNSMASQ_P2P_PATH);
        if (std::filesystem::exists(path))
        {
            std::filesystem::remove(path);
        }
    }
    {
        std::filesystem::path path(NETWORK_MANAGER_DNSMASQ_P2P_PATH);
        if (std::filesystem::exists(path))
        {
            std::filesystem::remove(path);
        }
    }
}

#if !NEW_WIFI_CONFIG
static void UninstallHostApd()
{
    if (IsApdInstalled())
    {
        sysExec(SYSTEMCTL_BIN " stop hostapd");
        sysExec(SYSTEMCTL_BIN " disable hostapd");
        sysExec(SYSTEMCTL_BIN " stop dnsmasq");
        sysExec(SYSTEMCTL_BIN " disable dnsmasq");

        restoreApdDhcpdConfFile();
        restoreApdDnsmasqConfFile();

        sysExec(SYSTEMCTL_BIN " unmask wpa_supplicant"); // TODO: Do we need this now we have no_hook line.
        sysExec(SYSTEMCTL_BIN " enable wpa_supplicant");
        sysExec(SYSTEMCTL_BIN " start wpa_supplicant");
    }
}
#endif

static void InstallP2p(const WifiDirectConfigSettings &settings)
{
    cout << "Enabling P2P" << endl;

    settings.Save();
    // ******************* device_uuid

    ServiceConfiguration deviceIdFile;
    deviceIdFile.Load();
    deviceIdFile.deviceName = settings.hotspotName_;
    deviceIdFile.Save();

    // ******************** dsnmasq ******

    std::filesystem::remove(DNSMASQ_P2P_PATH);
    std::filesystem::remove(NETWORK_MANAGER_DNSMASQ_P2P_PATH);

    // dnsmasq
    if (UsingNetworkManager())
    {
        applyDnsmasqP2pConfig();
    }
    else
    {
        std::filesystem::copy_file(DNSMASQ_P2P_SOURCE_PATH, DNSMASQ_P2P_PATH);
    }

    // ****** dhcpd.conf ***/
    std::filesystem::path dhcpcdConfig("/etc/dhcpcd.conf");
    if (std::filesystem::exists(dhcpcdConfig))
    {
        ConfigDhcpcdForP2p();
    }

    /* Network manager */
    if (UsingNetworkManager())
    {
        applyNetworkManagerP2pConfig();
    }

    // **************** start services ************

    cout << "Starting P2P services." << endl;
    sysExec("rfkill unblock wlan");

    sysExec(SYSTEMCTL_BIN " daemon-reload");
    sysExec(SYSTEMCTL_BIN " stop pipedal_nm_p2pd");
    sysExec(SYSTEMCTL_BIN " stop pipedal_p2pd");
    sysExec(SYSTEMCTL_BIN " stop dhcpcd");
    if (UsingNetworkManager())
    {
        sysExec(SYSTEMCTL_BIN " restart NetworkManager");
        // NetworkManager takes care of resetting wpa_supplicant.
    }
    else
    {
        sysExec(SYSTEMCTL_BIN " restart wpa_supplicant");
    }
    sysExec(SYSTEMCTL_BIN " start dhcpcd");

    sysExec(SYSTEMCTL_BIN " enable wpa_supplicant"); // ?? Not sure that's still right. :-/
    sysExec(SYSTEMCTL_BIN " start dnsmasq");
    sysExec(SYSTEMCTL_BIN " enable dnsmasq");
    if (UsingNetworkManager())
    {
        if (sysExec(SYSTEMCTL_BIN " start pipedal_nm_p2pd") != EXIT_SUCCESS)
        {
            throw PiPedalException("Unable to start the Wi-Fi Direct access point service (pipedal_nm_p2pd).");
        }
        sysExec(SYSTEMCTL_BIN " enable pipedal_nm_p2pd");
    }
    else
    {
        if (sysExec(SYSTEMCTL_BIN " start pipedal_p2pd") != EXIT_SUCCESS)
        {
            throw PiPedalException("Unable to start the Wi-Fi Direct access point service (pipedal_p2pd).");
        }
        sysExec(SYSTEMCTL_BIN " enable pipedal_p2pd");
    }

    sysExec(SYSTEMCTL_BIN " restart pipedald");
}

static void UninstallP2p();

#if !NEW_WIFI_CONFIG
static void SetHostapdWifiConfig(const WifiConfigSettings &settings)
{
    char band;
    if (!settings.enable_)
    {
        UninstallHostApd();
    }
    else
    {
        UninstallP2p();

        std::filesystem::path path("/etc/hostapd/hostapd.conf");
        SystemConfigFile apdConfig;
        if (std::filesystem::exists(path))
        {
            apdConfig.Load(path);
        }
        if (!UsingNetworkManager())
        {}
        apdConfig.Set("interface", "wlan0");
        apdConfig.Set("driver", "nl80211");
        apdConfig.Set("country_code", settings.countryCode_);

        apdConfig.Set("ieee80211n", "1", "Wi-Fi features");
        apdConfig.SetDefault("ieee80211d", "1");
        apdConfig.SetDefault("ieee80211ac", "1");
        apdConfig.SetDefault("ieee80211d", "1");
        apdConfig.Set("ht_capab", "[HT40][SHORT-GI-20][DSSS_CCK-40]");

        apdConfig.Set("wmm_enabled", "1", "Authentication options");
        apdConfig.Set("auth_algs", "1");
        apdConfig.Set("wpa", "2");
        apdConfig.SetDefault("wpa_pairwise", "TKIP CCMP");
        apdConfig.SetDefault("wpa_ptk_rekey", "600");
        apdConfig.SetDefault("rsn_pairwise", "CCMP");

        std::string hwMode = "g";
        if (settings.channel_.length() > 0 && settings.channel_[0] == 'a')
        {
            hwMode = "a"; // 5G
        }
        else
        {
            hwMode = "g"; // 2.4G
        }
        std::string channel;
        if (settings.channel_[0] > '9' || settings.channel_[0] < '0')
        {
            channel = settings.channel_.substr(1);
        }
        else
        {
            channel = settings.channel_;
            int iChannel = std::stoi(channel);
            if (iChannel <= 14)
            {
                hwMode = "g";
            }
            else
            {
                hwMode = "a";
            }
        }

        apdConfig.Set("ssid", settings.hotspotName_, "Access point configuration");
        if (settings.password_.length() != 0)
        {
            apdConfig.Set("wpa_passphrase", settings.password_);
        }
        apdConfig.Set("hw_mode", hwMode);
        apdConfig.Set("channel", channel);

        // ******************** dsnmasq ******
        std::filesystem::path dnsMasqPath(DNSMASQ_APD_PATH);
        SystemConfigFile dnsMasq;

        try
        {
            dnsMasq.Load(dnsMasqPath);
        }
        catch (const std::exception &)
        {
            // ignore.
        }

        dnsMasq.Set("interface", "wlan0", "Name of the Wi-Fi interface");
        dnsMasq.Set("listen-address", APD_IP_ADDRESS);

        dnsMasq.SetDefault("dhcp-range", APD_IP_PREFIX ".3," APD_IP_PREFIX ".127,12h", "dhcp configuration");
        dnsMasq.SetDefault("domain", "local");
        std::stringstream sAddress;
        sAddress << "/" << settings.mdnsName_ << ".local/" APD_IP_PREFIX << ".1";
        dnsMasq.Set("address", sAddress.str());

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
            }
            else
            {
                hostNameLine = "hostname"; // the default value.
            }
            if (line != -1)
            {
                dhcpcd.SetLineValue(line, hostNameLine);
            }
            else
            {
                dhcpcd.AppendLine(hostNameLine);
            }
            line = dhcpcd.GetLineNumber("interface wlan0");
            if (line != -1)
            {
                dhcpcd.EraseLine(line);
                while (line < dhcpcd.GetLineCount())
                {
                    const std::string &lineValue = dhcpcd.GetLineValue(line);
                    if (lineValue.length() > 0 && (lineValue[0] == ' ' || lineValue[0] == '\t'))
                    {
                        dhcpcd.EraseLine(line);
                    }
                    else
                    {
                        break;
                    }
                }
            }
            if (line == -1)
            {
                dhcpcd.AppendLine("");
                line = dhcpcd.GetLineCount();
            }
            dhcpcd.InsertLine(line++, "interface wlan0");
            dhcpcd.InsertLine(line++, "   static ip_address=" APD_INTERFACE_ADDR);
            dhcpcd.InsertLine(line++, "   nohook wpa_supplicant");
            dhcpcd.Save(dhcpcdConfig);
        }

        // ***** save the config files ***

        apdConfig.Save(path);
        dnsMasq.Save(dnsMasqPath);

        // **************** start services ************
        sysExec("rfkill unblock wlan");
        sysExec(SYSTEMCTL_BIN " daemon-reload");

        //        sysExec(SYSTEMCTL_BIN " mask wpa_supplicant");
        sysExec(SYSTEMCTL_BIN " stop wpa_supplicant");

        // sysExec(SYSTEMCTL_BIN " unmask hostapd");
        // if (sysExec(SYSTEMCTL_BIN " restart hostapd") != 0)
        // {
        //     throw PiPedalException("Unable to start the access point.");
        // }
        // if (sysExec("systemctl is-active --quiet hostapd") != 0)
        // {
        //     throw PiPedalException("Unable to start the access point.");
        // }
        // sysExec(SYSTEMCTL_BIN " enable hostapd");

        sysExec(SYSTEMCTL_BIN " stop wpa_supplicant");
        //        sysExec(SYSTEMCTL_BIN " mask wpa_supplicant");

        sysExec(SYSTEMCTL_BIN " restart dnsmasq");
        sysExec(SYSTEMCTL_BIN " enable dnsmasq");
    }
    ::sync();
}
#endif

#if NEW_WIFI_CONFIG

static void SetNetworkManagerWifiConfig(WifiConfigSettings &settings)
{
    settings.Save();
}
#endif

void pipedal::SetWifiConfig(WifiConfigSettings &settings)
{
#if NEW_WIFI_CONFIG
    SetNetworkManagerWifiConfig(settings);
#else 
    SetHostapdWifiConfig(const WifiConfigSettings &settings);
#endif
}


/*********************************************************************************

p2p configuration:


dnsmaqsq.d/30-pipedal-p2p.conf:

interface=p2p-wlan0-0
    dhcp-range=p2p-wlan0,173.24.0.3,172.23.0.127,1h
    domain=local
    address=/pipedal.local/172.23.0.2

except-interface=eth0
except-interface=wlan0
except-interface=lo


dhcpcd.conf:
======
interface p2p-wlan0-0
     static ip_address=172.24.0.1/16
     domain_name_servers=172.24.0.1

Watch resolv.conf to make sure we don't lose DNS servers.
************************************************************************************/

using namespace pipedal;

static void restoreNetworkManagerP2pConfig()
{
    std::filesystem::path path(NETWORK_MANAGER_CONFIG_PATH);
    if (std::filesystem::exists(path))
    {
        std::filesystem::remove(path);
    }
}
static void applyNetworkManagerP2pConfig()
{
    std::filesystem::path path(NETWORK_MANAGER_CONFIG_PATH);

    std::map<std::string, std::string> map;
    map["WLAN"] = GetWifiConfigWlanAddress();
    WriteTemplateFile(map, NETWORK_MANAGER_CONFIG_SOURCE_PATH, NETWORK_MANAGER_CONFIG_PATH);
}

static void applyDnsmasqP2pConfig()
{
    std::map<std::string, std::string> map;
    map["WLAN"] = GetWifiConfigWlanAddress();
    WriteTemplateFile(map, NETWORK_MANAGER_DNSMASQ_P2P_SOURCE_PATH, NETWORK_MANAGER_DNSMASQ_P2P_PATH);
}

void UninstallP2p()
{
    if (IsP2pInstalled())
    {
        if (UsingNetworkManager())
        {
            sysExec(SYSTEMCTL_BIN " stop pipedal_nm_p2pd");
            sysExec(SYSTEMCTL_BIN " disable pipedal_nm_p2pd");
        }
        else
        {
            sysExec(SYSTEMCTL_BIN " stop pipedal_p2pd");
            sysExec(SYSTEMCTL_BIN " disable pipedal_p2pd");
        }

        sysExec(SYSTEMCTL_BIN " stop dnsmasq");
        sysExec(SYSTEMCTL_BIN " disable dnsmasq");

        restoreP2pDhcpdConfFile();
        restoreP2pDnsmasqConfFile();
        restoreNetworkManagerP2pConfig();

        if (!UsingNetworkManager())
        {
            sysExec(SYSTEMCTL_BIN " restart dhcpcd");
        } else {
            sysExec(SYSTEMCTL_BIN " restart NetworkManager"); // bring up wlan0 wifi.

        }

    }
    if (UsingNetworkManager())
    {
        // dhcpcd comes up enabled after an install, so stop it always.
        sysExec(SYSTEMCTL_BIN " stop dhcpcd");
        sysExec(SYSTEMCTL_BIN " disable dhcpcd");

    }
    ::sync();
}

static void RemoveDhcpcdConfig()
{
    std::filesystem::path dhcpcdConfig("/etc/dhcpcd.conf");
    SystemConfigFile dhcpcd(dhcpcdConfig);

    int line = dhcpcd.GetLineNumber("interface p2p-wlan0-0");
    if (line != -1)
    {
        dhcpcd.EraseLine(line);
        while (line < dhcpcd.GetLineCount())
        {
            const std::string &lineValue = dhcpcd.GetLineValue(line);
            if (lineValue.length() > 0 && (lineValue[0] == ' ' || lineValue[0] == '\t'))
            {
                dhcpcd.EraseLine(line);
            }
            else
            {
                break;
            }
        }
    }
    line = dhcpcd.GetLineNumberStartingWith("allowinterfaces p2p-");
    if (line != -1)
    {
        dhcpcd.EraseLine(line);
        while (line < dhcpcd.GetLineCount())
        {
            dhcpcd.EraseLine(line);
        }
    } 
}
static void ConfigDhcpcdForP2p()
{
    RemoveDhcpcdConfig();

    std::filesystem::path dhcpcdConfig("/etc/dhcpcd.conf");
    cout << "Updating /etc/dhcpcd.conf" << endl;
    SystemConfigFile dhcpcd(dhcpcdConfig);

    // dhcpcd.conf:
    // ======
    // interface p2p-wlan0-0
    //      static ip_address=172.24.0.1/24
    //      domain_name_server=172.24.0.1

    int line = dhcpcd.GetLineThatStartsWith("hostname");
    std::string hostNameLine;
    hostNameLine = "hostname"; // the default value.
    if (line != -1)
    {
        dhcpcd.SetLineValue(line, hostNameLine);
    }
    else
    {
        dhcpcd.AppendLine(hostNameLine);
    }

    if (!UsingNetworkManager())
    {
        line = dhcpcd.GetLineCount();
        // interface p2p-wlan0-0
        //      static ip_address=172.24.0.1/16 (trying int on .1.)
        //      domain_name_server=172.24.0.1

        dhcpcd.InsertLine(line++, "interface p2p-wlan0-0");
        dhcpcd.InsertLine(line++, "   static ip_address=" P2P_INTERFACE_ADDR);
        dhcpcd.InsertLine(line++, "   nohook wpa_supplicant");
    }
    else
    {
        if (line == -1)
        {
            line = dhcpcd.GetLineCount();
        }
        // interface p2p-wlan0-0
        //      static ip_address=172.24.0.1/16 (trying int on .1.)
        //      domain_name_server=172.24.0.1

        dhcpcd.InsertLine(line++, SS("allowinterfaces p2p-" << GetWifiConfigWlanAddress() << "-*").c_str());
        dhcpcd.InsertLine(line++, "static ip_address=" P2P_INTERFACE_ADDR);
        dhcpcd.InsertLine(line++, "nohook wpa_supplicant");
    }
    dhcpcd.Save(dhcpcdConfig);
}
void pipedal::SetWifiDirectConfig(const WifiDirectConfigSettings &settings)
{
    settings.Save();
    try
    {
        char band;
        if (!settings.enable_)
        {
            cout << "Disabling P2P" << endl;
            UninstallP2p();

            ServiceConfiguration deviceIdFile;
            deviceIdFile.Load();
            deviceIdFile.deviceName = settings.hotspotName_;
            deviceIdFile.Save();

            // Announce new mDNS service with potentially new deviceName.
            sysExec(SYSTEMCTL_BIN " restart pipedald");
        }
        else
        {
            InstallP2p(settings);
        }
    }
    catch (const std::exception &e)
    {
        cout << e.what() << endl;
    }
    ::sync();
}
void pipedal::OnWifiReinstall() {
#if ENABLE_WIFI_P2P // Do not enable P2P going forward.
    WifiDirectConfigSettings settings;
    settings.Load();
    if (settings.enable_)
    {
        SetWifiDirectConfig(settings);
        ::sync();
    }
#endif
#if NEW_WIFI_CONFIG
    {
        // no action required.
        // pipedald will pick up the old settings at runtime.
    }

#endif
}
void pipedal::OnWifiUninstall(bool preserveState)
{
    // intaller hook
#if NEW_WIFI_CONFIG
    {

        // no action required.
        // shutting down pipedald is sufficient.
    }
#else
    if (IsApdInstalled())
    {
        UninstallHostApd();
    }
#endif
    if (IsP2pInstalled())
    {
        WifiDirectConfigSettings settings;
        settings.Load();
        UninstallP2p();

        // preserve the state for future installs.
        if (preserveState)
        {
            settings.Save();
        }
    }
    ::sync();
}
void pipedal::OnWifiInstallComplete()
{
    // install has already done a daemon-reload.
    // I can't think of anything that needs to be done.
    // if (IsApdInstalled())
    // {
    //     sysExec(SYSTEMCTL_BIN " daemon-reload");
    // } else if (IsP2pInstalled())
    // {
    //     sysExec(SYSTEMCTL_BIN " daemon-reload");
    // }
}

static void restartNetworkManagerService()
{
    if (UsingNetworkManager())
    {
        sysExec(SYSTEMCTL_BIN " restart NetworkManager");
    }
}

