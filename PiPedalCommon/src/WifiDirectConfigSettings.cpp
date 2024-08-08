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

// #include "pch.h"
#include "WifiDirectConfigSettings.hpp"
#include "P2pConfigFiles.hpp"
#include "ChannelInfo.hpp"

#include "WifiConfigSettings.hpp"
#include <stdexcept>
#include <random>
#include "ConfigUtil.hpp"
#include "ServiceConfiguration.hpp"
#include "WriteTemplateFile.hpp"
#include "ss.hpp"


using namespace pipedal;
using namespace std;

JSON_MAP_BEGIN(WifiDirectConfigSettings)
    JSON_MAP_REFERENCE(WifiDirectConfigSettings,valid)
    JSON_MAP_REFERENCE(WifiDirectConfigSettings,rebootRequired)
    JSON_MAP_REFERENCE(WifiDirectConfigSettings,enable)
    JSON_MAP_REFERENCE(WifiDirectConfigSettings,hotspotName)
    JSON_MAP_REFERENCE(WifiDirectConfigSettings,pinChanged)
    JSON_MAP_REFERENCE(WifiDirectConfigSettings,pin)
    JSON_MAP_REFERENCE(WifiDirectConfigSettings,countryCode)
    JSON_MAP_REFERENCE(WifiDirectConfigSettings,channel)
JSON_MAP_END()
 
static std::string MakePin()
{
    std::random_device rand_dev;

    std::mt19937 gen(rand_dev());
    std::uniform_int_distribution<int> dist { 0,9};

    std::stringstream s;
    for (size_t i = 0; i < 8; ++i)
    {
        int r = dist(gen);
        s << (char)('0' + r);
    }
    return s.str();
}
static void ValidatePin(const std::string&pin)
{
    for (char c : pin)
    {
        if (c < '0' || c > '9')
        {
            throw invalid_argument("Pin must consist of digits only.");
        }
    }
    if (pin.length() != 8)
    {
        throw invalid_argument("Pin must have exctly 8 digits.");
    }
}

void WifiDirectConfigSettings::ParseArguments(const std::vector<std::string> &arguments)
{
    this->valid_ = false;

    this->enable_ = true;

    Load();

    if (arguments.size() == 0)
    {
        this->enable_ = true;
    } else {
        if (arguments.size() < 2 || arguments.size() > 4)
        {
            throw invalid_argument("Incorrect number of arguments supplied");
        }
        this->countryCode_ = arguments[0];
        this->hotspotName_ = arguments[1];
        if (arguments.size() >= 3)
        {
            this->pin_ = arguments[2];
        }
        if (arguments.size() >= 4) {
            this->channel_ = arguments[3];
        }
    }
    if (this->channel_ == "")
    {
        this->channel_ = "1";
    }
    
    size_t maxLength = 32-string("DIRECT-XX-").length();
    if (hotspotName_.size() >= maxLength) { throw invalid_argument("Device name is too long."); }
    if (hotspotName_.size() < 1) { throw invalid_argument("Device name is too short."); }
    if (!WifiConfigSettings::ValidateCountryCode(countryCode_))
    {
        throw invalid_argument("Invalid country code.");
    }
    if (pin_.size() == 0)
    {
        pin_ = MakePin();
    }
    ValidatePin(pin_); // throws.

    // validate that the channel number is supported for the given country code.

    if (!WifiConfigSettings::ValidateChannel(this->countryCode_,channel_))
    {
        throw invalid_argument(SS("Invalid WiFi channel: " << channel_));
    }
}


void WifiDirectConfigSettings::Save() const
{
        // ******************* pipedal_p2pd ******
        {
            // ${COUNTRY_CODE}
            //${PIN}
            // ${DEVICE_NAME}
            // ${WIFI_GROUP_FREQUENCY}
            // ${INSTANCE_ID_FILE} /var/pipedal/instance_id

            std::map<string, string> map;
            map["COUNTRY_CODE"] = this->countryCode_;
            map["PIN"] = this->pin_;
            map["DEVICE_NAME"] = ConfigUtil::QuoteString(this->hotspotName_);
            map["WIFI_CHANNEL"] = SS(this->channel_);
            map["ENABLED"] = this->enable_ ? "true": "false";
            map["WIFI_GROUP_FREQUENCY"] = SS(ChannelToWifiFrequency(this->channel_));
            map["INSTANCE_ID_FILE"] = DEVICE_GUID_FILE;
            map["WLAN"] = this->wlan_;


            WriteTemplateFile(map, PIPEDAL_P2PD_CONF_PATH);
            
        }

}
void WifiDirectConfigSettings::Load()
{
    this->enable_ = false;
    std::string strEnable;
    if (ConfigUtil::GetConfigLine("/etc/pipedal/config/pipedal_p2pd.conf","enabled",&strEnable))
    {
        this->enable_ = (strEnable == "true" || strEnable == "1");
    }
    std::string strWlan;
    if (ConfigUtil::GetConfigLine("/etc/pipedal/config/pipedal_p2pd.conf","wlan",&strWlan))
    {
        this->wlan_ = strWlan;
    } else {
        this->wlan_ = "wlan0";
    }



    if (!ConfigUtil::GetConfigLine("/etc/pipedal/config/pipedal_p2pd.conf","p2p_device_name",&this->hotspotName_))
    {
        this->hotspotName_ = "PiPedal";
    }

    if (!ConfigUtil::GetConfigLine("/etc/pipedal/config/pipedal_p2pd.conf","country_code",&this->countryCode_))
    {
        this->countryCode_ = "US";
    }
    if (!ConfigUtil::GetConfigLine("/etc/pipedal/config/pipedal_p2pd.conf","p2p_pin",&this->pin_))
    {
        this->pin_ = "12345678";
    }
    if (!ConfigUtil::GetConfigLine("/etc/pipedal/config/pipedal_p2pd.conf","wifiChannel",&this->channel_))
    {
        this->channel_ = "6";
    }
    try {
        ServiceConfiguration deviceIdFile;
        deviceIdFile.Load();
        if (deviceIdFile.deviceName == "")
        {
            
        }
        this->hotspotName_ = deviceIdFile.deviceName;
    } catch (const std::exception &e)
    {
    }
    this->valid_ = true;

}
