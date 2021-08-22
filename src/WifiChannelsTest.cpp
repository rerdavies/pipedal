#include "pch.h"
#include "catch.hpp"
#include <sstream>
#include <cstdint>
#include <string>

#include "WifiChannels.hpp"



using namespace pipedal;

static WifiChannel GetChannel(std::vector<WifiChannel> channels, const std::string&channelId)
{
    for (auto &channel: channels)
    {
        if (channel.channelId_ == channelId)
        {
            return channel;
        }
    }
    throw PiPedalException("Channel not found.");;
}



TEST_CASE( "Wifi Channel Test", "[wifi_channels_test]" ) {
    std::vector<WifiChannel> result = pipedal::getWifiChannels("CA");

    // channel 36 is indoors only (CA)
    WifiChannel channel = GetChannel(result,"a36");
    REQUIRE(channel.channelName_.find("Indoors") != std::string::npos);

    // channel 36 is not indoors only (US)
    result = pipedal::getWifiChannels("US");
    channel = GetChannel(result,"a36");
    REQUIRE(channel.channelName_.find("Indoors") == std::string::npos);
}

