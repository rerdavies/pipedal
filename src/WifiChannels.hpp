#pragma once 
#include "json.hpp"
namespace pipedal {

class WifiChannel  {
public:
    std::string channelId_;
    std::string channelName_;

    DECLARE_JSON_MAP(WifiChannel);
};

std::vector<WifiChannel> getWifiChannels(const char*countryIso3661);


}