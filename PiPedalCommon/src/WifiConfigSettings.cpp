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

#include "WifiConfigSettings.hpp"
#include <stdexcept>
#include "ss.hpp"
#include "ChannelInfo.hpp"

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

int32_t pipedal::ChannelToChannelNumber(const std::string&channel)
{
    std::string t = channel;
    // remove dprecated band specs.
    if (t.size() > 1 && t[0] == 'a' || t[0] == 'g')
    {
        t = t.substr(1);
    }
    int32_t channelNumber = 1;
    std::stringstream ss(t);
    ss >> channelNumber;
    return channelNumber;

}

static uint32_t ParseChannel(const std::string & channel)
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
    return (uint32_t)lChannel;
}

uint32_t pipedal::ChannelToWifiFrequency(const std::string &channel_)
{
    uint32_t channel = ParseChannel(channel_);
    return ChannelToWifiFrequency(channel);
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
    throw invalid_argument(SS("Invalid WiFi channel: " << channel));

}


bool WifiConfigSettings::ValidateChannel(const std::string&countryCode,const std::string&value)
{
    if (value == "0")  // = "Autoselect".
    {
        return true;
    }
    // 1) unadorned channel number 1, 2,3 &c.
    // 2) With band annotated: g1, a51.
    if (countryCode.empty())
    {
        throw std::invalid_argument("Please supply a country code.");
    }
    if (countryCode.length() != 2)
    {
        throw std::invalid_argument(SS("Invalid country code: " << countryCode));
    }
    auto regDom = getWifiRegClass(countryCode,ParseChannel(value),40);
    if (regDom == -1) {
        std::vector<int32_t> valid_channels = getValidChannels(countryCode,40);
        std::stringstream ss;
        ss << "Channel " << value << " is not permitted in the selected country.\n     Valid channels: ";
        bool first = true;
        for (auto channel: valid_channels)
        {
            if (!first) ss << ", ";
            first = false;
            ss << channel;
        }
        throw invalid_argument(ss.str());
    }

    ChannelToWifiFrequency(value);
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

    if (!ValidateChannel(this->countryCode_,this->channel_))
    {
        throw invalid_argument("Channel is not valid.");
    }

    // validate that the channel number is supported for the given country code.


    this->valid_ = true;

 }
