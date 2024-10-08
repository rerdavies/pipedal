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


// #include <unistd.h>
// #include <fcntl.h>
// #include <string>
// #include <net/if.h>
// #include <netlink/netlink.h>
// #include <netlink/genl/genl.h>
// #include <netlink/genl/family.h>
// #include <netlink/genl/ctrl.h>  
// #include <netlink/msg.h>
// #include <netlink/attr.h>
// #include <linux/nl80211.h>

// #include <linux/nl80211.h>
#include "PiPedalException.hpp"
#include "WifiChannelSelectors.hpp"
#include "Lv2Log.hpp"
#include <set>

#include "RegDb.hpp"

#include "ChannelInfo.hpp"

using namespace pipedal;



// static bool firstTime = true;
// static std::unique_ptr<RegDb> g_regDb;

std::vector<WifiChannelSelector> pipedal::getWifiChannelSelectors(const char*countryIso3661, bool forCommandline)
{
    // Use the wifi physical device to get a list of channels
    // that the hardware supports.
    // The override the hardware's regulation flags using regulation.bin
    // database info for the selected country code.
    // If the regulation.bin database doesn't have a matching record,
    // just pass the data that the wifi reported.

    // regdb is broken on bookworm.
    // if (firstTime) {
    //     firstTime = false;
    //     try {
    //         g_regDb = std::make_unique<RegDb>();
    //     } catch (const std::exception& e)
    //     {
    //         std::stringstream s;
    //         s << "Failed to open the Wi-Fi regulatory.bin database. " << e.what();
    //         Lv2Log::error(s.str());
    //     }
    // }
    std::vector<WifiChannelSelector> result;


    if (forCommandline) {
        WifiChannelSelector autoSelect;
        autoSelect.channelId_ = "0";
        autoSelect.channelName_ = "0 Automatic";
        result.push_back(autoSelect);
    } else {
        WifiChannelSelector autoSelect;
        autoSelect.channelId_ = "0";
        autoSelect.channelName_ = "Automatic";
        result.push_back(autoSelect);
    }
    WifiInfo wifiInfo = getWifiInfo(countryIso3661);

    std::set<int32_t> visitedChannels;
    for (auto &channel: wifiInfo.channels) {
        if ((!channel.disabled) && (!channel.radarDetection))
        {
            if (channel.no20MHz)
            {
                continue;
            }
            // ignore duplicates
            if (visitedChannels.contains(channel.channelNumber))
            {
                continue;
            }
            visitedChannels.insert(channel.channelNumber);
            WifiChannelSelector ch;
            std::stringstream s;
            s << channel.channelNumber;
            ch.channelId_ = s.str();

            std::stringstream t;
            t << channel.channelNumber;
            switch (channel.hardwareMode)
            {
                case WifiMode::IEEE80211G:
                    t << " (2.4GHz)";
                    break;
                case WifiMode::IEEE80211A:
                case WifiMode::IEEE80211B:
                    t << " (5GHz)";
                    break;
                default: 
                    continue;
            }
            if (channel.indoorOnly) {
                t << " Indoors only";
            } 
            if (channel.outdoorsOnly) {
                t << " Outdoors only";
            }
            ch.channelName_ = t.str();
            result.push_back(ch);
        }
    }
    return result;
}


JSON_MAP_BEGIN(WifiChannelSelector)
    JSON_MAP_REFERENCE(WifiChannelSelector,channelId)
    JSON_MAP_REFERENCE(WifiChannelSelector,channelName)
JSON_MAP_END()


