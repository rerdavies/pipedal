#include "bluez_adaptor.h"
#include "bluez.h"

#include "catch.hpp"

#define TEST_UUID "6b49786f-271d-415d-8e2c-bd5ef8aa6f47"
using TestAdvertisementInterfaces = sdbus::AdaptorInterfaces<org::bluez::LEAdvertisement1_adaptor>;

class TestAdvertisement: public TestAdvertisementInterfaces
{
private:
    std::vector<std::string> serviceUuids;
public:
    TestAdvertisement(sdbus::IConnection& connection, std::string objectPath)
    : TestAdvertisementInterfaces(connection,std::move(objectPath))
    {
        serviceUuids.push_back(TEST_UUID);
    }
private:

    virtual void Release() { delete this; }

private:
    virtual std::string Type() { return "peripheral"; }
    virtual std::vector<std::string> ServiceUUIDs() { return serviceUuids; }
    virtual std::map<std::string, sdbus::Variant> ManufacturerData() {
        return std::map<std::string, sdbus::Variant>();
    };
    virtual std::vector<std::string> SolicitUUIDs() {
        return std::vector<std::string>();
    }
    virtual std::map<std::string, sdbus::Variant> ServiceData() 
    {
        return std::map<std::string, sdbus::Variant>();
    }
    virtual bool IncludeTxPower() {
        return false;
    }

    
};

void RunService()
{
    const char* SERVICE_NAME="com.twoplay.pipedal.onboarding";
    const char* SERVICE_PATH="/com/twoplay/pipedal/onboarding";

    auto connector = sdbus::createSystemBusConnection(SERVICE_NAME);
    TestAdvertisement testAdvertisement(*connector,SERVICE_PATH);

}

TEST_CASE( "Bluetooth service", "[bluetooth_service]" ) {
    org::bluez::BleDeviceProxy device("org.bluez","/org/bluez/hci0");

    auto includes = device.SupportedIncludes();

}
