#pragma once

#include <vector>
#include <string>

#include "WifiRegulations.hpp"
struct nl_sock;
struct nl_cache;
struct genl_family;

namespace pipedal
{

    WifiInfo getWifiInfo(const std::string&countryIso3661);
    int32_t getWifiRegClass(const std::string &countryIso3661, int32_t channel,int32_t maxChannelWidthMhz);
    std::vector<int32_t> getValidChannels(const std::string&countryIso3661,int32_t maxChannelWidthMhz);


}