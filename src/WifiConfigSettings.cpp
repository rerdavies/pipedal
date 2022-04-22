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
#include "WifiConfigSettings.hpp"
#include <stdexcept>

using namespace pipedal;
using namespace std;

JSON_MAP_BEGIN(WifiConfigSettings)
    JSON_MAP_REFERENCE(WifiConfigSettings,valid)
    JSON_MAP_REFERENCE(WifiConfigSettings,wifiWarningGiven)
    JSON_MAP_REFERENCE(WifiConfigSettings,rebootRequired)
    JSON_MAP_REFERENCE(WifiConfigSettings,enable)
    JSON_MAP_REFERENCE(WifiConfigSettings,hotspotName)
    JSON_MAP_REFERENCE(WifiConfigSettings,mdnsName)
    JSON_MAP_REFERENCE(WifiConfigSettings,hasPassword)
    JSON_MAP_REFERENCE(WifiConfigSettings,password)
    JSON_MAP_REFERENCE(WifiConfigSettings,countryCode)
    JSON_MAP_REFERENCE(WifiConfigSettings,channel)
JSON_MAP_END()

bool WifiConfigSettings::ValidateCountryCode(const std::string&value)
{
    if (value.size() < 2 || value.size() > 3) return false;
    return true;
}


uint32_t pipedal::ChannelToWifiFrequency(const std::string &channel)
{
    std::string t = channel;
    // remove dprecated band specs.
    if (t.size() > 1 && t[0] == 'a' || t[0] == 'g')
    {
        t = t.substr(1);
    }
    size_t size = t.length();

    unsigned long long lChannel = std::stoull(t,&size);
    if (size != t.length())
    {
        throw invalid_argument("Expecting a number: '" + t + "'.");
    }
    return ChannelToWifiFrequency((uint32_t)lChannel);
}

uint32_t pipedal::ChannelToWifiFrequency(uint32_t channel)
{
    if (channel > 1000) // must be a frequency.
    {
        return channel;
    }
    // 2.4GHz.
    if (channel >= 1 && channel <= 13)
    {
        return 2412 + 5*(channel-1);
    }
    if (channel == 14)
    {
        return 2484;
    }
    // 802.11y
    if (channel >= 131 && channel < 137)
    {
        return 3660 + (channel-131)*5;
    }
    if (channel >= 32 && channel <= 68 && (channel & 1) == 0)
    {
        return 5160 + (channel-32)/2*10;
    }
    if (channel == 96) return 5480;

    if (channel >= 100 && channel <= 196)
    {
        return 5500 + (channel-100)/5;
    }
    throw invalid_argument(SS("Invalid channel: " << channel));

}


bool WifiConfigSettings::ValidateChannel(const std::string&value)
{
    // 1) frequency in khz.
    // 2) unadorned channel number 1, 2,3 &c.
    // 3) With band annotated: g1, a51.
    try {
        ChannelToWifiFrequency(value);
        return true;
    } catch (const std::string&/**/)
    {
        return false;
    }
    // come back to this.
    return true;
}

 void WifiConfigSettings::ParseArguments(const std::vector<std::string> &argv)
 {
    this->valid_ = false;
    if (argv.size() != 4) {
        throw invalid_argument("Invalid number of arguments.");
    }
    this->enable_ = true;
    this->countryCode_ = argv[0];
    this->hotspotName_ = argv[1];
    this->mdnsName_ = this->hotspotName_;
    this->password_ = argv[2];
    this->channel_ = argv[3];
    this->hasPassword_ = this->password_.length() != 0;

    if (!ValidateCountryCode(this->countryCode_))
    {
        throw invalid_argument("Invalid country code.");
    }
    if (this->hotspotName_.length() > 31) throw invalid_argument("Hotspot name is too long.");
    if (this->hotspotName_.length() < 1) throw invalid_argument("Hotspot name is too short.");
    if (this->password_.size() != 0 && this->password_.size() < 8) throw invalid_argument("Passphrase must be at least 8 characters long.");

    if (!ValidateChannel(this->channel_))
    {
        throw invalid_argument("Channel is not valid.");
    }
    this->valid_ = true;

 }
