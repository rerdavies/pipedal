#include "NMManager.hpp"
#include <sdbus-c++/sdbus-c++.h>
#include "NetworkManagerInterfaces.hpp"
#include "WpaInterfaces.hpp"
#include "DBusLog.hpp"
#include "NMP2pSettings.hpp"
#include "ss.hpp"

////////////////////////////////////////////////////////////

class NMManagerImpl : public NMManager
{
public:
    NMManagerImpl();
    virtual ~NMManagerImpl();
    virtual void Init();
    virtual void Run();
    virtual void Stop();

private:
    void MaybeStartWpas();
    void MaybeStopWpas();
    using PostCallback = DBusDispatcher::PostCallback;
    void Post(PostCallback &&callback);

    void OnWpaInterfaceAdded(const sdbus::ObjectPath &path);
    void OnWpaInterfaceRemoved(const sdbus::ObjectPath &path);
    void StartConnection(const std::vector<uint8_t> &peerAddress);

private:
    void InitWpaP2PDevice(WpaP2PDevice::ptr &&device);
    void InitWpaP2PGoDevice(WpaP2PDevice::ptr &&device);
    P2pSettings settings;
    DBusDispatcher dispatcher;
    WpaSupplicant::ptr wpaSupplicant;
    WpaP2PDevice::ptr wpaP2PDevice;
    WpaP2PDevice::ptr wpaP2PGoDevice;
    WpaGroup::ptr wpaGroup;
    Device::ptr nmDevice;
    std::string groupPath;
};

////////////////////////////////////////////////////////////
NMManager::NMManager()
{
}
NMManager::~NMManager()
{
}

NMManagerImpl::NMManagerImpl()
    : dispatcher(true)
{
}

void NMManagerImpl::InitWpaP2PGoDevice(WpaP2PDevice::ptr &&device)
{
    if (this->wpaP2PGoDevice)
        return;
    this->wpaP2PGoDevice = std::move(device);
}
void NMManagerImpl::InitWpaP2PDevice(WpaP2PDevice::ptr &&device)
{
    if (this->wpaP2PDevice)
        return;

    // make sure the NetworkManager and wpa_supplicant objects are for the same device.
    std::string wpaInterfaceName = device->Ifname();
    std::string nmInterface = nmDevice->Interface();
    if (nmInterface != "p2p-dev-" + wpaInterfaceName)
    {
        return;
    }

    this->wpaP2PDevice = std::move(device);

    std::map<std::string, sdbus::Variant> deviceConfig;
    deviceConfig["DeviceName"] = SS(settings.device_name());
    deviceConfig["PrimaryDeviceType"] = std::vector<uint8_t>{0x00, 0x01, 0x00, 0x50, 0xF2, 0x04, 0x00, 0x01};
    deviceConfig["PersistentReconnect"] = false;
    deviceConfig["SsidPostfix"] = SS("-" << settings.device_name());
    deviceConfig["GOIntent"] = (uint32_t)15;
    // deviceConfig["PersistentReconnect"] = (uint32_t)2;
    // deviceConfig["ListenRegClass"] = (uint32_t)81;
    // deviceConfig["ListenChannel"] = (uint32_t)settings.listen_channel();
    // deviceConfig["OperRegClass"] = (uint32_t)81;
    // deviceConfig["OperChannel"] = (uint32_t)settings.op_channel(); // ch 1

    deviceConfig["IpAddrGo"] = std::vector<uint8_t>{172, 23, 0, 1};
    deviceConfig["IpAddrMask"] = std::vector<uint8_t>{255, 255, 255, 0};
    deviceConfig["IpAddrStart"] = std::vector<uint8_t>{172, 23, 0, 10};
    deviceConfig["IpAddrEnd"] = std::vector<uint8_t>{172, 23, 0, 250};


    wpaP2PDevice->P2PDeviceConfig(deviceConfig);

    {
        WpaWPS::ptr wpsDevice = WpaWPS::Create(dispatcher.Connection(), wpaP2PDevice->getObjectPath());
        wpsDevice->DeviceName(settings.device_name());
        wpsDevice->Manufacturer(settings.manufacturer());
        wpsDevice->ModelName(settings.model_name());
        wpsDevice->ModelNumber(settings.model_number());
        wpsDevice->SerialNumber(settings.serial_number());
        wpsDevice->DeviceType(settings.device_type());
    }

    wpaP2PDevice->OnGroupStarted.add(
        [this](const std::map<std::string, sdbus::Variant> &)
        {
            try
            {
                Post([this]()
                     {
                    LogDebug("NMManager", "OnGroupStarted", "GroupStarted");
                    auto group = this->wpaP2PDevice->Group();
                    if (!group.empty())
                    {
                        wpaGroup = WpaGroup::Create(dispatcher.Connection(),group);
                        wpaGroup->OnPeerJoined.add([this](const sdbus::ObjectPath&path) {
                            Post([this,path]() 
                            {
                                try {
                                    LogDebug("NMManager","OnPeerJoined",path.c_str());
                                } catch (const std::exception&e)
                                {
                                    LogError("NMManager","OnPeerJoined",e.what());

                                }
                            }
                            );
                        });
                        wpaGroup->OnPeerDisconnected.add([this](const sdbus::ObjectPath&path){
                            Post([this,path](){
                                LogDebug("NMManager","OnPeerDisconnected",path.c_str());
                            });
                        });
                    } });
                wpaP2PDevice->OnGONegotiationFailure.add(
                    [this](const std::map<std::string, sdbus::Variant> &info)
                    {
                        LogInfo(wpaP2PDevice->getObjectPath().c_str(), "OnGoNegotiationFailure", "P2P GO negotiation failed.");
                    });
                wpaP2PDevice->OnGroupFormationFailure.add(
                    [this](const std::string &reason)
                    {
                        LogInfo(wpaP2PDevice->getObjectPath().c_str(), "OnGroupFormationFailure", 
                            SS("Group formation failed. " << reason));
                    });
            }

            catch (const std::exception &e)
            {
                LogError("P2pManager", "OnGroupStarted", e.what());
            }
        });

    // std::map<std::string,sdbus::Variant> startParams;
    // startParams["Role"] = "registrar";
    // startParams["Pin"] = settings.pin();
    // wpsDevice->Start(startParams);

    wpaP2PDevice->OnGroupFormationFailure.add(
        [this](const std::string &reason)
        {
            Post([this]()
                 {
                if (this->wpaP2PDevice)
                {
                    try {
                    } catch (const std::exception&e)
                    {
                        LogError("NMManager","OnGroupFormationFailure", SS("Listen failed: " << e.what()));
                    }
                } });
        });
    wpaP2PDevice->OnGONegotiationFailure.add(
        [this](const std::map<std::string, sdbus::Variant> &info)
        {
            Post([this]()
                 {
                if (this->wpaP2PDevice)
                {
                    try {
                    } catch (const std::exception&e)
                    {
                        LogError("NMManager","OnGONegotiationFailure", SS("Listen failed: " << e.what()));
                    }
                } });
        });

    wpaP2PDevice->OnProvisionDiscoveryRequestDisplayPin.add(
        [this](const sdbus::ObjectPath &peer_object, const std::string &pin)
        {
            LogTrace("NMManager", "InitWpaP2PDevice",
                     SS("OnProvisionDiscoveryRequestDisplayPin - " << peer_object.c_str() << " pin: " << pin));

            Post(
                [this, peer_object]()
                {
                    try
                    {
                        if (!this->wpaP2PDevice)
                            return;
                        WpaPeer::ptr peer = WpaPeer::Create(dispatcher.Connection(), peer_object);
                        std::vector<uint8_t> deviceAddress = peer->DeviceAddress();
                        StartConnection(deviceAddress);
                    }
                    catch (const std::exception &e)
                    {
                        LogError(
                            "NMManagerImpl", "OnProvisionDiscoveryRequestDisplayPin",
                            SS("Connection not started. " << e.what()));
                    };
                });
        });

    {
        this->wpaP2PDevice->Listen((int32_t)5000);

        std::map<std::string, sdbus::Variant> listenArgs;
        listenArgs["interval"] = (int32_t)5000; // listen for...
        listenArgs["period"] = (int32_t)100;    // check every...
        this->wpaP2PDevice->ExtendedListen(listenArgs);
        LogInfo(this->wpaP2PDevice->getObjectPath(), "InitWpaP2PDevice", "Listening");
    };
}
void NMManagerImpl::MaybeStartWpas()
{
    if (!wpaSupplicant)
        return;

    try
    {
        for (const auto &interfacePath : wpaSupplicant->Interfaces())
        {
            try
            {
                auto p2pDevice = WpaP2PDevice::Create(dispatcher.Connection(), interfacePath);
                // must be for the same wpa instance as network manager device.

                std::string role = p2pDevice->Role();
                if (role == "GO")
                {
                    this->InitWpaP2PGoDevice(std::move(p2pDevice));
                }
                else if (role == "device")
                {
                    this->InitWpaP2PDevice(std::move(p2pDevice));
                }
            }
            catch (const std::exception &e)
            {
                LogWarning("NMManager", "MaybeStartWpas", e.what());
            }
        }
    }
    catch (const std::exception &e)
    {
        LogError("NMManagerImpl", "MaybeStartWpas", e.what());
    }
}
void NMManagerImpl::MaybeStopWpas()
{
}
void NMManagerImpl::OnWpaInterfaceAdded(const sdbus::ObjectPath &objectPath)
{
    Post([this]()
         { MaybeStartWpas(); });
}
void NMManagerImpl::OnWpaInterfaceRemoved(const sdbus::ObjectPath &objectPath)
{
    Post([this, objectPath]()
         {
        if (this->wpaP2PDevice && this->wpaP2PDevice->getObjectPath() == objectPath)
        {
            this->wpaP2PDevice = nullptr;
        }
        if (this->wpaP2PGoDevice->getObjectPath() == objectPath)
        {
            this->wpaP2PGoDevice = nullptr;
        } });
    MaybeStopWpas();
}
void NMManagerImpl::Init()
{
    this->wpaSupplicant = WpaSupplicant::Create(dispatcher.Connection());

    this->wpaSupplicant->OnInterfaceAdded.add(
        [this](const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties)
        {
            this->OnWpaInterfaceAdded(path);
        });
    this->wpaSupplicant->OnInterfaceRemoved.add(
        [this](const sdbus::ObjectPath &path)
        {
            this->OnWpaInterfaceRemoved(path);
        });

    Post([this]()
         { 
            MaybeStartWpas(); });
}
NMManagerImpl::~NMManagerImpl()
{
    Stop();
}


NMManager::ptr NMManager::Create()
{
    return std::make_unique<NMManagerImpl>();
}

void NMManagerImpl::Run()
{
    dispatcher.Run();
}

void NMManagerImpl::Stop()
{
    if (this->wpaP2PDevice)
    {
        // no parameters cancels Extended listen.
        std::map<std::string, sdbus::Variant> listenArgs;
        this->wpaP2PDevice->ExtendedListen(listenArgs);
    }

    wpaP2PDevice = nullptr;
    wpaP2PGoDevice = nullptr;
    wpaSupplicant = nullptr;
    nmDevice = nullptr;

    dispatcher.Stop();
}

void NMManagerImpl::Post(NMManagerImpl::PostCallback &&callback)
{
    dispatcher.Post(std::move(callback));
}

static const char hexDigits[] = "0123456789ABCDEFx";

static std::string peerToString(const std::vector<uint8_t> &peer)
{
    std::stringstream s;
    bool firstTime = true;
    for (uint8_t byte : peer)
    {
        if (!firstTime)
            s << ':';
        firstTime = false;
        s << hexDigits[(byte >> 4) & 0x0F] << hexDigits[byte & 0x0F];
    }
    return s.str();
}

void NMManagerImpl::StartConnection(const std::vector<uint8_t> &peer)
{
}