#include "pch.h"
#include "catch.hpp"
#include <sstream>
#include <cstdint>
#include <string>
#include "PiPedalException.hpp"
#include <iostream>


#include "SystemConfigFile.hpp"


using namespace pipedal;


TEST_CASE( "SystemConfigFile Test", "[system_config_file_test]" ) {
    boost::filesystem::path path("/etc/hostapd/hostapd.conf");

    SystemConfigFile file(path);
    std::string driverName;

    REQUIRE(file.Get("driver") == "nl80211");

    file.Set("ssid","ssid","Name of the access point");
    file.Set("xx","new_feature","A new feature.");
    file.Set("ssid","newSsid");
    file.Set("country_code","FR");
    

    file.Save(std::cout);



}

