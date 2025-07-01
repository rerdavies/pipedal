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

#include "WifiConfigSettings.hpp"
#include <stdexcept>
#include "ss.hpp"
#include "ChannelInfo.hpp"
#include <filesystem>
#include "ofstream_synced.hpp"
#include <stdexcept>
#include <cctype>
#include "SysExec.hpp"
#include "Lv2Log.hpp"
#include <sdbus-c++/sdbus-c++.h>
#include <iostream>
#include <memory>
#include "util.hpp"

using namespace pipedal;
using namespace std;
namespace fs = std::filesystem;

static const fs::path CONFIG_PATH = "/var/pipedal/config/wifiConfig.json";

WifiConfigSettings::WifiConfigSettings()
{
    Load();
}

static std::string getWifiCountryCode()
{
    try
    {
        SysExecOutput result = sysExecForOutput("iw", "reg get");
        if (result.exitCode != EXIT_SUCCESS)
        {
            return "";
        }
        const std::string &output = result.output;
        size_t pos = output.find("country");
        if (pos != std::string::npos)
        {
            std::string result = output.substr(pos + 8, 2);
            if (WifiConfigSettings::ValidateCountryCode(result))
            {
                return result;
            }
        }
    }
    catch (const std::exception &e)
    {
        return ""; // unset.
    }
    return "";
}

static void setWifiCountryCode(const std::string &countryCode)
{
    if (!WifiConfigSettings::ValidateCountryCode(countryCode))
    {
        throw std::runtime_error("Invalid country code.");
    }
    SysExecOutput result = sysExecForOutput("iw", SS("reg set " << countryCode));
    if (result.exitCode != EXIT_SUCCESS)
    {
        throw std::runtime_error(SS("Failed to set country code: " << result.output));
    }
}
bool WifiConfigSettings::ValidateCountryCode(const std::string &text)
{
    if (text.length() != 2)
        return false;
    return std::isalpha(text[0]) && std::isalpha(text[1]);
}

void WifiConfigSettings::Load()
{
    try
    {
        if (fs::exists(CONFIG_PATH))
        {
            std::ifstream f(CONFIG_PATH);
            if (!f)
            {
                throw std::runtime_error("Can't open file.");
            }
            json_reader reader(f);
            reader.read(this);
        }
        this->countryCode_ = getWifiCountryCode();
        this->enable_ = this->autoStartMode_ != (uint16_t)HotspotAutoStartMode::Never;
        this->mdnsName_ = GetHostName();
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(SS("Can't load WifiConfigSettings. " << e.what()));
    }
}

static void openWithPerms(
    pipedal::ofstream_synced &f,
    const std::filesystem::path &path,
    std::filesystem::perms perms =
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write |
        std::filesystem::perms::group_read | std::filesystem::perms::group_write)
{
    auto directory = path.parent_path();
    if (!std::filesystem::exists(directory))
    {
        std::filesystem::create_directories(directory);
        // open and close to make an existing empty file.
        // close it.
        {
            std::ofstream f;
            f.open(path);
            f.close();
        }
        try
        {
            // set the perms.
            std::filesystem::permissions(
                path,
                perms,
                std::filesystem::perm_options::replace);
        }
        catch (const std::exception &)
        {
            Lv2Log::warning(SS("Failed to set permissions on" << path << "."));
        }
    }

    // open for re3al.
    f.open(path);
}

void WifiConfigSettings::Save()
{
    WifiConfigSettings newSettings{*this};

    // sync legacy settings, just in case i don't know what.
    newSettings.mdnsName_ = newSettings.hotspotName_;
    newSettings.enable_ = newSettings.IsEnabled();
    // fill in the password, if required.
    if (!newSettings.hasPassword_)
    {
        WifiConfigSettings oldSettings;
        oldSettings.Load();
        newSettings.hasPassword_ = oldSettings.hasPassword_;
        newSettings.password_ = oldSettings.password_;
    }
    newSettings.hasSavedPassword_ = newSettings.hasPassword_;

    try
    {
        ofstream_synced f;
        openWithPerms(f, CONFIG_PATH);
        json_writer writer(f, false);
        writer.write(&newSettings);
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(SS("Can't save WifiConfigSettings. " << e.what()));
    }
}

namespace pipedal::priv
{

    class Device
    {
    public:
        Device(const std::string &path)
            : m_proxy(sdbus::createProxy(
                  sdbus::createSystemBusConnection(),
                  "org.freedesktop.NetworkManager",
                  path))
        {
        }

        uint32_t GetDeviceType() const
        {
            sdbus::Variant device_type_variant;
            m_proxy->callMethod("Get")
                .onInterface("org.freedesktop.DBus.Properties")
                .withArguments("org.freedesktop.NetworkManager.Device", "DeviceType")
                .storeResultsTo(device_type_variant);
            return device_type_variant.get<uint32_t>();
        }

        std::string GetPath() const
        {
            return m_proxy->getObjectPath();
        }

    private:
        std::unique_ptr<sdbus::IProxy> m_proxy;
    };
    class NetworkManager
    {
    public:
        NetworkManager()
            : m_proxy(sdbus::createProxy(
                  sdbus::createSystemBusConnection(),
                  "org.freedesktop.NetworkManager",
                  "/org/freedesktop/NetworkManager")),
              m_settings_proxy(sdbus::createProxy(
                  sdbus::createSystemBusConnection(),
                  "org.freedesktop.NetworkManager",
                  "/org/freedesktop/NetworkManager/Settings"))
        {
        }

        struct HotspotInfo
        {
            std::string connection_path;
            std::string active_connection_path;
        };

        std::string AddConnection(const std::map<std::string, std::map<std::string, sdbus::Variant>> &connection)
        {
            std::string path;
            m_settings_proxy->callMethod("AddConnection")
                .onInterface("org.freedesktop.NetworkManager.Settings")
                .withArguments(connection)
                .storeResultsTo(path);
            return path;
        }
        std::string AddConnection(const std::string &ssid, const std::string &password)
        {
            // Create connection
            std::map<std::string, std::map<std::string, sdbus::Variant>> connection;
            connection["connection"]["type"] = sdbus::Variant("802-11-wireless");
            connection["connection"]["id"] = sdbus::Variant(ssid);
            std::vector<uint8_t> ssid_bytes(ssid.begin(), ssid.end());
            connection["802-11-wireless"]["ssid"] = sdbus::Variant(ssid_bytes);
            connection["802-11-wireless"]["mode"] = sdbus::Variant(std::string("ap"));
            connection["802-11-wireless-security"]["key-mgmt"] = sdbus::Variant(std::string("wpa-psk"));
            connection["802-11-wireless-security"]["psk"] = sdbus::Variant(password);
            connection["ipv4"]["method"] = sdbus::Variant(std::string("shared"));
            connection["ipv6"]["method"] = sdbus::Variant(std::string("ignore"));

            return AddConnection(connection);
        }

        std::string ActivateConnection(const std::string &connection_path, const std::string &device_path)
        {
            std::string active_connection_path;
            m_proxy->callMethod("ActivateConnection")
                .onInterface("org.freedesktop.NetworkManager")
                .withArguments(connection_path, device_path, "/")
                .storeResultsTo(active_connection_path);
            return active_connection_path;
        }

        void DeactivateConnection(const std::string &active_connection_path)
        {
            m_proxy->callMethod("DeactivateConnection")
                .onInterface("org.freedesktop.NetworkManager")
                .withArguments(active_connection_path);
        }

        std::string GetWirelessDevicePath()
        {
            auto devices = GetAllDevices();
            for (const auto &device : devices)
            {
                if (device.GetDeviceType() == 2)
                { // 2 is the value for Wi-Fi devices
                    return device.GetPath();
                }
            }
            throw std::runtime_error("No Wi-Fi device found");
        }

        std::string CreateHotspot(const std::string &ssid, const std::string &password)
        {
            // Create connection
            std::map<std::string, std::map<std::string, sdbus::Variant>> connection;
            connection["connection"]["type"] = sdbus::Variant("802-11-wireless");
            connection["connection"]["id"] = sdbus::Variant(ssid);
            std::vector<uint8_t> ssid_bytes(ssid.begin(), ssid.end());
            connection["802-11-wireless"]["ssid"] = sdbus::Variant(ssid_bytes);
            connection["802-11-wireless"]["mode"] = sdbus::Variant(std::string("ap"));
            connection["802-11-wireless-security"]["key-mgmt"] = sdbus::Variant(std::string("wpa-psk"));
            connection["802-11-wireless-security"]["psk"] = sdbus::Variant(password);
            connection["ipv4"]["method"] = sdbus::Variant(std::string("shared"));
            connection["ipv6"]["method"] = sdbus::Variant(std::string("ignore"));

            connection["ipv4"]["address-data"] = sdbus::Variant(std::vector<std::map<std::string, sdbus::Variant>>{{{"address", sdbus::Variant("192.168.4.1")},
                                                                                                                    {"prefix", sdbus::Variant(uint32_t(24))}}});
            connection["ipv4"]["dhcp-send-hostname"] = sdbus::Variant(true);
            connection["ipv4"]["dhcp-hostname"] = sdbus::Variant("raspberrypi");
            connection["ipv4"]["dhcp-options"] = sdbus::Variant(std::vector<std::string>{
                "option:classless-static-route,192.168.4.0/24,0.0.0.0,0.0.0.0/0,192.168.4.1"
                //"option:classless-static-route,192.168.4.0/24,192.168.4.1"
            });

            auto connection_path = AddConnection(connection);
            return connection_path;
        }

        std::string EnableHotspot(const std::string &connection_path)
        {

            // Enable hotspot
            std::string device_path = GetWirelessDevicePath();
            std::string active_connection_path = ActivateConnection(connection_path, device_path);

            return active_connection_path;
        }

        void RemoveConnection(const std::string &connection)
        {
            auto connection_proxy = sdbus::createProxy(
                m_proxy->getConnection(),
                "org.freedesktop.NetworkManager",
                connection);
            connection_proxy->callMethod("Delete")
                .onInterface("org.freedesktop.NetworkManager.Settings.Connection");
        }

        std::optional<std::string> GetActiveConnection(const std::string &device_path)
        {
            std::vector<sdbus::ObjectPath> active_connections;
            m_proxy->callMethod("Get")
                .onInterface("org.freedesktop.DBus.Properties")
                .withArguments("org.freedesktop.NetworkManager", "ActiveConnections")
                .storeResultsTo(active_connections);

            for (const auto &conn : active_connections)
            {
                auto conn_proxy = sdbus::createProxy(
                    m_proxy->getConnection(),
                    "org.freedesktop.NetworkManager",
                    conn);

                sdbus::ObjectPath conn_device;
                conn_proxy->callMethod("Get")
                    .onInterface("org.freedesktop.DBus.Properties")
                    .withArguments("org.freedesktop.NetworkManager.Connection.Active", "Devices")
                    .storeResultsTo(conn_device);

                if (conn_device == device_path)
                {
                    return conn;
                }
            }

            return std::nullopt;
        }
        std::string GetHotspotConnectionPath(const std::string &ssid)
        {
            std::vector<sdbus::ObjectPath> connections;
            m_settings_proxy->callMethod("ListConnections")
                .onInterface("org.freedesktop.NetworkManager.Settings")
                .storeResultsTo(connections);

            for (const auto &conn_path : connections)
            {
                auto conn_proxy = sdbus::createProxy(sdbus::createSystemBusConnection(),
                                                     "org.freedesktop.NetworkManager",
                                                     conn_path);

                sdbus::Variant settings_variant;
                conn_proxy->callMethod("GetSettings")
                    .onInterface("org.freedesktop.NetworkManager.Settings.Connection")
                    .storeResultsTo(settings_variant);

                auto settings = settings_variant.get<std::map<std::string, std::map<std::string, sdbus::Variant>>>();

                // Check if this is a Wi-Fi connection
                if (settings["connection"]["type"].get<std::string>() == "802-11-wireless")
                {
                    // Check if it's an access point (hotspot) connection
                    if (settings["802-11-wireless"]["mode"].get<std::string>() == "ap")
                    {
                        // Check if the SSID matches
                        std::vector<uint8_t> conn_ssid = settings["802-11-wireless"]["ssid"].get<std::vector<uint8_t>>();
                        std::string conn_ssid_str(conn_ssid.begin(), conn_ssid.end());
                        if (conn_ssid_str == ssid)
                        {
                            return conn_path;
                        }
                    }
                }
            }

            return ""; // No matching hotspot connection found
        }

    private:
        std::vector<Device> GetAllDevices()
        {
            sdbus::Variant devices_variant;
            m_proxy->callMethod("Get")
                .onInterface("org.freedesktop.DBus.Properties")
                .withArguments("org.freedesktop.NetworkManager", "AllDevices")
                .storeResultsTo(devices_variant);

            std::vector<sdbus::ObjectPath> device_paths = devices_variant.get<std::vector<sdbus::ObjectPath>>();
            std::vector<Device> devices;
            for (const auto &path : device_paths)
            {
                devices.emplace_back(path);
            }
            return devices;
        }

        std::unique_ptr<sdbus::IProxy> m_proxy;
        std::unique_ptr<sdbus::IProxy> m_settings_proxy;
    };
}

JSON_MAP_BEGIN(WifiConfigSettings)
// v0
JSON_MAP_REFERENCE(WifiConfigSettings, valid)
JSON_MAP_REFERENCE(WifiConfigSettings, wifiWarningGiven)
JSON_MAP_REFERENCE(WifiConfigSettings, rebootRequired)
JSON_MAP_REFERENCE(WifiConfigSettings, enable)
JSON_MAP_REFERENCE(WifiConfigSettings, hotspotName)
JSON_MAP_REFERENCE(WifiConfigSettings, mdnsName)
JSON_MAP_REFERENCE(WifiConfigSettings, hasPassword)
JSON_MAP_REFERENCE(WifiConfigSettings, password)
JSON_MAP_REFERENCE(WifiConfigSettings, countryCode)
JSON_MAP_REFERENCE(WifiConfigSettings, channel)

// v1: Auto-hotspot

JSON_MAP_REFERENCE(WifiConfigSettings, autoStartMode)
JSON_MAP_REFERENCE(WifiConfigSettings, homeNetwork)
JSON_MAP_REFERENCE(WifiConfigSettings, hasSavedPassword)

JSON_MAP_END()

int32_t pipedal::ChannelToChannelNumber(const std::string &channel)
{
    std::string t = channel;
    // remove deprecated band specs.
    if (t.size() > 1 && t[0] == 'a' || t[0] == 'g')
    {
        t = t.substr(1);
    }
    int32_t channelNumber = 0;
    std::stringstream ss(t);
    ss >> channelNumber;
    return channelNumber;
}

static uint32_t ParseChannel(const std::string &channel)
{
    std::string t = channel;
    // remove dprecated band specs.
    if (t.size() > 1 && t[0] == 'a' || t[0] == 'g')
    {
        t = t.substr(1);
    }
    size_t size = t.length();

    unsigned long long lChannel = std::stoull(t, &size);
    if (size != t.length())
    {
        throw invalid_argument("Expecting a number: '" + t + "'.");
    }
    return (uint32_t)lChannel;
}

uint32_t pipedal::ChannelToWifiFrequency(const std::string &channel_)
{
    uint32_t channel = ParseChannel(channel_);
    return ChannelToWifiFrequency(channel);
}

uint32_t pipedal::ChannelToWifiFrequency(uint32_t channel)
{
    if (channel > 1000) // must be a frequency.
    {
        return channel;
    }
    // 2.4GHz.
    if (channel >= 1 && channel <= 13)
    {
        return 2412 + 5 * (channel - 1);
    }
    if (channel == 14)
    {
        return 2484;
    }
    // 802.11y
    if (channel >= 131 && channel < 137)
    {
        return 3660 + (channel - 131) * 5;
    }
    if (channel >= 32 && channel <= 68 && (channel & 1) == 0)
    {
        return 5160 + (channel - 32) / 2 * 10;
    }
    if (channel == 96)
        return 5480;

    if (channel >= 100 && channel <= 196)
    {
        return 5500 + (channel - 100) / 5;
    }
    throw invalid_argument(SS("Invalid Wi-Fi channel: " << channel));
}

bool WifiConfigSettings::ValidateChannel(const std::string &countryCode, const std::string &value)
{
    if (value == "0") // = "Autoselect".
    {
        return true;
    }
    // 1) unadorned channel number 1, 2,3 &c.
    // 2) With band annotated: g1, a51.
    if (countryCode.empty())
    {
        throw std::invalid_argument("Please supply a country code.");
    }
    if (countryCode.length() != 2)
    {
        throw std::invalid_argument(SS("Invalid country code: " << countryCode));
    }
    auto regDom = getWifiRegClass(countryCode, ParseChannel(value), 40);
    if (regDom == -1)
    {
        std::vector<int32_t> valid_channels = getValidChannels(countryCode, 40);
        std::stringstream ss;
        ss << "Channel " << value << " is not permitted in the selected country.\n     Valid channels: ";
        bool first = true;
        for (auto channel : valid_channels)
        {
            if (!first)
                ss << ", ";
            first = false;
            ss << channel;
        }
        throw invalid_argument(ss.str());
    }

    ChannelToWifiFrequency(value);
    return true;
}

static const char *trueValues[]{
    "true",
    "on",
    "yes"
    "1",
    nullptr};
static const char *falseValues[]{
    "false",
    "off",
    "no",
    "0",
    nullptr};

static bool Matches(const std::string &value, const char **matches)
{
    while (*matches)
    {
        if (value == *matches)
            return true;
        ++matches;
    }
    return false;
}

static bool TryStringToBool(const std::string &value, bool *outputValue)
{
    if (Matches(value, trueValues))
    {
        *outputValue = true;
        return true;
    }
    if (Matches(value, falseValues))
    {
        *outputValue = false;
        return true;
    }
    *outputValue = false;
    return false;
}

static WifiConfigSettings::ssid_t readSsid(std::istream &ss)
{
    using ssid_t = WifiConfigSettings::ssid_t;
    char c;

    ssid_t result;
    while (ss.peek() == ' ')
    {
        ss >> c;
    }
    if (ss.peek() == '"')
    {
        ss >> c;
        while (!ss.eof() && ss.peek() != '"')
        {
            ss >> c;
            if (c == '\\')
            {
                if (ss.eof())
                {
                    break;
                }
                ss >> c;
                switch (c)
                {
                case 'n':
                    result.push_back((uint8_t)'\n');
                    break;
                case 'r':
                    result.push_back((uint8_t)'\r');
                    break;
                case 't':
                    result.push_back((uint8_t)'\t');
                    break;
                case 'b':
                    result.push_back((uint8_t)'\b');
                    break;
                default:
                    result.push_back((uint8_t)c);
                    break;
                }
            }
            else
            {
                result.push_back((uint8_t)c);
            }
        }
    }
    else
    {
        while (!ss.eof())
        {
            if (c == ':')
            {
                break;
            }
            ss >> c;
            if (c == ':')
            {
                break;
            }
            result.push_back((uint8_t)c);
        }
    }
    return result;
}
static std::vector<WifiConfigSettings::ssid_t> stringToSsidArray(const std::string &value)
{
    using ssid_t = WifiConfigSettings::ssid_t;
    std::istringstream ss(value);

    std::vector<WifiConfigSettings::ssid_t> result;
    while (true)
    {
        ssid_t ssid = readSsid(ss);
        if (ssid.size() == 0)
        {
            break;
        }
        result.push_back(ssid);
        if (ss.peek() == ':')
        {
            char c;
            ss >> c;
        }
    }
    return result;
}
void WifiConfigSettings::ParseArguments(
    const std::vector<std::string> &argv,
    HotspotAutoStartMode startMode,
    const std::string homeNetworkSsid

)
{
    this->valid_ = false;
    if (argv.size() != 4)
    {
        throw invalid_argument("Invalid number of arguments.");
    }
    WifiConfigSettings oldSettings;
    oldSettings.Load();
    this->valid_ = false;

    this->autoStartMode_ = (int16_t)startMode;
    this->enable_ = startMode != HotspotAutoStartMode::Never;
    this->homeNetwork_ = homeNetworkSsid;
    this->mdnsName_ = GetHostName();

    this->countryCode_ = argv[0];
    this->hotspotName_ = argv[1];
    this->mdnsName_ = this->hotspotName_;
    this->password_ = argv[2];
    this->channel_ = argv[3];
    this->hasPassword_ = this->password_.length() != 0;
    this->hasSavedPassword_ = oldSettings.hasSavedPassword_;

    if (!ValidateCountryCode(this->countryCode_))
    {
        throw invalid_argument("Invalid country code.");
    }
    if (this->hotspotName_.length() > 31)
        throw invalid_argument("Hotspot name is too long.");
    if (this->hotspotName_.length() < 1)
        throw invalid_argument("Hotspot name is too short.");

    if (this->password_.size() != 0 && this->password_.size() < 8)
        throw invalid_argument("Passphrase must be at least 8 characters long.");

    if (this->password_.size() == 0 && !this->hasSavedPassword_)
    {
        throw invalid_argument("Passphrase required.");
    }

    if (!ValidateChannel(this->countryCode_, this->channel_))
    {
        throw invalid_argument("Channel is not valid.");
    }

    // validate that the channel number is supported for the given country code.

    this->valid_ = true;
}

bool WifiConfigSettings::ConfigurationChanged(const WifiConfigSettings &other) const
{
    return !(
        this->valid_ == other.valid_ &&
        // this->rebootRequired_ == other.rebootRequired_ &&
        // this->enable_ == other.enable_ &&
        this->countryCode_ == other.countryCode_ &&
        this->hotspotName_ == other.hotspotName_ &&
        this->hasPassword_ == other.hasPassword_ &&
        this->password_ == other.password_ &&
        this->channel_ == other.channel_ &&

        this->homeNetwork_ == other.homeNetwork_ &&
        this->autoStartMode_ == other.autoStartMode_ &&
        this->hasSavedPassword_ == other.hasSavedPassword_);
}
bool WifiConfigSettings::operator==(const WifiConfigSettings &other) const
{
    return !ConfigurationChanged(other) && this->wifiWarningGiven_ == other.wifiWarningGiven_;
}

static bool CanSeeHomeNetwork(const std::string &home, const std::vector<std::string> &availableNetworks)
{
    for (const auto &availableNetwork : availableNetworks)
    {
        if (availableNetwork == home)
        {
            return true;
        }
    }
    return false;
}

bool WifiConfigSettings::WantsHotspot(
    bool ethernetConnected,
    const std::vector<std::string> &availableRememberedNetworks,
    const std::vector<std::string> &availableNetworks)
{
    if ((!this->valid_))
        return false;

    HotspotAutoStartMode autoStartMode = (HotspotAutoStartMode)this->autoStartMode_;

    switch (autoStartMode)
    {
    case HotspotAutoStartMode::Never:
    default:
        return false;

    case HotspotAutoStartMode::NoEthernetConnection:
        return !ethernetConnected;
    case HotspotAutoStartMode::NotAtHome:
        return !CanSeeHomeNetwork(this->homeNetwork_, availableNetworks);
    case HotspotAutoStartMode::NoRememberedWifiConections:
        return availableRememberedNetworks.size() == 0;
    case HotspotAutoStartMode::Always:
        return true;
    }
}

std::string pipedal::ssidToString(const std::vector<uint8_t> &ssid)
{
    std::stringstream s;
    for (auto v : ssid)
    {
        if (v == 0)
            break; // breaks for some arguably legal ssids, but I can live with that.
        s << (char)v;
    }
    return s.str();
}

std::vector<std::string> pipedal::ssidToStringVector(const std::vector<std::vector<uint8_t>> &ssids)
{
    std::vector<std::string> result;
    result.reserve(ssids.size());
    for (const std::vector<uint8_t> &ssid : ssids)
    {
        result.push_back(ssidToString(ssid));
    }
    return result;
}
bool WifiConfigSettings::WantsHotspot(
    bool ethernetConnected,
    const std::vector<std::vector<uint8_t>> &availableRememberedNetworks, // remembered networks that are currently visible
    const std::vector<std::vector<uint8_t>> &availableNetworks            // all visible networks.
)
{
    std::vector<std::string> sAvailableRememberedNetworks = ssidToStringVector(availableRememberedNetworks);
    std::vector<std::string> sAvailableNetworks = ssidToStringVector(availableNetworks);
    return WantsHotspot(ethernetConnected, sAvailableRememberedNetworks, sAvailableNetworks);
}

bool WifiConfigSettings::NeedsWifi() const
{
    HotspotAutoStartMode autoStartMode = (HotspotAutoStartMode)this->autoStartMode_;
    return autoStartMode != HotspotAutoStartMode::Never;
}


bool WifiConfigSettings::NeedsScan() const
{
    HotspotAutoStartMode autoStartMode = (HotspotAutoStartMode)this->autoStartMode_;
    switch (autoStartMode)
    {
    case HotspotAutoStartMode::Never:
    case HotspotAutoStartMode::NoEthernetConnection:
    case HotspotAutoStartMode::Always:
    default:
        return false;

    case HotspotAutoStartMode::NotAtHome:
    case HotspotAutoStartMode::NoRememberedWifiConections:
        return true;
    }
}
