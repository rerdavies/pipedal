#include "bluez_adaptor.h"
#include "bluez.h"
#include <thread>

#include "catch.hpp"
#include "gsl/gsl"

#define TEST_UUID "6b49786f-271d-415d-8e2c-bd5ef8aa6f47"
using TestAdvertisementInterfaces = sdbus::AdaptorInterfaces<org::bluez::LEAdvertisement1_adaptor>;


static const char* SERVICE_NAME="com.twoplay.pipedal.onboarding";
static const char* SERVICE_PATH="/com/twoplay/pipedal/onboarding";


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

    virtual void Release() { }

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


using namespace gsl;
using namespace std;

class TestServer {
private:
    std::string devicePath;
    
    std::unique_ptr<TestAdvertisement> advertisement;

private:

    void ListenProc() 
    {

    }
public:
    TestServer(const std::string &devicePath)
    : devicePath(devicePath)
    {



    }

    void Run()
    {

        auto connector = sdbus::createSystemBusConnection(SERVICE_NAME);
        std::unique_ptr<TestAdvertisement> advertisement  = std::make_unique<TestAdvertisement>(*connector,SERVICE_PATH);

        std::unique_ptr<org::bluez::BleDeviceProxy> device = std::make_unique<org::bluez::BleDeviceProxy>(devicePath);


        std::map<std::string,sdbus::Variant> advertisementOptions;
        device->RegisterAdvertisement(advertisement->getObjectPath(),advertisementOptions);

        auto _ = finally([&device,&advertisement] {
            device->UnregisterAdvertisement(advertisement->getObjectPath());
        });

        std::thread thread(
            [this] () {
                this->ListenProc();
            }
        );
        
        thread.join();


    }
};

TEST_CASE( "Bluetooth service", "[bluetooth_service]" ) {
    org::bluez::BleDeviceProxy device("/org/bluez/hci0");

    auto includes = device.SupportedIncludes();

    TestServer server {"/org/bluez/hci0"};

    server.Run();

}
