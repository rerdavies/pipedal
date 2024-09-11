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

using namespace pipedal;
using namespace dbus::networkmanager;

namespace pipedal::impl
{

    constexpr std::string PIPEDAL_HOTSPOT_NAME = "PiPedal Hotspot";

    class HotspotManagerImpl : public HotspotManager
    {

    public:
        using ssid_t = WifiConfigSettings::ssid_t;

        HotspotManagerImpl();
        virtual ~HotspotManagerImpl() noexcept;

        virtual void Open() override;
        virtual void Reload() override;
        virtual void Close() override;

        virtual PostHandle Post(PostCallback&&fn) override;
        virtual PostHandle PostDelayed(const clock::duration&delay,PostCallback&&fn) override;
        virtual bool CancelPost(PostHandle handle) override;

        virtual void SetNetworkChangingListener(NetworkChangingListener &&listener) override;


    private:
        enum class State
        {
            Initial,
            WaitingForNetworkManager,
            Disabled,
            Monitoring,
            HotspotConnecting,
            HotspotConnected,
            Error,
            Closed
        };
        void SetState(State state);
        State state = State::Initial;

        void onClose();
        void onError(const std::string &message);
        void onInitialize();
        void onDevicesChanged();
        void onDisconnect();
        void onReload();
        void onDisableHotspot();

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
        std::vector<ssid_t> GetAutoConnectSsids();
        std::vector<ssid_t> GetKnownVisibleAccessPoints();

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

        bool ethernetConnected = true;
        bool wlanConnected = true;
        bool isAutoWlanConnectionVisible = true;

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
    SetDBusLogLevel(DBusLogLevel::None);
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
    onDisableHotspot();
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

        if (wifiConfigSettings.valid_ && wifiConfigSettings.enable_)
        {
            onStartMonitoring();
        }
        else
        {
            onDisableHotspot();
        }
    }
    catch (const std::exception &e)
    {
        onError(SS("HotspotManager: " << e.what()));
    }
}
void HotspotManagerImpl::onDisconnect()
{
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
    this->wlanConnected = (state == 100);
    Lv2Log::debug(SS("HotspotMonitor: OnWlanStateChanged"));
    MaybeStartHotspot();
}

void HotspotManagerImpl::onStartMonitoring()
{
    try
    {
        ReleaseNetworkManager();

        this->networkManager = NetworkManager::Create(dbusDispatcher);

        if (!networkManager->WirelessEnabled())
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
            throw std::runtime_error("eth0 device not found.");
        }
        this->ethernetDevice->OnStateChanged.add(
            [this](uint32_t, uint32_t, uint32_t)
            {
                OnEthernetStateChanged(ethernetDevice->State());
            });

        wlanDevice = GetDevice(NM_DEVICE_TYPE_WIFI);
        if (!wlanDevice)
        {
            throw std::runtime_error("wlan0 device not found.");
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
                OnWlanStateChanged(ethernetDevice->State());
            });
        Lv2Log::debug("HotspotManager: state=Monitoring");
        SetState(State::Monitoring);

        Lv2Log::info("HotspotManager: Monitoring network status.");

        OnEthernetStateChanged(ethernetDevice->State());
        OnWlanStateChanged(wlanDevice->State());

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
void HotspotManagerImpl::onDisableHotspot()
{
    if (this->state != State::Disabled)
    {
        ReleaseNetworkManager();

        Lv2Log::debug("HotspotManager: state=Disabled");
        SetState(State::Disabled);
        Lv2Log::info("HotspotManager: Hotspot disabled.");
    }
}
void HotspotManagerImpl::onReload()
{
    if (closed)
        return;
    WifiConfigSettings oldSettings = this->wifiConfigSettings;
    ;
    wifiConfigSettings.Load();

    switch (state)
    {
    case State::Initial:
        // ignore.
        return;
    case State::Disabled:
        if (wifiConfigSettings.valid_ && wifiConfigSettings.enable_)
        {
            onStartMonitoring();
        }
        return;
    default:
        this->onDisableHotspot();
        break;
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

static std::string ssidToString(const std::vector<uint8_t> &ssid)
{
    std::stringstream s;
    for (auto v: ssid)
    {
        if (v == 0) break; // breaks for some arguably legal ssids, but I can live with that.
        s << (char)v;
    }
    return s.str();
}

std::vector<std::vector<uint8_t>> HotspotManagerImpl::GetKnownVisibleAccessPoints()
{
    std::vector<std::vector<uint8_t>> knownSsids = this->GetAutoConnectSsids();

    std::unordered_set<std::vector<uint8_t>, VectorHash<uint8_t>> index{knownSsids.begin(), knownSsids.end()};

    std::vector<ssid_t> result; 
    for (const auto &accessPointPath : wlanWirelessDevice->AccessPoints())
    {
        auto accessPoint = AccessPoint::Create(dbusDispatcher, accessPointPath);
        auto ssid = accessPoint->Ssid();
        if (index.contains(ssid))
        {
            result.push_back(ssid);
        }
    }
    return result;
}

std::vector<std::vector<uint8_t>> HotspotManagerImpl::GetAutoConnectSsids()
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
            if (settings["connect"].count("autoconnect") > 0)
            {
                autoConnect = settings["connect"]["autoconnect"];
            }
            bool isInfrastructure =
                settings["802-11-wireless"].count("mode") > 0 && settings["802-11-wireless"]["mode"].get<std::string>() == "infrastructure";
            if (isInfrastructure && autoConnect)
            {
                std::vector<uint8_t> ssid =
                    settings["802-11-wireless"]["ssid"];
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

void HotspotManagerImpl::MaybeStartHotspot()
{
    std::vector<std::vector<uint8_t>> connectableSsids = GetKnownVisibleAccessPoints(); // all the ssids currently visible for which we have credentials.
    bool wantsHotspot =  this->wifiConfigSettings.WantsHotspot(
        this->ethernetConnected,connectableSsids);

    this->isAutoWlanConnectionVisible = connectableSsids.size() != 0;

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

            connection["type"] = "802-11-wireless";
            connection["autoconnect"] = false;
            connection["id"] = PIPEDAL_HOTSPOT_NAME;
            connection["interface-name"] = wlanDevice->Interface();
            connection["uuid"] = serviceConfiguration.uuid;

            std::map<std::string, sdbus::Variant> &wireless = settings["802-11-wireless"];

            std::string ssid = this->wifiConfigSettings.hotspotName_;
            std::vector<uint8_t> vSsid{ssid.begin(), ssid.end()};
            wireless["ssid"] = vSsid;
            wireless["mode"] = "ap";
            wireless["band"] = "a";
            wireless["band"] = "bg";

            uint32_t iChannel;
            auto channel = this->wifiConfigSettings.channel_;
            if (channel.length() != 0)
            {
                std::stringstream ss{channel};
                ss >> iChannel;
            }
            if (iChannel != 0)
            {
                wireless["channel"] = iChannel;
            }

            std::map<std::string, sdbus::Variant> &wirelessSecurity = settings["802-11-wireless-security"];
            wirelessSecurity["key-mgmt"] = "wpa-psk";
            wirelessSecurity["psk"] = wifiConfigSettings.password_;

            settings["ipv4"]["method"] = "shared";
            settings["ipv6"]["method"] = "shared";
            // settings["ipv6"]["addr-gen-mode"] = "stable-privacy";

            std::map<std::string, sdbus::Variant> options;
            options["persist"] = "disk";

            Connection::ptr existingConnection = FindExistingConnection();
            if (existingConnection)
            {
                existingConnection->Update(settings);
                auto activeConnectionPath = networkManager->ActivateConnection(
                    existingConnection->getObjectPath(),
                    wlanDevice->getObjectPath(),
                    "/");
                this->activeConnection = ActiveConnection::Create(dbusDispatcher, activeConnectionPath);
            }
            else
            {

                // Call AddAndActivateConnection2 method to create and activate the hotspot
                sdbus::ObjectPath nullPath("/");
                auto result = networkManager->AddAndActivateConnection2(
                    settings, wlanDevice->getObjectPath(), "/", options

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
            networkManager->DeactivateConnection(activeConnection->getObjectPath());
            activeConnection = nullptr;

            FireNetworkChanging();

        }
    }
    catch (const std::exception &e)
    {
        Lv2Log::error("HotspotManager: Failed to deactivate hotspot.");
        activeConnection = nullptr;
    }
    if (this->state == State::HotspotConnected || this->state == State::HotspotConnecting)
    {
        Lv2Log::info("HotspotManager: state=HotspotMonitoring");
        SetState(State::Monitoring);

        Lv2Log::info("HotspotManager: PiPedal hotspot disabled.");
    }
}

static const std::chrono::seconds scanInterval { 60};

void HotspotManagerImpl::StartScanTimer()
{
    StopScanTimer();
    ScanNow();
}
void HotspotManagerImpl::StopScanTimer()
{
    if (this->scanTimerHandle) {
        dbusDispatcher.CancelPost(this->scanTimerHandle);
        this->scanTimerHandle = 0;
    }
}
void HotspotManagerImpl::ScanNow()
{
    this->scanTimerHandle = 0;

    if (wlanWirelessDevice) {

        std::map<std::string, sdbus::Variant> options;

        try {
            Lv2Log::debug("Scanning");
            wlanWirelessDevice->RequestScan(options);
        } catch (const std::exception &e)
        {
            Lv2Log::error(SS("HotspotMonitor: Wi-Fi RequestScan failed." << e.what()));
            return;
        }

        this->scanTimerHandle = this->dbusDispatcher.PostDelayed(
            std::chrono::duration_cast<std::chrono::steady_clock::duration>(scanInterval),
            [this]() {
                ScanNow();
            }
        );
    }
}

HotspotManagerImpl::PostHandle HotspotManagerImpl::Post(PostCallback&&fn)
{
    return dbusDispatcher.Post(std::move(fn));
}
HotspotManagerImpl::PostHandle HotspotManagerImpl::PostDelayed(const clock::duration&delay,PostCallback&&fn)
{
    return dbusDispatcher.PostDelayed(delay,std::move(fn));

}
bool HotspotManagerImpl::CancelPost(PostHandle handle) {
    return dbusDispatcher.CancelPost(handle);
}

void HotspotManagerImpl::SetNetworkChangingListener(NetworkChangingListener &&listener) 
{
    std::lock_guard<std::recursive_mutex> lock { this->networkChangingListenerMutex};
    this->networkChangingListener = std::move(listener);
}
void HotspotManagerImpl::FireNetworkChanging() {
    std::lock_guard<std::recursive_mutex> lock { this->networkChangingListenerMutex};
    if (this->networkChangingListener)
    {
        this->networkChangingListener(this->ethernetConnected,!!activeConnection);
    }

}
