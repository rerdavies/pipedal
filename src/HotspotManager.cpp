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

#include "HotspotManager.hpp"
#include <thread>
#include "Lv2Log.hpp"
#include "DBusLog.hpp"
#include "DBusEvent.hpp"
#include "DBusDispatcher.hpp"
#include "NetworkManagerInterfaces.hpp"
#include "ss.hpp"
#include "WifiConfigSettings.hpp"
#include "ServiceConfiguration.hpp"
#include <unordered_set>
#include <mutex>
#include <algorithm>
#include "SysExec.hpp"

using namespace pipedal;
using namespace dbus::networkmanager;
namespace fs = std::filesystem;

namespace pipedal::impl
{

#define PIPEDAL_HOTSPOT_NAME "PiPedal Hotspot"

    class HotspotManagerImpl : public HotspotManager
    {

    public:
        using ssid_t = WifiConfigSettings::ssid_t;

        HotspotManagerImpl();
        virtual ~HotspotManagerImpl() noexcept;

        virtual void Open() override;
        virtual void Reload() override;
        virtual void Close() override;

        std::vector<std::string> GetKnownWifiNetworks();

        virtual PostHandle Post(PostCallback &&fn) override;
        virtual PostHandle PostDelayed(const clock::duration &delay, PostCallback &&fn) override;
        virtual bool CancelPost(PostHandle handle) override;

        virtual void SetNetworkChangingListener(NetworkChangingListener &&listener) override;
        virtual void SetHasWifiListener(HasWifiListener &&listener) override;

    private:
        enum class State
        {
            Initial,
            WaitingForNetworkManager,
            Monitoring,
            HotspotConnecting,
            HotspotConnected,
            Error,
            Closed
        };
        void SetState(State state);
        State state = State::Initial;

        bool hasWifi = false;
        HasWifiListener hasWifiListener;
        void SetHasWifi(bool hasWifi);
        virtual bool GetHasWifi() override;

        void onClose();
        void onError(const std::string &message);
        void onInitialize();
        void onDevicesChanged();
        void onDisconnect();
        void onReload();

        void DisableHotspot();
        void EnableHotspot();
        void StartHotspot();
        void StopHotspot();
        void onStartMonitoring();
        void onAccessPointChanged();
        void onAccessPointsChanged();
        void CancelAccessPointsChangedTimer();
        void WaitForNetworkManager();
        void StartWaitForNetworkManagerTimer();
        void CancelWaitForNetworkManagerTimer();
        void UpdateNetworkManagerStatus();
        void CancelDeviceChangedTimer();
        void ReleaseNetworkManager();
        void OnEthernetStateChanged(uint32_t state);
        void OnWlanStateChanged(uint32_t state);
        std::vector<AccessPoint::ptr> GetAllAccessPoints();
        std::vector<ssid_t> GetAllAutoConnectSsids();
        std::vector<ssid_t> GetKnownVisibleAccessPoints(const std::vector<ssid_t> &allAccessPoints);

        void UpdateKnownNetworks(
            std::vector<ssid_t> &knownSsids,
            std::vector<AccessPoint::ptr> &allAccessPoints);

        void FireNetworkChanging();

        void MaybeStartHotspot();

        void StartScanTimer();
        void StopScanTimer();
        void ScanNow();

        Connection::ptr FindExistingConnection();

        Device::ptr GetDevice(uint32_t nmDeviceType);
        std::atomic<bool> closed;
        DBusDispatcher dbusDispatcher;

        std::unique_ptr<std::thread> thread;
        void ThreadProc();
        WifiConfigSettings wifiConfigSettings;
        NetworkManager::ptr networkManager;
        Device::ptr ethernetDevice;
        Device::ptr wlanDevice;
        DeviceWireless::ptr wlanWirelessDevice;
        ActiveConnection::ptr activeConnection;

        std::recursive_mutex networkChangingListenerMutex;
        NetworkChangingListener networkChangingListener;

        std::mutex knownWifiNetworksMutex;
        std::vector<std::string> knownWifiNetworks;

        bool ethernetConnected = true;
        bool wlanConnected = true;

        DBusEventHandle onDeviceAddedHandle = INVALID_DBUS_EVENT_HANDLE;
        DBusEventHandle onDeviceRemovedHandle = INVALID_DBUS_EVENT_HANDLE;
        DBusDispatcher::PostHandle devicesChangedTimerHandle = 0;
        DBusDispatcher::PostHandle networkManagerTimerHandle = 0;
        DBusDispatcher::PostHandle accessPointsChangedTimerHandle = 0;
        DBusDispatcher::PostHandle scanTimerHandle = 0;
    };
}
using namespace pipedal::impl;

HotspotManagerImpl::HotspotManagerImpl()
{
}

void HotspotManagerImpl::Open()
{

    dbusDispatcher.Run();
    dbusDispatcher.Post(
        [this]()
        {
            UpdateNetworkManagerStatus(); // waits for valid network manager state, and calls onInitialize when ready.
        });
}

void HotspotManagerImpl::onClose()
{
    this->closed = true; // avoids a memory barrier probelm.

    CancelDeviceChangedTimer();
    CancelWaitForNetworkManagerTimer();
    CancelAccessPointsChangedTimer();

    if (networkManager && activeConnection)
    {
        try
        {
            networkManager->DeactivateConnection(activeConnection->getObjectPath());
            activeConnection = nullptr;
        }
        catch (const std::exception &e)
        {
            // nothrow.
        }
    }

    ReleaseNetworkManager();

    Lv2Log::debug("HotspotManager: state=Closed");
    SetState(State::Closed);
}
void HotspotManagerImpl::onError(const std::string &message)
{
    Lv2Log::error(message);
    ReleaseNetworkManager();
    Lv2Log::debug("HotspotManager: state=Error");

    SetState(State::Error);
}

void HotspotManagerImpl::SetState(State state)
{
    this->state = state;
}
void HotspotManagerImpl::onInitialize()
{
    if (closed)
        return;
    if (state != State::Initial && state != State::WaitingForNetworkManager)
    {
        onError("Invalid state (onInitialize).");
        return;
    }
    try
    {
        wifiConfigSettings.Load();

        onStartMonitoring();
    }
    catch (const std::exception &e)
    {
        onError(SS("HotspotManager: " << e.what()));
    }
}
void HotspotManagerImpl::onDisconnect()
{
    SetHasWifi(false);
    if (state != State::Initial && state != State::WaitingForNetworkManager)
    {
        WaitForNetworkManager();
    }
}

void HotspotManagerImpl::UpdateNetworkManagerStatus()
{
    try
    {
        if (!networkManager)
        {
            networkManager = NetworkManager::Create(dbusDispatcher);
        }
        // turn on wifi if required.
        auto ethernetDevice = GetDevice(NM_DEVICE_TYPE_ETHERNET);
        auto wlanDevice = GetDevice(NM_DEVICE_TYPE_WIFI);

        SetHasWifi(!!wlanDevice);

        if (ethernetDevice && wlanDevice)
        {
            if (state == State::Initial || state == State::WaitingForNetworkManager)
            {
                CancelWaitForNetworkManagerTimer();
                this->onInitialize();
            }
            return;
        }

        // check to see whether we have BOTH an eth0 device and a wlan0 device.
    }
    catch (const std::exception &e)
    {
    }
    if (state != State::WaitingForNetworkManager)
    {
        this->WaitForNetworkManager();
    }
    else
    {
        this->StartWaitForNetworkManagerTimer();
    }
}

void HotspotManagerImpl::CancelDeviceChangedTimer()
{
    if (devicesChangedTimerHandle)
    {
        dbusDispatcher.CancelPost(devicesChangedTimerHandle);
        devicesChangedTimerHandle = 0;
    }
}
void HotspotManagerImpl::ReleaseNetworkManager()
{
    StopScanTimer();
    CancelAccessPointsChangedTimer();
    CancelWaitForNetworkManagerTimer();
    CancelDeviceChangedTimer();

    DisableHotspot();

    ethernetDevice = nullptr;
    wlanDevice = nullptr;
    wlanWirelessDevice = nullptr;
    networkManager = nullptr;

    this->onDeviceRemovedHandle = INVALID_DBUS_EVENT_HANDLE;
    this->onDeviceAddedHandle = INVALID_DBUS_EVENT_HANDLE;
    SetHasWifi(false);
}

void HotspotManagerImpl::onDevicesChanged()
{
    UpdateNetworkManagerStatus();
}
Device::ptr HotspotManagerImpl::GetDevice(uint32_t nmDeviceType)
{
    const auto &allDevices = networkManager->GetAllDevices();
    for (const auto &devicePath : allDevices)
    {
        auto device = Device::Create(dbusDispatcher, devicePath);
        if (device->DeviceType() == nmDeviceType)
        {
            return device;
        }
    }
    return nullptr;
}
void HotspotManagerImpl::OnEthernetStateChanged(uint32_t state)
{
    bool newValue = (state == 100);
    if (newValue != this->ethernetConnected)
    {
        this->ethernetConnected = newValue;
        Lv2Log::debug(SS("HotspotMonitor: ethernetConnected=" << ethernetConnected));
        FireNetworkChanging();
        MaybeStartHotspot();
    }
}
void HotspotManagerImpl::OnWlanStateChanged(uint32_t state)
{
    bool value = (state == 100);
    if (value != this->wlanConnected)
    {
        this->wlanConnected = value;
        MaybeStartHotspot();
    }
}

void HotspotManagerImpl::onStartMonitoring()
{
    try
    {
        ReleaseNetworkManager();

        this->networkManager = NetworkManager::Create(dbusDispatcher);

        if (!networkManager->WirelessEnabled() && wifiConfigSettings.NeedsWifi())
        {
            networkManager->WirelessEnabled(true);
        }

        this->onDeviceAddedHandle = this->networkManager->OnDeviceAdded.add(
            [this](const sdbus::ObjectPath &)
            {
                onDevicesChanged();
            });
        this->onDeviceRemovedHandle = this->networkManager->OnDeviceRemoved.add(
            [this](const sdbus::ObjectPath &objectPath)
            {
                if (this->ethernetDevice && this->ethernetDevice->getObjectPath() == objectPath)
                {
                    onDisconnect();
                }
                else if (this->wlanDevice && this->wlanDevice->getObjectPath() == objectPath)
                {
                    onDisconnect();
                }
            });

        ethernetDevice = GetDevice(NM_DEVICE_TYPE_ETHERNET);
        if (!ethernetDevice)
        {
            throw std::runtime_error("ethernet device not found.");
        }
        this->ethernetDevice->OnStateChanged.add(
            [this](uint32_t, uint32_t, uint32_t)
            {
                OnEthernetStateChanged(ethernetDevice->State());
            });

        wlanDevice = GetDevice(NM_DEVICE_TYPE_WIFI);
        if (!wlanDevice)
        {
            throw std::runtime_error("Wi-Fi device not found.");
        }
        wlanWirelessDevice = DeviceWireless::Create(dbusDispatcher, wlanDevice->getObjectPath());

        wlanWirelessDevice->OnAccessPointAdded.add(
            [this](const sdbus::ObjectPath &accessPoint)
            {
                onAccessPointChanged();
            });
        wlanWirelessDevice->OnAccessPointRemoved.add(
            [this](const sdbus::ObjectPath &accessPoint)
            {
                onAccessPointChanged();
            });

        this->wlanDevice->OnStateChanged.add(
            [this](uint32_t, uint32_t, uint32_t)
            {
                // Use WLAN device state, not ethernet
                OnWlanStateChanged(wlanDevice->State());
            });
        Lv2Log::debug("HotspotManager: state=Monitoring");
        SetState(State::Monitoring);

        Lv2Log::info("HotspotManager: Monitoring network status.");

        OnEthernetStateChanged(ethernetDevice->State());
        OnWlanStateChanged(wlanDevice->State());

        SetHasWifi(true);

        StartScanTimer();
        onAccessPointChanged();
    }
    catch (const std::exception &e)
    {
        Lv2Log::error(SS("HotspotManager: " << e.what()));
        WaitForNetworkManager();
    }
}

void HotspotManagerImpl::StartWaitForNetworkManagerTimer()
{

    CancelWaitForNetworkManagerTimer();
    networkManagerTimerHandle = dbusDispatcher.PostDelayed(
        std::chrono::seconds(5),
        [this]()
        {
            networkManagerTimerHandle = 0;
            this->UpdateNetworkManagerStatus();
        });
}
void HotspotManagerImpl::CancelWaitForNetworkManagerTimer()
{

    if (networkManagerTimerHandle)
    {
        dbusDispatcher.CancelPost(networkManagerTimerHandle);
        networkManagerTimerHandle = 0;
    }
}

void HotspotManagerImpl::WaitForNetworkManager()
{
    Lv2Log::debug("HotspotManager: state=WaitingForNetworkManager");
    SetState(State::WaitingForNetworkManager);
    ReleaseNetworkManager();
    StartWaitForNetworkManagerTimer();
}
void HotspotManagerImpl::onReload()
{
    if (closed)
        return;
    WifiConfigSettings oldSettings = this->wifiConfigSettings;
    this->wifiConfigSettings.Load();
    if (wifiConfigSettings == oldSettings)
    {
        return;
    }

    switch (state)
    {
    case State::Error:
        Lv2Log::error("NetworkManager is in an error state. Reboot and try again.");
        return;
    case State::Initial:
    case State::WaitingForNetworkManager:
        // ignore. new config will be picked up once initialization completes.
        return;

    case State::Closed:
        return;

    case State::HotspotConnecting:
    case State::HotspotConnected:
    default:
        // force a reload.
        StopHotspot();
        SetState(State::Monitoring);
        StartScanTimer(); // or stop it as the case may be.
        MaybeStartHotspot();
        return;
    }
}

HotspotManagerImpl::~HotspotManagerImpl()
{
    Close();
}

void HotspotManagerImpl::ThreadProc()
{
    try
    {
    }
    catch (const std::exception &e)
    {
        Lv2Log::error(SS("Hotspot Manager thread terminated. " << e.what()));
    }
}
void HotspotManagerImpl::Reload()
{
    dbusDispatcher.Post([this]()
                        { onReload(); });
}
void HotspotManagerImpl::Close()
{
    if (!closed)
    {
        closed = true;
        dbusDispatcher.Post([this]
                            { onClose(); });
        dbusDispatcher.Stop();
    }
}

HotspotManager::ptr HotspotManager::Create()
{
    return std::make_unique<HotspotManagerImpl>();
}

template <typename T>
class VectorHash
{
public:
    std::size_t operator()(const std::vector<T> &vec) const
    {
        std::size_t hash = vec.size();
        for (auto &i : vec)
        {
            hash += static_cast<size_t>(i);
            hash = ((hash >> 32) ^ hash) * 0x119de1F3;
        }
        return hash;
    }
};

struct NetworkSortRecord
{
    std::string ssid;
    bool connected = false;
    bool knownNetwork = false;
    bool visibleNetwork = false;
    uint8_t strength;
};
void HotspotManagerImpl::UpdateKnownNetworks(
    std::vector<ssid_t> &knownSsids,
    std::vector<AccessPoint::ptr> &allAccessPoints)
{
    std::map<std::string, NetworkSortRecord> map;

    for (auto &knownSsid : knownSsids)
    {
        std::string ssid = ssidToString(knownSsid);
        if (ssid.length() != 0)
        {
            NetworkSortRecord &record = map[ssid];
            record.ssid = ssid;
            record.knownNetwork = true;
        }
    }
    for (auto &accessPoint : allAccessPoints)
    {
        try
        {
            uint8_t strength = accessPoint->Strength();
            auto vSsid = accessPoint->Ssid();
            std::string ssid = ssidToString(vSsid);
            if (ssid.length() != 0)
            {
                NetworkSortRecord &record = map[ssid];
                record.ssid = ssid;
                record.visibleNetwork = true;
                record.strength = strength;
            }
        }
        catch (const std::exception &ignored)
        {
            // race to get the info before it changes. np.
        }
    }
    if (this->wlanDevice)
    {
        auto activeConnectionPath = this->wlanDevice->ActiveConnection();
        if (activeConnectionPath.length() > 2) // "/" -> no connection. Be paranoid about "".
        {
            auto activeConnection = ActiveConnection::Create(dbusDispatcher, activeConnectionPath);
            std::string activeSsid = activeConnection->Id();
            auto it = map.find(activeSsid);
            if (it != map.end())
            {
                it->second.connected = true;
            }
        }
    }
    std::vector<NetworkSortRecord> records;
    records.reserve(map.size());
    for (auto &mapEntry : map)
    {
        records.push_back(mapEntry.second);
    }
    std::sort(records.begin(), records.end(), [](const NetworkSortRecord &left, const NetworkSortRecord &right)
              {
        if (left.connected != right.connected)
        {
            return left.connected > right.connected;
        }
        if (left.knownNetwork != right.knownNetwork)
        {
            return left.knownNetwork > right.knownNetwork;
        }
        if (left.visibleNetwork != right.visibleNetwork)
        {
            return left.visibleNetwork > right.visibleNetwork;
        }
        if (left.strength != right.strength)
        {
            return left.strength > right.strength;
        }
        return false; });
    if (records.size() > 10)
    {
        records.resize(10);
    }
    std::vector<std::string> result;
    result.reserve(records.size());
    for (auto &record : records)
    {
        result.push_back(std::move(record.ssid));
    }
    {
        std::lock_guard lock{this->knownWifiNetworksMutex};
        this->knownWifiNetworks = std::move(result);
    }
}

std::vector<AccessPoint::ptr> HotspotManagerImpl::GetAllAccessPoints()
{
    std::vector<AccessPoint::ptr> accessPoints;

    for (const auto &accessPointPath : wlanWirelessDevice->AccessPoints())
    {
        auto accessPoint = AccessPoint::Create(dbusDispatcher, accessPointPath);
        accessPoints.push_back(std::move(accessPoint));
    }
    return accessPoints;
}
std::vector<std::vector<uint8_t>> HotspotManagerImpl::GetKnownVisibleAccessPoints(const std::vector<ssid_t> &allAccessPoints)
{
    std::vector<std::vector<uint8_t>> knownSsids = this->GetAllAutoConnectSsids();

    std::unordered_set<std::vector<uint8_t>, VectorHash<uint8_t>> index{knownSsids.begin(), knownSsids.end()};

    std::vector<ssid_t> result;
    std::vector<AccessPoint::ptr> accessPoints;

    for (const auto &accessPoint : allAccessPoints)
    {
        if (index.contains(accessPoint))
        {
            result.push_back(accessPoint);
        }
    }
    return result;
}

std::vector<std::vector<uint8_t>> HotspotManagerImpl::GetAllAutoConnectSsids()
{
    auto availableConnections = wlanDevice->AvailableConnections();
    std::vector<std::vector<uint8_t>> ssids;

    for (const auto &connectionPath : availableConnections)
    {
        auto connection = Connection::Create(dbusDispatcher, connectionPath);
        try
        {
            auto settings = connection->GetSettings();
            bool autoConnect = true;
            if (settings["connection"].count("autoconnect") > 0)
            {
                autoConnect = settings["connection"]["autoconnect"].get<bool>();
            }
            bool isInfrastructure =
                settings["802-11-wireless"].count("mode") > 0 && settings["802-11-wireless"]["mode"].get<std::string>() == "infrastructure";
            if (isInfrastructure && autoConnect)
            {
                std::vector<uint8_t> ssid =
                    settings["802-11-wireless"]["ssid"].get<std::vector<uint8_t>>();
                ssids.push_back(std::move(ssid));
            }
        }
        catch (const std::exception &ignored)
        {
            // not totally sure of structure of all connection types.
        }
    }
    return ssids;
}

void HotspotManagerImpl::onAccessPointsChanged()
{
    MaybeStartHotspot();
}

void HotspotManagerImpl::CancelAccessPointsChangedTimer()
{
    if (accessPointsChangedTimerHandle)
    {
        dbusDispatcher.CancelPost(accessPointsChangedTimerHandle);
        accessPointsChangedTimerHandle = 0;
    }
}
void HotspotManagerImpl::onAccessPointChanged()
{
    // coalesce large bursts of AccessPointChanged calls using a timer.
    if (!accessPointsChangedTimerHandle)
    {
        accessPointsChangedTimerHandle = dbusDispatcher.PostDelayed(
            std::chrono::milliseconds(512),
            [this]()
            {
                accessPointsChangedTimerHandle = 0;
                onAccessPointsChanged(); // coalesced handling of one or more changes.
            });
    }
}

static std::vector<std::vector<uint8_t>> GetAccessPointSsids(std::vector<AccessPoint::ptr> &accessPoints)
{
    std::vector<std::vector<uint8_t>> result;
    for (const auto &accessPoint : accessPoints)
    {
        try
        {
            result.push_back(accessPoint->Ssid());
        }
        catch (const std::exception &ignored)
        {
            // race to get a disappearing ssid. Ignore the error.
        }
    }
    return result;
}
void HotspotManagerImpl::MaybeStartHotspot()
{
    if (this->state == State::Error)
        return;
    if (this->closed)
        return;

    if (!wlanDevice || !wlanWirelessDevice)
    {
        // devices are transitioning. Do nothing.
        return;
    }

    bool wantsHotspot;

    if (wifiConfigSettings.NeedsScan())
    {

        std::vector<AccessPoint::ptr> allAccessPoints = GetAllAccessPoints();
        std::vector<std::vector<uint8_t>> allAccessPointSsids = GetAccessPointSsids(allAccessPoints);
        std::vector<std::vector<uint8_t>> connectableSsids = GetKnownVisibleAccessPoints(allAccessPointSsids); // all the ssids currently visible for which we have credentials.

        this->UpdateKnownNetworks(connectableSsids, allAccessPoints);

        wantsHotspot = this->wifiConfigSettings.WantsHotspot(
            this->ethernetConnected, connectableSsids, allAccessPointSsids);
    }
    else
    {
        wantsHotspot = this->wifiConfigSettings.WantsHotspot(
            this->ethernetConnected);
    }

    if (this->state == State::Monitoring && wantsHotspot)
    {
        StartHotspot();
    }
    else
    {
        if ((!wantsHotspot) && (this->state == State::HotspotConnected || this->state == State::HotspotConnecting))
        {
            StopHotspot();
        }
    }
}

void HotspotManagerImpl::EnableHotspot()
{
}

void HotspotManagerImpl::DisableHotspot()
{
    StopHotspot();
}

Connection::ptr HotspotManagerImpl::FindExistingConnection()
{
    if (wlanDevice)
    {
        for (const auto &connectionPath : wlanDevice->AvailableConnections())
        {
            auto connection = Connection::Create(dbusDispatcher, connectionPath);
            auto settings = connection->GetSettings();
            bool isHotspot =
                settings["802-11-wireless"].count("mode") > 0 && settings["802-11-wireless"]["mode"].get<std::string>() == "ap";
            if (isHotspot)
            {
                std::string id;
                if (settings["connection"].count("id") > 0)
                {
                    id = settings["connection"]["id"].get<std::string>();
                }
                if (id.starts_with(PIPEDAL_HOTSPOT_NAME))
                {
                    return connection;
                }
            }
        }
    }

    return nullptr;
}

void HotspotManagerImpl::StartHotspot()
{
    if (this->state == State::Monitoring)
    {
        try
        {
            ServiceConfiguration serviceConfiguration;
            serviceConfiguration.Load();

            Lv2Log::debug("HotspotManager: state=HotspotConnecting");
            SetState(State::HotspotConnecting);

            Lv2Log::info("HotspotManager: Enabling PiPedal hotspot.");

            // do it ONLY if we're in monitoring state.
            // Create a proxy for NetworkManager
            // Create connection settings for the hotspot
            std::map<std::string, std::map<std::string, sdbus::Variant>> settings;

            std::map<std::string, sdbus::Variant> &connection = settings["connection"];

            using namespace sdbus;

            connection["type"] = sdbus::Variant("802-11-wireless");
            connection["autoconnect"] = sdbus::Variant(false);
            connection["id"] = sdbus::Variant(PIPEDAL_HOTSPOT_NAME);
            connection["interface-name"] = sdbus::Variant(wlanDevice->Interface());
            connection["uuid"] = sdbus::Variant(serviceConfiguration.uuid);

            std::map<std::string, sdbus::Variant> &wireless = settings["802-11-wireless"];

            std::string ssid = this->wifiConfigSettings.hotspotName_;
            std::vector<uint8_t> vSsid{ssid.begin(), ssid.end()};
            wireless["ssid"] = sdbus::Variant(vSsid);
            wireless["mode"] = sdbus::Variant("ap");

            uint32_t iChannel = 0;
            auto channel = this->wifiConfigSettings.channel_;
            if (!channel.empty())
            {
                std::stringstream ss{channel};
                ss >> iChannel; // 0 means auto
            }
            // If a specific channel is requested, set it and derive band accordingly.
            // NM "band" values: "a" (5GHz), "bg" (2.4GHz). Channels 1-14 -> 2.4GHz; >14 -> 5GHz.
            if (iChannel != 0)
            {
                wireless["channel"] = sdbus::Variant(iChannel);
                wireless["band"] = sdbus::Variant((iChannel > 14) ? "a" : "bg");
            }

            std::map<std::string, sdbus::Variant> &wirelessSecurity = settings["802-11-wireless-security"];
            wirelessSecurity["key-mgmt"] = sdbus::Variant("wpa-psk");
            wirelessSecurity["psk"] = sdbus::Variant(wifiConfigSettings.password_);

            // IPv4 shared method: NM will configure NAT and DHCP; static address is fine.
            settings["ipv4"]["method"] = sdbus::Variant("shared");
            // For IPv6, use ignore to avoid advertising IPv6 if not needed; shared IPv6 is less common and can cause issues.
            settings["ipv6"]["method"] = sdbus::Variant("ignore");
            // If IPv6 were used, addr-gen-mode would be numeric enum; with method=ignore, omit it.

            ////////////////////////////////////////////////////////////////

            // Create connection

            settings["ipv4"]["address-data"] = sdbus::Variant(
                std::vector<std::map<std::string, sdbus::Variant>>{
                    {{"address", sdbus::Variant("192.168.60.1")},
                     {"prefix", sdbus::Variant(uint32_t(24))}}});
            // For method=shared, NM provides DHCP; explicit DHCP client options on this device are not applicable.
            // settings["ipv4"]["dhcp-options"] = sdbus::Variant(std::vector<std::string>{
            //     "option:classless-static-route,192.168.60.0/24,0.0.0.0,0.0.0.0/0,192.168.60.1"});

            // settings["ipv4"]["dhcp-options"] = sdbus::Variant(std::vector<std::string>{
            //        "option:classless-static-route,192.168.60.0/24,192.168.60.1"});

            ////////////////////////////////////////////////////////////

            // With IPv6 ignored, don't set address-data or gateway; NM may reject extraneous keys.
            std::map<std::string, sdbus::Variant> options;
            options["persist"] = sdbus::Variant("disk");

            Connection::ptr existingConnection = FindExistingConnection();
            if (existingConnection)
            {
                existingConnection->Update(settings);
                auto activeConnectionPath = networkManager->ActivateConnection(
                    existingConnection->getObjectPath(),
                    wlanDevice->getObjectPath(),
                    sdbus::ObjectPath("/"));
                this->activeConnection = ActiveConnection::Create(dbusDispatcher, activeConnectionPath);
            }
            else
            {

                // Call AddAndActivateConnection2 method to create and activate the hotspot
                sdbus::ObjectPath nullPath("/");
                auto result = networkManager->AddAndActivateConnection2(
                    settings, 
                    wlanDevice->getObjectPath(), 
                    sdbus::ObjectPath("/"),
                    options

                );
                auto connectionPath = std::get<0>(result);
                auto activeConnectionPath = std::get<1>(result);
                // auto resultArgs = std::get<2>(result);

                this->activeConnection = ActiveConnection::Create(dbusDispatcher, activeConnectionPath);
            }
            SetState(State::HotspotConnected);
            Lv2Log::info("HotspotManager: Hotspot activated.");

            FireNetworkChanging();
        }
        catch (const std::exception &e)
        {
            onError(SS("HotspotManager: Activation failed: " << e.what()));
        }
    }
    else
    {
        onError("HotspotManager: Illegal state (StartHotspot)");
    }
}
void HotspotManagerImpl::StopHotspot()
{
    // do it regardless of state.
    try
    {
        if (activeConnection)
        {
            FireNetworkChanging();
            networkManager->DeactivateConnection(activeConnection->getObjectPath());
            activeConnection = nullptr;
        }
    }
    catch (const std::exception &e)
    {
        Lv2Log::error(SS("HotspotManager: Failed to deactivate hotspot. " << e.what()));
        activeConnection = nullptr;
    }
    if (this->state == State::HotspotConnected || this->state == State::HotspotConnecting)
    {
        Lv2Log::debug("HotspotManager: state=HotspotMonitoring");
        SetState(State::Monitoring);

        Lv2Log::info("HotspotManager: PiPedal hotspot disabled.");
    }
}

static const std::chrono::seconds scanInterval{60};

void HotspotManagerImpl::StartScanTimer()
{
    StopScanTimer();
    ScanNow();
}
void HotspotManagerImpl::StopScanTimer()
{
    if (this->scanTimerHandle)
    {
        dbusDispatcher.CancelPost(this->scanTimerHandle);
        this->scanTimerHandle = 0;
    }
}
void HotspotManagerImpl::ScanNow()
{
    this->scanTimerHandle = 0;

    if (wlanWirelessDevice)
    {

        std::map<std::string, sdbus::Variant> options;

        try
        {
            if (this->wifiConfigSettings.NeedsScan())
            {

                Lv2Log::debug("Scanning");
                wlanWirelessDevice->RequestScan(options);
            }
            else
            {
                return;
            }
        }
        catch (const std::exception &e)
        {
            Lv2Log::error(SS("HotspotMonitor: Wi-Fi RequestScan failed." << e.what()));
            return;
        }

        this->scanTimerHandle = this->dbusDispatcher.PostDelayed(
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(scanInterval),
            [this]()
            {
                ScanNow();
            });
    }
}

HotspotManagerImpl::PostHandle HotspotManagerImpl::Post(PostCallback &&fn)
{
    return dbusDispatcher.Post(std::move(fn));
}
HotspotManagerImpl::PostHandle HotspotManagerImpl::PostDelayed(const clock::duration &delay, PostCallback &&fn)
{
    return dbusDispatcher.PostDelayed(delay, std::move(fn));
}
bool HotspotManagerImpl::CancelPost(PostHandle handle)
{
    return dbusDispatcher.CancelPost(handle);
}

void HotspotManagerImpl::SetHasWifiListener(HasWifiListener &&listener)
{
    std::lock_guard<std::recursive_mutex> lock{this->networkChangingListenerMutex};
    this->hasWifiListener = listener;
    if (!this->closed && this->hasWifiListener)
    {
        this->hasWifiListener(this->hasWifi);
    }
}
bool HotspotManagerImpl::GetHasWifi()
{
    std::lock_guard<std::recursive_mutex> lock{this->networkChangingListenerMutex};
    return hasWifi;
}
void HotspotManagerImpl::SetHasWifi(bool hasWifi)
{
    std::lock_guard<std::recursive_mutex> lock{this->networkChangingListenerMutex};
    if (hasWifi != this->hasWifi)
    {
        this->hasWifi = hasWifi;
        if (this->hasWifiListener)
        {
            this->hasWifiListener(this->hasWifi);
        }
    }
}

void HotspotManagerImpl::SetNetworkChangingListener(NetworkChangingListener &&listener)
{
    std::lock_guard<std::recursive_mutex> lock{this->networkChangingListenerMutex};
    this->networkChangingListener = std::move(listener);
}
void HotspotManagerImpl::FireNetworkChanging()
{
    std::lock_guard<std::recursive_mutex> lock{this->networkChangingListenerMutex};
    if (this->networkChangingListener)
    {
        this->networkChangingListener(this->ethernetConnected, !!activeConnection);
    }
}

std::vector<std::string> HotspotManagerImpl::GetKnownWifiNetworks()
{
    std::lock_guard lock(knownWifiNetworksMutex); // pushed off the service thread.
    return this->knownWifiNetworks;
}

static bool is_wireless_sysfs(const std::string &ifname)
{
    return fs::exists("/sys/class/net/" + ifname + "/wireless");
}

static std::vector<std::string> get_wireless_interfaces_sysfs()
{
    std::vector<std::string> wireless_interfaces;
    const std::string net_path = "/sys/class/net";

    try
    {
        for (const auto &entry : fs::directory_iterator(net_path))
        {
            std::string ifname = entry.path().filename();
            if (is_wireless_sysfs(ifname))
            {
                wireless_interfaces.push_back(ifname);
            }
        }
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "Error accessing " << net_path << ": " << e.what() << std::endl;
    }

    return wireless_interfaces;
}

static bool gNetworkManagerTestExecuted = false;
static bool gUsingNetworkManager = false;

static bool IsNetworkManagerRunning()
{
    if (gNetworkManagerTestExecuted)
    {
        return gUsingNetworkManager;
    }
    bool bResult = false;
    auto result = sysExecForOutput("systemctl", "is-active NetworkManager");
    if (result.exitCode == EXIT_SUCCESS)
    {
        std::string text = result.output.erase(result.output.find_last_not_of(" \n\r\t") + 1);
        if (text == "active")
        {
            bResult = true;
        }
        else if (text == "inactive")
        {
            bResult = false;
        }
        else
        {
            throw std::runtime_error(SS("UsingNetworkManager: unexpected result (" << text << ")"));
        }
        gNetworkManagerTestExecuted = true;
        gUsingNetworkManager = bResult;
        return bResult;
    }

    // does the neworkManager service path exists?
    return false;
}

bool HotspotManager::HasWifiDevice()
{
    // use procfs to decide this, as NetworkManager may not be available yet.
    if (!get_wireless_interfaces_sysfs().empty())
    {
        return true;
    }
    return false;
}
