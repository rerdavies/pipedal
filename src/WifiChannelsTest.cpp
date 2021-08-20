#include "pch.h"
#include "catch.hpp"
#include <sstream>
#include <cstdint>
#include <string>

#include "WifiChannels.hpp"



using namespace pipedal;



TEST_CASE( "Wifi Channel Test", "[wifi_channels_test]" ) {
    std::vector<WifiChannel> result = pipedal::getWifiChannels("CA");
}

