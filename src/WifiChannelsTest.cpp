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

