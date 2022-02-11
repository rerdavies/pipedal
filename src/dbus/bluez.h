#include "bluez_proxy.h"


namespace org { namespace bluez {


using BleDevice_Interfaces = sdbus::ProxyInterfaces<
    org::bluez::LEAdvertisingManager1_proxy,
    org::bluez::NetworkServer1_proxy,
    org::bluez::GattManager1_proxy
    >;

class BleDeviceProxy : public BleDevice_Interfaces
{
public:
    BleDeviceProxy(std::string devicePath)
    : BleDevice_Interfaces("org.bluez",std::move(devicePath))
    {
        registerProxy();
    }
    ~BleDeviceProxy()
    {
        unregisterProxy();
    }
};


}}; // namesapce