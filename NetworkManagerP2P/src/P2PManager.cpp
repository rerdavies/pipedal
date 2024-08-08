#ifndef USE_NETWORKMANAGER
#include "P2PManager.hpp"
#include "ss.hpp"
#include <future>
#include <chrono>
#include <poll.h>

#include "WpaInterfaces.hpp"
#include "NMP2pSettings.hpp"
#include <thread>
#include <chrono>
#include <memory>

#include "DBusDispatcher.hpp"
#include "NetworkManagerInterfaces.hpp"
#include "DBusVariantHelper.hpp"
#include "SignalHandler.hpp"

static const char CONFIG_METHODS[] = "keypad";
static const char CONFIG_CONNECT_METHOD[] = "display";

enum class p2p_status_code
{
    P2P_SC_SUCCESS = 0,
    P2P_SC_FAIL_INFO_CURRENTLY_UNAVAILABLE = 1,
    P2P_SC_FAIL_INCOMPATIBLE_PARAMS = 2,
    P2P_SC_FAIL_LIMIT_REACHED = 3,
    P2P_SC_FAIL_INVALID_PARAMS = 4,
    P2P_SC_FAIL_UNABLE_TO_ACCOMMODATE = 5,
    P2P_SC_FAIL_PREV_PROTOCOL_ERROR = 6,
    P2P_SC_FAIL_NO_COMMON_CHANNELS = 7,
    P2P_SC_FAIL_UNKNOWN_GROUP = 8,
    P2P_SC_FAIL_BOTH_GO_INTENT_15 = 9,
    P2P_SC_FAIL_INCOMPATIBLE_PROV_METHOD = 10,
    P2P_SC_FAIL_REJECTED_BY_USER = 11,
    P2P_SC_SUCCESS_DEFERRED = 12,
};

std::string InvitationStatusToString(int32_t status_code_)
{
    if (status_code_ == -1)
    {
        return "FAIL";
    }
    p2p_status_code status_code = (p2p_status_code)status_code_;
#define STATUS_CODE_CASE(x)  \
    case p2p_status_code::x: \
        return #x;
    switch (status_code)
    {
        STATUS_CODE_CASE(P2P_SC_SUCCESS)
        STATUS_CODE_CASE(P2P_SC_FAIL_INFO_CURRENTLY_UNAVAILABLE)
        STATUS_CODE_CASE(P2P_SC_FAIL_INCOMPATIBLE_PARAMS)
        STATUS_CODE_CASE(P2P_SC_FAIL_LIMIT_REACHED)
        STATUS_CODE_CASE(P2P_SC_FAIL_INVALID_PARAMS)
        STATUS_CODE_CASE(P2P_SC_FAIL_UNABLE_TO_ACCOMMODATE)
        STATUS_CODE_CASE(P2P_SC_FAIL_PREV_PROTOCOL_ERROR)
        STATUS_CODE_CASE(P2P_SC_FAIL_NO_COMMON_CHANNELS)
        STATUS_CODE_CASE(P2P_SC_FAIL_UNKNOWN_GROUP)
        STATUS_CODE_CASE(P2P_SC_FAIL_BOTH_GO_INTENT_15)
        STATUS_CODE_CASE(P2P_SC_FAIL_INCOMPATIBLE_PROV_METHOD)
        STATUS_CODE_CASE(P2P_SC_FAIL_REJECTED_BY_USER)
        STATUS_CODE_CASE(P2P_SC_SUCCESS_DEFERRED)

    default:
        return "UNKNOWN_STATUS";
    }
#undef STATUS_CODE_CASE
}
class P2PManagerImpl : public P2PManager
{
public:
    P2PManagerImpl() : dispatcher(true) {}
    virtual ~P2PManagerImpl();

    bool Init() override;
    void Run() override;
    void Stop() override;
    bool IsFinished() override;
    void Wait() override;
    bool ReloadRequested() override;
    bool TerminatedNormally() override;

private:
    using clock = std::chrono::steady_clock;

    void OnServiceDiscoveryRequest(const std::map<std::string, sdbus::Variant> &sd_request);

    void CancelDelayedConnectDevice();
    void DelayedConnectDevice(const sdbus::ObjectPath&peer_object);
    void OnInvitationResult(const sdbus::ObjectPath &objectPath, const std::map<std::string, sdbus::Variant> &invite_result);
    sdbus::ObjectPath GetActivePersistentGroup();
    void StartListen();
    void SignalStop();
    void SignalHangup();
    void ExecuteOrderlyShutdown();
    void ShutdownStage2();
    void TurnOnWifi();
    void UnInitialize();
    void StartWaitUntilDone();
    void PostWaitUntilDone();
    void MaybeRemoveGroup();
    void CancelMaybeRemoveGroup();
    void MaybeCancel();

    void StartGroup();
    bool HasPersistentGroup();
    void AddExistingInterfaces();
    void RemoveExistingInterfaces();
    void HandleInterfaceAdded(const std::string &objectPath);
    void HandleInterfaceRemoved(const std::string &objectPath);
    void InitWpaP2PDevice(WpaP2PDevice::ptr&&device);
    void InitWpaP2PGoDevice(WpaP2PDevice::ptr&&device);
    void ConnectDevice(const sdbus::ObjectPath &peer_object, bool enterRequest);
    bool HasClient();
    std::string UpnpServiceName();
    void StartServiceBroadcast();
    void StopServiceBroadcast();

    std::atomic<bool> reloadRequested{false};
    std::atomic<bool> signalStop{false};
    std::atomic<bool> normalTermination{false};
    bool signalStopStarted = false;
    bool shutdownExecuted = false;
    bool waited = false;
    bool serviceBroadcastStarted = false;
    DBusDispatcher dispatcher;
    DBusDispatcher::PostHandle maybeCloseHandle = 0;
    DBusDispatcher::PostHandle delayedConnectHandle = 0;
    std::mutex postMutex;
    std::atomic<bool> stopping;
    std::vector<std::string> pendingAddedInterfaces;

    using PostCallback = DBusDispatcher::PostCallback;
    using PostHandle = DBusDispatcher::PostHandle;
    void Post(PostCallback &&fn);

    WpaSupplicant::ptr wpaSupplicant;
    WpaP2PDevice::ptr p2pDevice;
    WpaP2PDevice::ptr p2pGoDevice;
    WpaWPS::ptr wpsDevice;
    WpaGroup::ptr wpaGroup;
    std::vector<WpaPeer::ptr> connectedPeers;

    sdbus::ObjectPath persistentGroupPath;
    P2pSettings settings{};

    std::string oldWpasDebugLevel;
    clock::time_point stage2Timeout;
    clock::time_point waitUntilDoneTimeout;
};

P2PManager::ptr P2PManager::Create()
{
    return std::make_unique<P2PManagerImpl>();
}

P2PManagerImpl::~P2PManagerImpl()
{
    CancelDelayedConnectDevice();
    Stop();
}

static const char hexDigits[] = "0123456789abcdef";

static std::string AddressToString(const std::vector<uint8_t> &address)
{
    std::stringstream ss;
    bool firstTime = true;
    for (auto byte : address)
    {
        if (!firstTime)
        {
            ss << ":";
        }
        firstTime = false;
        ss << hexDigits[(byte >> 4) & 0x0F] << hexDigits[byte & 0x0F];
    }
    return ss.str();
}

void P2PManagerImpl::OnInvitationResult(const sdbus::ObjectPath &objectPath, const std::map<std::string, sdbus::Variant> &invite_result)
{
    int32_t status;
    try
    {
        status = invite_result.at("status");
    }
    catch (const std::exception &)
    {
        status = -1;
    }
    std::string strStatus = InvitationStatusToString(status);
    if (status == 0)
    {
        LogDebug(objectPath, "OnInvitationResult", strStatus);
    }
    else
    {
        LogInfo(objectPath, "OnInvitationResult", strStatus);
    }
}

void P2PManagerImpl::RemoveExistingInterfaces()
{
    try
    {
        while (true)
        {
            if (!this->wpaSupplicant)
                break;

            auto interfaces = this->wpaSupplicant->Interfaces();
            if (interfaces.size() == 0)
                break;

            wpaSupplicant->RemoveInterface(interfaces[0]);
        }
    }
    catch (const std::exception &e)
    {
        LogWarning("P2PManagerImpl", "RemoveExistingInterfaces", e.what());
    }
}
bool P2PManagerImpl::Init()
{
    settings.Load();
    if (!settings.valid())
    {
        return false;
    }

    LogInfo(
        "P2PManager",
        "Init",
        SS(
            "Configuration:"
            << " " << settings.country()
            << " " << settings.wlan()
            << " " << settings.device_name()
            << " " << settings.web_server_port()
            << " " << settings.listen_channel()
            << " " << settings.op_channel()));
    SignalHandler::HandleSignal(
        SignalHandler::Interrupt,
        [this]()
        {
            this->normalTermination = true; // don't restart service
            this->SignalStop();
        });
    SignalHandler::HandleSignal(
        SignalHandler::Terminate,
        [this]()
        {
            this->normalTermination = true; // don't restart service
            this->SignalStop();
        });
    SignalHandler::HandleSignal(
        SignalHandler::Hangup,
        [this]()
        {
            this->normalTermination = false; // don't restart service
            this->SignalStop(); // we can't restart, but at lease we can do an orderly shutdown.
            // disable this, because sdbus::dispatcher doesn't seem to be able to handle a restart.
            // this->SignalHangup();
        });

#ifdef DEBUG
    dispatcher.Connection().setMethodCallTimeout(500 * 1000 * 1000);
#endif

    for (int retry = 0; retry < 4; ++retry) // defend against slow starup of wpa_supplicant
    {
        try
        {
            this->wpaSupplicant = WpaSupplicant::Create(dispatcher.Connection());
            this->wpaSupplicant->DebugLevel();
            if (retry != 0)
            {
                LogInfo("P2PManager", "Init", "wpa_supplicant DBus connection established.");
            }
            break;
        }
        catch (std::exception &e)
        {
            if (retry == 0)
            {
                LogInfo("P2PManager", "Init", "Waiting for wpa_supplicant DBus connection.");
            }
            this->wpaSupplicant = nullptr;
            std::this_thread::sleep_for(std::chrono::seconds(5));
        }
    }
    if (!this->wpaSupplicant)
    {
        throw std::runtime_error("wpa_supplicant service is not reachable by DBus.");
    }

    std::string wpaLogLevel = "info";
    switch (GetDBusLogLevel())
    {
    default:
    case DBusLogLevel::Info:
        wpaLogLevel = "info";
        break;
    case DBusLogLevel::Debug:
        wpaLogLevel = "debug";
        break;
    case DBusLogLevel::Warning:
        wpaLogLevel = "warning";
        break;
    case DBusLogLevel::Error:
        wpaLogLevel = "error";
        break;
    case DBusLogLevel::Trace:
        wpaLogLevel = "debug";
        break;
    }
    oldWpasDebugLevel = wpaSupplicant->DebugLevel();
    wpaSupplicant->DebugLevel(wpaLogLevel);

    TurnOnWifi();

    this->wpaSupplicant->OnInterfaceAdded.add(
        [this](const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties)
        {
            this->HandleInterfaceAdded(path);
        });
    this->wpaSupplicant->OnInterfaceRemoved.add(
        [this](const sdbus::ObjectPath &path)
        {
            this->HandleInterfaceRemoved(path);
        });

    RemoveExistingInterfaces();
    if (wpaSupplicant->Interfaces().size() == 0)
    {
        std::map<std::string, sdbus::Variant> createArgs;
        createArgs["Ifname"] = settings.wlan();
        createArgs["Driver"] = settings.driver();
        createArgs["ConfigFile"] = settings.wpas_config_file();
        wpaSupplicant->CreateInterface(createArgs);
    }
    return true;
}

void P2PManagerImpl::AddExistingInterfaces()
{
    if (!this->p2pDevice)
    {
        for (const auto &objectPath : this->wpaSupplicant->Interfaces())
        {
            HandleInterfaceAdded(objectPath);
        }
    }
}

void P2PManagerImpl::HandleInterfaceRemoved(const std::string &objectPath)
{
    bool deleted = false;

    // removal from pending list can be done in a handler.
    for (auto i = this->pendingAddedInterfaces.begin(); i != pendingAddedInterfaces.end(); ++i)
    {
        if (*i == objectPath)
        {
            pendingAddedInterfaces.erase(i);
            return;
        }
    }
    // removal of existing objects needs to be posted.
    Post([this, objectPath]()
         {
        if (this->p2pDevice && this->p2pDevice->getObjectPath() == objectPath)
        {
            CancelDelayedConnectDevice();
            this->p2pDevice = nullptr;
            this->wpsDevice = nullptr;
            dispatcher.SignalStop(
                [this]()
            {
                UnInitialize(); // shut down, wpa_supplicant has stopped.
            });
        }
        if (this->p2pGoDevice && this->p2pGoDevice->getObjectPath() == objectPath)
        {
            this->wpaGroup = nullptr;
            this->p2pGoDevice = nullptr;
            StartListen();
        } });
}

void P2PManagerImpl::HandleInterfaceAdded(const std::string &objectPath)
{

    // Create the objects in the callback, but defer processing.
    // Guard against the interface being synchronously removed before the async post handler excecutes./
    this->pendingAddedInterfaces.push_back(objectPath.c_str());
    {
        std::string msg = SS("Device added: " << objectPath.c_str());
        LogDebug("P2PManagerImpl", "HandleInterfaceAdded", msg);
    }

    Post([this,
          objectPath]() mutable
         {
        // check to see whether the new interface  was removed from pending list.
        auto found = std::find(pendingAddedInterfaces.begin(),pendingAddedInterfaces.end(),objectPath);
        if (found != pendingAddedInterfaces.end())
        {
            pendingAddedInterfaces.erase(found);
            try {
                auto p2pDevice = WpaP2PDevice::Create(dispatcher.Connection(), objectPath);

                LogDebug(objectPath,"HandleInterfaceAdded",SS("Role: " << p2pDevice->Role()));
                if (p2pDevice->Role() == "device" && ((!this->p2pDevice) || this->p2pDevice->getObjectPath() != objectPath))
                {
                    LogDebug("P2PManagerImpl", "HandleInterfaceAdded", "P2P device found.");
                    InitWpaP2PDevice(std::move(p2pDevice));
                }
                else if (p2pDevice->Role() == "GO" && ((!this->p2pGoDevice) || this->p2pGoDevice->getObjectPath() != objectPath))
                {
                    LogDebug("P2PManagerImpl", "HandleInterfaceAdded", "P2P GO group interface found.");

                    InitWpaP2PGoDevice(std::move(p2pDevice));
                }
            } catch (const std::exception&e)
            {
                LogError("P2PManagerImpl","HandleInterfaceAdded",SS("Failed to add " << objectPath.c_str() << " (" << e.what() << ")"));
            }
        } });
}

void P2PManagerImpl::InitWpaP2PGoDevice(WpaP2PDevice::ptr&&device)
{
    this->p2pGoDevice = std::move(device);
    if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
        auto deviceConfig = p2pGoDevice->P2PDeviceConfig();
        std::stringstream s;
        s << deviceConfig;
        LogTrace("P2PManagerImpl", "InitWpaP2PGoDevice", SS("GO Device config: " << s.str()));
    }
    p2pGoDevice->ConfigMethods(CONFIG_METHODS);
    LogTrace("P2PManagerImpl", "InitWpaP2PGoDevice", SS("ConfigMethods: " << p2pGoDevice->ConfigMethods()));

    if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
        auto deviceConfig = p2pGoDevice->P2PDeviceConfig();
        std::stringstream s;
        s << deviceConfig;
        LogTrace("P2PManager", "InitWpaP2PGoDevice", SS("P2PDeviceConfig: " << s.str()));
    }


    p2pGoDevice->OnInvitationResult.add(
        [this](const std::map<std::string, sdbus::Variant> &invite_result)
        {
            OnInvitationResult(p2pDevice->getObjectPath(), invite_result);
        });

    p2pGoDevice->OnStaAuthorized.add(
        [this](const std::string &name)
        {
            LogInfo(
                p2pGoDevice->getObjectPath(),
                "OnStationAuthorized",
                SS("Station authorized: " << name));
        });
    p2pGoDevice->OnStaDeauthorized.add(
        [this](const std::string &name)
        {
            LogInfo(
                p2pGoDevice->getObjectPath(),
                "OnStationDeauthorized",
                SS("Station deauthorized: " << name));
        });

    p2pGoDevice->OnWpsFailed.add(
        [this](const std::string &name, const std::map<std::string, sdbus::Variant> &args)
        {
            {
                std::stringstream ss;
                ss << name << " " << args << " ";
                LogError("P2PManagerImp", "OnWpsFailed", ss.str());
            }
            try
            {
                this->p2pGoDevice->Cancel();
            }
            catch (const std::exception &)
            {
            }
            MaybeRemoveGroup();
        });
    p2pGoDevice->OnGONegotiationFailure.add(
        [this](const std::map<std::string, sdbus::Variant> &info)
        {
        std::stringstream  ss;
        ss << info;
        LogError(p2pGoDevice->getObjectPath().c_str(),"OnGoNegotiationFailure",ss.str()); });
    p2pGoDevice->OnGroupFormationFailure.add(
        [this](const std::string &message)
        {
            std::stringstream ss;
            ss << message;
            LogError(p2pGoDevice->getObjectPath().c_str(), "OnGroupFormationFailure", ss.str());
            MaybeRemoveGroup();
        });

    p2pGoDevice->OnStationAdded.add(
        [this](const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties) {

        });
    p2pGoDevice->OnStationRemoved.add(
        [this](const sdbus::ObjectPath &)
        {
            MaybeRemoveGroup();
        });
    LogTrace("P2PManagerImpl", "InitWpaP2PGoDevice", SS("GoCountry: " << p2pGoDevice->Country()));

    p2pGoDevice->OnProvisionDiscoveryRequestDisplayPin.add(
        [this](const sdbus::ObjectPath &peer_object, const std::string &pin)
        {
            LogTrace("P2PManagerImpl", "InitWpaP2PGoDevice",
                     SS("GO OnProvisionDiscoveryRequestDisplayPin - " << peer_object.c_str() << " pin: " << pin));
        });
}

bool P2PManagerImpl::HasClient()
{
    return this->p2pDevice->Stations().size() != 0;
}

void P2PManagerImpl::CancelDelayedConnectDevice()
{
    if (delayedConnectHandle) {
        dispatcher.CancelPost(delayedConnectHandle);
        delayedConnectHandle = 0;
    }
    
}
void P2PManagerImpl::DelayedConnectDevice(const sdbus::ObjectPath&peer_object)
{
    delayedConnectHandle = 0;
    if (dispatcher.IsStopping()) return; // brief race window during shutdown.
    if (!p2pGoDevice)
    {
        ConnectDevice(peer_object,false);
    } else {
        delayedConnectHandle = dispatcher.PostDelayed(
            std::chrono::milliseconds(500),
            [this,peer_object]()
            {
                DelayedConnectDevice(peer_object);
            }
        );
    }
}

void P2PManagerImpl::ConnectDevice(const sdbus::ObjectPath &peer_object, bool enterRequest)
{
    // The implemented behaviour: if a second connection request comes in, disconnect the first connection
    // before establishing the second connection.
    //
    // I've tried every way I can think of to allow a second group member, without success.
    // if we received a connection request with a a currently active group, 
    // disconnect the group, asynchronously, waiting for the group intefrace to close, and THEN issue the connect call.
#if 1
    CancelDelayedConnectDevice();
    if (this->p2pGoDevice)
    {
        try {
            this->p2pGoDevice->Disconnect();
        } catch (const std::exception &e)
        {
            LogDebug("P2PManager","ConnectDevice",SS("Disconnect failed. " << e.what()));
            try {
                this->wpaSupplicant->RemoveInterface(p2pGoDevice->getObjectPath()); // draconian!
            } catch (const std::exception&e)
            {
                LogDebug("P2PManager","ConnectDevice",SS("RemoveInterface failed. " << e.what()));

            }
        }
        DelayedConnectDevice(peer_object);
        return;
    }
#endif
    
#if 1
    try
    {
        MaybeCancel();
        if (!this->p2pGoDevice)
        {
            auto groups = p2pDevice->PersistentGroups();
            if (false && groups.size() >= 2)
            {
                // p2pDevice, groups[1] -> OnInvitationResult(P2P_SC_FAIL_UNKNOWN_GROUP)
                auto groupPath = groups[1];
                // start a new group using an existing persistent group.
                std::map<std::string, sdbus::Variant> inviteArgs;
                inviteArgs["peer"] = peer_object;
                inviteArgs["persistent_group_object"] = groupPath;

                p2pDevice->Invite(inviteArgs);
            } else {
                // 2nd connection, join = true: EAP auth failure, PIN_NEEDED.
                // 2nd connection, join = false:  immedate failure with device busy.
                std::map<std::string, sdbus::Variant> connectArgs;
                connectArgs["peer"] = peer_object;
                connectArgs["persistent"] = true;
                connectArgs["frequency"] = (int32_t)this->settings.op_frequency();
                connectArgs["go_intent"] = (int32_t)this->settings.p2p_go_intent();
                connectArgs["wps_method"] = "display",
                connectArgs["pin"] = this->settings.pin();
                connectArgs["authorize_only"] = false;
                connectArgs["join"] = false; //false; // HasClient();

                this->p2pDevice->Connect(connectArgs);
                    // // start a new group.
                    // std::map<std::string, sdbus::Variant> inviteArgs;
                    // inviteArgs["peer"] = peer_object;
                    // sdbus::ObjectPath groupPath;
                    // auto persistentGroups = p2pDevice->PersistentGroups();
                    // if (persistentGroups.size() >= 2)
                    // {
                    //     groupPath = persistentGroups[0];

                    //     // inviteArgs["persistent_group_object"] = groupPath;
                    //     p2pDevice->Invite(inviteArgs);
                    // } else {
                    //     throw std::runtime_error("NOT IMPLEMENTED");
                    // }
            }
        }
        else
        {
            // p2pDevice, Connect,join=true. Bizarre extra group devices.  Connected device gets orphaned. borked.
            // std::map<std::string, sdbus::Variant> connectArgs;
            // connectArgs["peer"] = peer_object;
            // connectArgs["persistent"] = true;
            // connectArgs["frequency"] = (int32_t)this->settings.op_frequency();
            // connectArgs["go_intent"] = (int32_t)this->settings.p2p_go_intent();
            // connectArgs["wps_method"] = "display",
            // connectArgs["pin"] = this->settings.pin();
            // connectArgs["authorize_only"] = false;
            // connectArgs["join"] = true; // HasClient();

            // this->p2pDevice->Connect(connectArgs);
            // join the existing group.

            // 2pGoDevice Join("") -> NO_INFO error.
            // p2pDevice Join("") -> Failed to join an active group. looking for wlan0 interfacpe when it should be looking for p2p-wlan-N
            // p2pDevice join("GetActivePersistentGroup") -> Failed to create interface p2p-wlan0-2: -16 (Device or resource busy)

            std::map<std::string, sdbus::Variant> inviteArgs;
            inviteArgs["peer"] = peer_object;
            // inviteArgs["persistent_group_object"] = GetActivePersistentGroup();
            p2pGoDevice->Invite(inviteArgs);
        }
    }
    catch (const std::exception &e)
    {
        LogError("P2pManager", "ConnectDevice", e.what());
        return;
    }
#else
    std::map<std::string, sdbus::Variant> inviteArgs;
    inviteArgs["peer"] = peer_object;
    inviteArgs["persistent_group_object"] = this->persistentGroupPath;
    p2pDevice->Invite(inviteArgs);
#endif
}

static bool ValidateSetting(const std::map<std::string, sdbus::Variant> &properties, const std::string &key, int32_t value)
{
    auto iter = properties.find(key);
    if (iter == properties.end())
        return false;
    if (!iter->second.containsValueOfType<int32_t>())
        return false;
    return iter->second.get<int32_t>() == value;
}
static bool ValidateSetting(const std::map<std::string, sdbus::Variant> &properties, const std::string &key, const std::string &value)
{
    auto iter = properties.find(key);
    if (iter == properties.end())
        return false;
    if (!iter->second.containsValueOfType<std::string>())
        return false;
    return iter->second.get<std::string>() == value;
}
bool P2PManagerImpl::HasPersistentGroup()
{
    auto persistentGroups = p2pDevice->PersistentGroups();
    return persistentGroups.size() != 0;
}

void P2PManagerImpl::InitWpaP2PDevice(WpaP2PDevice::ptr&&device)
{
    p2pDevice = std::move(device);
    this->wpsDevice = WpaWPS::Create(dispatcher.Connection(), p2pDevice->getObjectPath());


    p2pDevice->Country(settings.country());

    std::map<std::string, sdbus::Variant> deviceConfig;
    deviceConfig["DeviceName"] = SS(settings.device_name());
    deviceConfig["PrimaryDeviceType"] = std::vector<uint8_t>{0x00, 0x01, 0x00, 0x50, 0xF2, 0x04, 0x00, 0x01};
    deviceConfig["GOIntent"] = (uint32_t)15;
    deviceConfig["ListenChannel"] = (uint32_t)settings.listen_channel();
    deviceConfig["ListenRegClass"] = (uint32_t)settings.listen_reg_class();
    deviceConfig["OperChannel"] = (uint32_t)settings.op_channel();
    deviceConfig["OperRegClass"] = (uint32_t)settings.op_reg_class();

    // 
    // deviceConfig["ListenChannel"] = (uint32_t)settings.listen_channel();

    deviceConfig["SsidPostfix"] = SS("-" << settings.device_name());
    deviceConfig["IpAddrGo"] = std::vector<uint8_t>{172, 23, 0, 2};
    deviceConfig["IpAddrMask"] = std::vector<uint8_t>{255, 255, 0, 0};
    deviceConfig["IpAddrStart"] = std::vector<uint8_t>{172, 23, 0, 4};
    deviceConfig["IpAddrEnd"] = std::vector<uint8_t>{172, 23, 0, 9};
    // deviceConfig["IntraBss"] = false;
    deviceConfig["PersistentReconnect"] = true;
    // per_sta_psk
    //p2pDevice->MeshFwding("0");
    p2pDevice->P2PDeviceConfig(deviceConfig);
    if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
        auto deviceConfig = p2pDevice->P2PDeviceConfig();
        std::stringstream s;
        s << deviceConfig;
        LogTrace("P2PManager", "InitWpaP2PDevice", SS("P2PDeviceConfig: " << s.str()));
    }

    LogDebug("P2PManager", "InitWpaP2PDevice", SS("ConfigMethods" << p2pDevice->ConfigMethods()));
    p2pDevice->ConfigMethods(CONFIG_METHODS);
    // p2pDevice->P2pGoHt40(settings.p2p_go_ht40() ? "1" : "0");

    std::string groupName = p2pDevice->Group().c_str();

    auto t = wpsDevice->DeviceType();

    wpsDevice->DeviceName(settings.device_name());
    wpsDevice->Manufacturer(settings.manufacturer());
    wpsDevice->ModelName(settings.model_name());
    wpsDevice->ModelNumber(settings.model_number());
    wpsDevice->SerialNumber(settings.serial_number());
    wpsDevice->DeviceType(settings.device_type());


    p2pDevice->OnServiceDiscoveryRequest.add(
        [this](const std::map<std::string, sdbus::Variant> &sd_request)
        {
            OnServiceDiscoveryRequest(sd_request);
        });
    p2pDevice->OnInvitationResult.add(
        [this](const std::map<std::string, sdbus::Variant> &invite_result)
        {
            OnInvitationResult(p2pDevice->getObjectPath(), invite_result);
        });
    p2pDevice->OnGroupStarted.add(
        [this](const std::map<std::string, sdbus::Variant> &args)
        {
            try
            {
                LogInfo("P2PManager", "OnGroupStarted", "GroupStarted");

                const sdbus::Variant &vGroup = args.at("group_object");
                sdbus::ObjectPath group = vGroup;
                if (!group.empty())
                {
                    wpaGroup = WpaGroup::Create(dispatcher.Connection(), group);
                    wpaGroup->OnPeerJoined.add(
                        [this](const sdbus::ObjectPath &path)
                        {
                            Post(
                                [this, path]()
                                {
                                    try
                                    {
                                        LogDebug("P2PManager", "OnPeerJoined", path.c_str());
                                        this->connectedPeers.push_back(WpaPeer::Create(dispatcher.Connection(), path));
                                    }
                                    catch (const std::exception &e)
                                    {
                                        LogError("P2PManager", "OnPeerJoined", e.what());
                                    }
                                });
                        });
                    wpaGroup->OnPeerDisconnected.add(
                        [this](const sdbus::ObjectPath &path)
                        {
                            Post(
                                [this, path]()
                                {
                                LogDebug("P2PManager","OnPeerDisconnected",path.c_str());
                                this->connectedPeers.push_back(WpaPeer::Create(dispatcher.Connection(),path));
                                for (auto iter = this->connectedPeers.begin(); iter != this->connectedPeers.end(); iter++)
                                {
                                    if ((*iter)->getObjectPath() == path)
                                    {
                                        this->connectedPeers.erase(iter);
                                        break;
                                    }
                                } });
                        });
                }
            }
            catch (const std::exception &e)
            {
                LogError("InitWpaP2PDevice", "OnGroupAdded", e.what());
            }
        });
    p2pDevice->OnGONegotiationFailure.add(
        [this](const std::map<std::string, sdbus::Variant> &info)
        {
            std::stringstream ss;
            ss << info;
            LogError(p2pDevice->getObjectPath().c_str(), "OnGoNegotiationFailure", ss.str());
        });
    p2pDevice->OnGroupFormationFailure.add(
        [this](const std::string &reason)
        {
            LogError(p2pDevice->getObjectPath().c_str(), "OnGroupFormationFailure", reason);
        });

    p2pDevice->OnGroupFinished.add([this](const std::map<std::string, sdbus::Variant> &properties)
                                   {
        LogInfo("P2PManager","OnGroupFinished","");
        wpaGroup = nullptr; });

    p2pDevice->OnGroupFormationFailure.add(
        [this](const std::string &reason)
        {
            Post([this]()
                 {
                if (this->p2pDevice)
                {
                    try {
                        StartListen();
                    } catch (const std::exception&e)
                    {
                        LogError("P2PManager","OnGroupFormationFailure", SS("Listen failed: " << e.what()));
                    }
                } });
        });
    p2pDevice->OnGONegotiationFailure.add(
        [this](const std::map<std::string, sdbus::Variant> &info)
        {
            Post([this]()
                 {
                if (this->p2pDevice)
                {
                    try {
                        StartListen();
                    } catch (const std::exception&e)
                    {
                        LogError("P2PManager","OnGONegotiationFailure", SS("Listen failed: " << e.what()));
                    }
                } });
        });

    p2pDevice->OnProvisionDiscoveryEnterPin.add(
        [this](const sdbus::ObjectPath &peer_object)
        {
            // old android device picks wrong method. pretend they asked for OnProvisionDiscoveryRequestDisplayPin instead
            LogDebug(p2pDevice->getObjectPath(), "OnProvisionDiscoveryEnterPin",
                     peer_object.c_str());

            Post(
                [this, peer_object]()
                {
                    if (!this->p2pDevice)
                        return;

                    ConnectDevice(peer_object, true);
                });
        });

    p2pDevice->OnProvisionDiscoveryRequestDisplayPin.add(
        [this](const sdbus::ObjectPath &peer_object, const std::string &pin)
        {
            LogDebug(p2pDevice->getObjectPath(), "OnProvisionDiscoveryRequestDisplayPin",
                     SS(peer_object.c_str()));

            Post(
                [this, peer_object]()
                {
                    if (!this->p2pDevice)
                        return;

                    ConnectDevice(peer_object, false);
                });
        });

    if (!HasPersistentGroup() || settings.auth_changed())
    {
        if (settings.auth_changed())
        {
            LogInfo("P2PManagerImpl", "InitWpaP2PDevice", "Auth has changed. Resetting persistent groups");
        }
        p2pDevice->RemoveAllPersistentGroups();

        std::map<std::string, sdbus::Variant> persistentGroupAddParams;
        persistentGroupAddParams["bssid"] = settings.bssid();
        persistentGroupAddParams["ssid"] = settings.device_name();
        persistentGroupAddParams["psk"] = settings.pin();
        persistentGroupAddParams["mode"] = (uint32_t)3; //"3";
        sdbus::ObjectPath persistentGroupPath = p2pDevice->AddPersistentGroup(persistentGroupAddParams);
        this->persistentGroupPath = persistentGroupPath;
        if (settings.auth_changed())
        {
            settings.auth_changed(false);
            settings.Save();
        }
    }
    else
    {
        this->persistentGroupPath = p2pDevice->PersistentGroups()[0];
    }

    StartListen();
    StartServiceBroadcast();

    LogInfo(this->p2pDevice->getObjectPath(), "InitWpaP2PDevice", "Listening");

    dispatcher.Post(
        [this]()
        {
            // StartGroup();
        });
}

void P2PManagerImpl::Post(PostCallback &&fn)
{
    dispatcher.Post(std::move(fn));
}

void P2PManagerImpl::Run()
{

    dispatcher.Run();
}

void P2PManagerImpl::ExecuteOrderlyShutdown()
{
    dispatcher.Post(
        [this]()
        {
            if (this->shutdownExecuted)
                return;
            this->shutdownExecuted = true;

            StopServiceBroadcast();

            CancelMaybeRemoveGroup();

            if (this->p2pDevice)
            {
                // no parameters cancels Extended listen.
                std::map<std::string, sdbus::Variant> listenArgs;
                this->p2pDevice->ExtendedListen(listenArgs);
            }

            if (this->p2pGoDevice)
            {
                try
                {
                    this->p2pGoDevice->Disconnect();
                }
                catch (const std::exception &e)
                {
                    LogWarning("P2PManagerImpl", "Stop", SS("Can't disconnect " << this->p2pGoDevice->Ifname()));
                }
            }
            stage2Timeout = clock::now() + std::chrono::duration_cast<clock::duration>(std::chrono::milliseconds(1500));
            ShutdownStage2();
        });
}

void P2PManagerImpl::ShutdownStage2()
{
    if (clock::now() > stage2Timeout)
    {
        LogError("P2PManagerImpl", "ShutdownStage2", "Stage2 shutdown timeout exipred.");
    }
    else
    {
        // wait until the Disconnect has completed.
        if (this->p2pGoDevice)
        {
            dispatcher.PostDelayed(std::chrono::milliseconds(50),
                                   [this]()
                                   {
                                       ShutdownStage2();
                                   });
            return;
        }
    }
    LogDebug("P2PManagerImpl", "ShutdownStage2", "Sutdown Stage2 complete.");
    if (this->p2pDevice)
    {
        this->wpaSupplicant->RemoveInterface(p2pDevice->getObjectPath());
    }
    StartWaitUntilDone();
}
void P2PManagerImpl::Stop()
{
    SignalStop();
    Wait();
}
bool P2PManagerImpl::IsFinished()
{
    return dispatcher.IsFinished();
}

bool P2PManagerImpl::ReloadRequested()
{
    return this->reloadRequested;
}

void P2PManagerImpl::Wait()
{
    if (!waited)
    {
        waited = true;
        dispatcher.Wait();
        LogInfo("P2PManager", "Stop", "Shutdown complete.");
    }
}

void P2PManagerImpl::StartWaitUntilDone()
{
    this->waitUntilDoneTimeout = clock::now() + std::chrono::duration_cast<clock::duration>(std::chrono::milliseconds(1000));

    PostWaitUntilDone();
}
void P2PManagerImpl::PostWaitUntilDone()
{
    if (clock::now() > this->waitUntilDoneTimeout)
    {
        LogError("P2PManager", "PostWaitUntilDone", "WaitUntilDone timeout expired.");
    }
    else
    {
        auto interfaces = this->wpaSupplicant->Interfaces();
        if (interfaces.size() > 0)
        {
            dispatcher.PostDelayed(
                std::chrono::milliseconds(100),
                [this]()
                {
                    PostWaitUntilDone();
                });
        }
    }
    // the dispatcher won't exit while there's a pending post.
    dispatcher.PostDelayed(
        std::chrono::milliseconds(100),
        [this]()
        {
            UnInitialize();
        });
}
void P2PManagerImpl::StartGroup()
{
    auto groups = p2pDevice->PersistentGroups();
    if (groups.size() == 0)
        throw std::runtime_error("StartGroup() called with no persistent groups.");

    std::map<std::string, sdbus::Variant> args;
    args["persistent_group_object"] = groups[0];
    this->p2pDevice->GroupAdd(args);
}

void P2PManagerImpl::CancelMaybeRemoveGroup()
{
    if (maybeCloseHandle)
    {
        dispatcher.CancelPost(maybeCloseHandle);
        maybeCloseHandle = 0;
    }
}
void P2PManagerImpl::MaybeRemoveGroup()
{

    CancelMaybeRemoveGroup();
    if (dispatcher.IsStopping())
    {
        return;
    }
    maybeCloseHandle = dispatcher.PostDelayed(
        std::chrono::milliseconds(100),
        [this]()
        {
            maybeCloseHandle = 0;

            if (this->p2pDevice->Stations().size() == 0)
            {
                if (this->p2pGoDevice)
                {
                    try
                    {
                        p2pGoDevice->Disconnect();
                    }
                    catch (const std::exception &e)
                    {
                        LogError(p2pGoDevice->getObjectPath(), "MaybeRemoveGroup", "Failed to disconnect.");
                    }
                }
            }
        });
}

std::vector<std::string> s_idleStates{
    "disconnected",
    "inactive",
    "interface_disabled",
    "scanning",
    //"authenticating",
    //"associating",
    //"associated",
    //"4way_handshake",
    //"group_handshake",
    "completed",
    //"unknown",
};

void P2PManagerImpl::MaybeCancel()
{
    std::string state = this->p2pDevice->State();
    for (const auto &idleState : s_idleStates)
    {
        if (idleState == state)
        {
            return;
        }
    }
    try
    {
        LogWarning("P2PManager","MaybeCancel",SS("Cancelling (State=" << state << ")"));
        this->p2pDevice->Cancel();
    }
    catch (const std::exception &)
    {
    }
}

std::string P2PManagerImpl::UpnpServiceName()
{
    return SS("uuid:" << settings.service_uuid() << "::urn:schemas-twoplay-com:service:PiPedal:1"
                      << "::port:" << settings.web_server_port());
}

void P2PManagerImpl::StartServiceBroadcast()
{
    // Upnp service advertising allows the android client to automatically discover pipedal Wifi P2P devices.
    try
    {
        try {
            p2pDevice->FlushService();
        } catch (const std::exception&)
        {
            
        }
        std::map<std::string, sdbus::Variant> args;
        args["service_type"] = "upnp";
        args["version"] = (int32_t)0x10;
        args["service"] = UpnpServiceName();
        std::vector<std::uint8_t> response;
        response.push_back(0);
        args["response"] = response;
        p2pDevice->AddService(
            args);
        serviceBroadcastStarted = true;
    }
    catch (const std::exception &e)
    {
        LogError(
            p2pDevice->getObjectPath(),
            "StartServiceBroadcast",
            SS("Failed to start Wi-Fi service broadcast. " << e.what()));
    }
}
void P2PManagerImpl::StopServiceBroadcast()
{
    try
    {
        if (serviceBroadcastStarted)
        {
            serviceBroadcastStarted = false;

            std::map<std::string, sdbus::Variant> args;
            args["service_type"] = "upnp";
            args["version"] = (int32_t)10;
            args["service"] = UpnpServiceName();
            p2pDevice->DeleteService(args);
        }
    }
    catch (std::exception &e)
    {
        LogDebug("P2PManager", "StopServiceBroadcast", e.what());
    }
}

void P2PManagerImpl::UnInitialize()
{
    // should all have been released at this point,
    // but still best to be certain.

    // the must NOT survive destruction of the dispatcher.

    this->connectedPeers.clear();
    this->wpaGroup = nullptr;
    this->p2pGoDevice = nullptr;
    this->wpsDevice = nullptr;
    this->p2pDevice = nullptr;
    this->wpsDevice = nullptr;

    if (!oldWpasDebugLevel.empty())
    {
        wpaSupplicant->DebugLevel(oldWpasDebugLevel);
    }

    wpaSupplicant = nullptr; // completely shut down after this.
}

void P2PManagerImpl::TurnOnWifi()
{
    bool wirelessEnabled;
    auto networkManager = NetworkManager::Create(dispatcher);

    try
    {
        wirelessEnabled = networkManager->WirelessEnabled();
    }
    catch (const std::exception &e)
    {
        LogDebug("P2PManager", "TurnOnWifi", "NetworkManager is not available.");
        return;
    }

    if (!wirelessEnabled)
    {
        networkManager->WirelessEnabled(true);
    }
}

void P2PManagerImpl::SignalHangup()
{
    if (signalStop)
        return;
    // reloadRequested = true;

    dispatcher.SignalStop([this]()
                          {
        dispatcher.Post([this]()
        {
            LogInfo("P2PManager","SignalHangup","Reload requested.");
            ExecuteOrderlyShutdown();
        });
        ExecuteOrderlyShutdown(); });
    signalStop = true;
}
void P2PManagerImpl::SignalStop()
{
    if (signalStop)
        return;
    dispatcher.SignalStop([this]()
                          { ExecuteOrderlyShutdown(); });
    signalStop = true;
}
void P2PManagerImpl::StartListen()
{
    if (!dispatcher.IsStopping())
    {
        try {
            p2pDevice->Listen((int32_t)5);

            std::map<std::string, sdbus::Variant> listenArgs;
            listenArgs["interval"] = (int32_t)3000; // listen for...
            listenArgs["period"] = (int32_t)1000;    // check every...
            this->p2pDevice->ExtendedListen(listenArgs);
        } catch (const std::exception&)
        {
        }
    }
}

static std::string UnquoteSsid(const std::string &ssid)
{
    if (ssid[0] != '\"') return ssid;
    return ssid.substr(1,ssid.length()-2);

}
sdbus::ObjectPath P2PManagerImpl::GetActivePersistentGroup()
{
    if (!wpaGroup)
    {
        throw std::runtime_error("No active group.");
    }

    auto groupSsidBytes = wpaGroup->SSID();
    std::string groupSsid { (const char*)(&groupSsidBytes[0]),groupSsidBytes.size()};
    
    for (const auto&persistentGroupPath: p2pDevice->PersistentGroups())
    {
        auto persistentGroup = WpaPersistentGroup::Create(dispatcher.Connection(),persistentGroupPath);
        auto properties = persistentGroup->Properties();
        std::string persistentGroupSsid = properties["ssid"];
        persistentGroupSsid = UnquoteSsid(persistentGroupSsid);
        if (persistentGroupSsid == groupSsid)
        {
            return persistentGroupPath;
        }
    }
    throw std::runtime_error("Active persistent group not found.");
}

class ServiceTLV {
public:
    enum class ProtocolTypes: uint8_t { All = 0, Bonjour = 1, UPnP = 2, WSDiscovery = 3, WifiDisplay = 4, P2Ps = 11};

    ServiceTLV() { }
    ServiceTLV(const std::vector<uint8_t> &tlv) {
        size_t length = tlv[1] *256 + tlv[0];
        protocolType = (ProtocolTypes)tlv[2];
        transactionId = tlv[3];
        for (size_t i = 4; i <= length; ++i)
        {
            queryData.push_back(tlv[i]);
        }
    }

    ProtocolTypes ProtocolType() const { return protocolType; }
    void ProtocolType(ProtocolTypes type) { protocolType = type;}
    uint8_t TransactionId() const { return transactionId; }
    void TransactionId(uint8_t value) { transactionId = value; }
    const std::vector<uint8_t> QueryData() const { return queryData; }
    std::string UpnpQueryString() const { return std::string((char*)&(queryData[1]),queryData.size()-1);}

    void QueryData(const std::vector<uint8_t> &value) { queryData = value; }
    uint8_t UpnpVersion() const { return queryData[0]; }
    void UpnpQueryString(const std::string &value) {
        queryData.resize(0);
        queryData.reserve(value.length()+1);
        queryData.push_back(10);
        for (char c: value)
        {
            queryData.push_back((uint8_t)c);
        }
    }
    std::vector<uint8_t> Build() const {
        std::vector<uint8_t> result;
        size_t length = queryData.size()+4;
        result.reserve(length);
        result.push_back((uint8_t)length);
        result.push_back((uint8_t)(length >> 8));
        result.push_back((uint8_t) protocolType);
        result.push_back(transactionId);
        for (uint8_t v: queryData)
        {
            result.push_back(v);
        }
        return result;
    }

private:
    ProtocolTypes protocolType;
    uint8_t transactionId;
    uint16_t length;
    std::vector<uint8_t> queryData;
};

static bool IsDigit(char c)
{
    return c >= '0' && c <= '9';
}

static std::string StripVersion(const std::string&v)
{
    size_t len = v.length();
    while (len > 0 && IsDigit(v[len-1]))
    {
        --len;
    }
    if (len > 0 && v[len-1] == '.')
    {
        --len;
    }
    return v.substr(0,len);
}
void P2PManagerImpl::OnServiceDiscoveryRequest(const std::map<std::string, sdbus::Variant> &sd_request)
{
    // try {
    //     sdbus::ObjectPath peer_object = sd_request.at("peeer_object");
    //     int32_t frequency = sd_request.at("frequency");
    //     int32_t dialog_token = sd_request.at("dialog_token");
    //     std::vector<std::uint8_t> request_tlvs = sd_request.at("tlvs");

    //     ServiceTLV request { request_tlvs};
    //     if (request.ProtocolType() != ServiceTLV::ProtocolTypes::Any && request.ProtocolType() != ServiceTLV::ProtocolTypes::UPnP )
    //     {
    //         return;
    //     }
    //     std::string requestUrn = request.UpnpQueryString();
    //     requestUrn = StripVersion(requestUrn);
    //     if (requestUrn != "urn:schemas-twoplay-com:service:PiPedal")
    //     {
    //         return;
    //     }
    //     ServiceTLV response;
    //     response.ProtocolType(ServiceTLV::ProtocolTypes::UPnP);
    //     response.TransactionId(request.TransactionId());
    //     response.UpnpQueryString(UpnpServiceName());




    //     // [44, 0, 2, 2, 16, 117, 114, 110, 58, 115, 99, 104, 101, 109, 97, 115, 45, 116, 119, 111, 112, 108, 97, 121, 45, 99, 111, 109, 58, 115, 101, 114, 118, 105, 99, 101, 58, 80, 105, 80, 101, 100, 97, 108, 58, 49
    //     std::map<std::string, sdbus::Variant> args;
    //     args["peer_object"] = peer_object;
    //     args["frequency"] = frequency;

    //     this->p2pDevice->ServiceDiscoveryResponse(args);
    // } catch (const std::exception&e)
    // {
    //     LogError("P2PManager","OnServiceDiscoveryRequest",e.what());
    // }
}

bool P2PManagerImpl::TerminatedNormally()
{
    return normalTermination;
}


#endif