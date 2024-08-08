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

#include "WifiChannelSelectors.hpp"
#include "ChannelInfo.hpp"
#include "RegDb.hpp"
#include <stdexcept>

using namespace pipedal;

static WifiChannelSelector GetChannel(const std::vector<WifiChannelSelector> &channels, const std::string &channelId)
{
    for (auto &channel : channels)
    {
        if (channel.channelId_ == channelId)
        {
            return channel;
        }
    }
    throw std::runtime_error("Channel not found.");
    ;
}
void TestWifiChannels()
{
    using namespace std;
    cout << "    ==== CA Channels ====" << endl;
    std::vector<WifiChannelSelector> result = pipedal::getWifiChannelSelectors("CA");
    for (const auto &channel : result)
    {
        std::cout << "   " << channel.channelName_ << std::endl;
    }

    // channel 36 is indoors only (CA)
    WifiChannelSelector channel = GetChannel(result, "36");


    // present before bookworm. no longer.
    REQUIRE(channel.channelName_.find("Indoors") != std::string::npos);
 
 
    for (const auto &channel : result)
    {
        std::cout << "   " << channel.channelName_ << std::endl;
    }

    // channel 36 is not indoors only (US)
    cout << "    ==== US  Channels ====" << endl;

    result = pipedal::getWifiChannelSelectors("US");
    for (const auto &channel : result)
    {
        std::cout << "   " << channel.channelName_ << std::endl;
    }
    channel = GetChannel(result, "36");
    REQUIRE(channel.channelName_.find("Indoors") == std::string::npos);
}

struct RegulatoryDomain {
    RegulatoryDomain(int32_t domain) : domain(domain) { }
    RegulatoryDomain(int32_t domain,const std::vector<int32_t>&channels) : domain(domain),channels(channels) { }
    void AddChannel(int32_t channel)
    {
        std::vector<int32_t>::iterator i;
        for (i = channels.begin(); i != channels.end(); ++i)
        {
            if ((*i) == channel) return;
            if (*i > channel) break;
        }
        channels.insert(i,channel);
    }
    bool operator==(const RegulatoryDomain&other) const
    {
        if (this->domain != other.domain) return false;
        return std::equal(channels.begin(),channels.end(),other.channels.begin(),other.channels.end());
    }
    int32_t domain;
    std::vector<int32_t> channels;
};

class ChannelsByRegDomain {
public:
    ChannelsByRegDomain() { }
    ChannelsByRegDomain(const std::vector<RegulatoryDomain>&domains): regulations(domains) { }
    void AddChannel(int32_t domain, int32_t channel)
    {
        RegulatoryDomain&regDomain = GetDomain(domain);
        regDomain.AddChannel(channel);
    }
    std::string toString() const {
        std::stringstream s;
        bool first = true;
        for (const auto&domain: regulations)
        {
            if (!first) s << " ";
            first = false;
            s << domain.domain << ":";
            bool firstChan = true;
            for (const auto c : domain.channels)
            {
                if(!firstChan) s << ",";
                firstChan = false;

                s << c;
            }
        }
        return s.str();
    }
    bool operator==(const ChannelsByRegDomain&other) const
    {
        return std::equal(regulations.begin(),regulations.end(),other.regulations.begin(),other.regulations.end());
    }
private:
    RegulatoryDomain&GetDomain(int32_t domain) {

        std::vector<RegulatoryDomain>::iterator i;  
        for (i = regulations.begin(); i != regulations.end(); i++)
        {
            if (i->domain == domain) return *i;
            if (i->domain > domain) break;
        }
        i = regulations.insert(i,RegulatoryDomain(domain));
        return *i;

    }
    std::vector<RegulatoryDomain> regulations;
};

void DisplayP2pChannels(const char*isoCountry, const ChannelsByRegDomain&expected)
{
    std::cout << "   " << isoCountry <<":";

    ChannelsByRegDomain result;
    auto wifiInfo =  pipedal::getWifiInfo("CA");
    for (const auto&channel: wifiInfo.channels)
    {
        result.AddChannel(channel.regDomain,channel.channelNumber);
    }
    std::cout << result.toString() << std::endl;
    if (result != expected)
    {
        throw std::runtime_error("Incorrect results..");
    }

}

void TestBookwormRegDbLoad()
{
    std::filesystem::path dbPath = "test_data/debian_bookworm_regulatory.db";
    RegDb regdb{dbPath};
    REQUIRE(regdb.IsValid());
}
void TestP2pChannels()
{
    std::cout << "---- TestP2PChannels ---" << std::endl;

    // expecting (for CA)
    //81:1,2,3,4,5,6,7,8,9,10,11 115:36,40,44,48 116:36,44 117:40,48 124:149,153,157,161 125:149,153,157,161,165 126:149,157 
    //81:1,2,3,4,5,6,7,8,9,10,11 115:36,40,44,48 116:36,44 117:40,48 124:149,153,157,161 125:149,153,157,161,165 126:149,157 127:153,161
    // 127:153,161 128:36,40,44,48,149,153,157,161 130:36,40,44,48,149,153,157,161,165
    // 127:153,161


    ChannelsByRegDomain expectedCaDomains = 
    {
        std::vector<RegulatoryDomain>{
        {81, { 1,2,3,4,5,6,7,8,9,10,11}},
        { 115, {36,40,44,48}},
        { 116, {36,44}},
        { 117, { 40,48}},
        { 124, {149,153,157,161}},
        { 125, {149,153,157,161,165}},
        { 126, {149,157 }},
        { 127, {153,161}},
        // ignore 80ghz ac channels (which pi supports, but we don't).
        // { 128, {36,40,44,48,149,153,157,161}},
        // {  130, {36,40,44,48,149,153,157,161,165}}
        }
    };
    DisplayP2pChannels("CA",expectedCaDomains);
    // DisplayP2pChannels("US");
    // DisplayP2pChannels("JP");
    // DisplayP2pChannels("00");
    
}

static void DisplayRegulations(const std::string& country)
{
    using namespace std;
    RegDb&regDb = RegDb::GetInstance();

    cout << "    ---- " << country << " regulations --- " << endl;
    const auto& regulations = regDb.getWifiRegulations(country);
    for (const auto&rule: regulations.rules)
    {
        cout << rule.start_freq_khz/1000 << "-" << rule.end_freq_khz/1000 << " (" << rule.max_bandwidth_khz/1000 << ")";
        cout << (rule.HasFlag(RegRuleFlags::NO_INDOOR) ? " NO_INDOORS" : "")
                << (rule.HasFlag(RegRuleFlags::NO_OUTDOOR) ? " NO_OUTDOOR" : "")
                << (rule.HasFlag(RegRuleFlags::DFS) ? " RADAR" : "")
                << (rule.HasFlag(RegRuleFlags::NO_IR) ? " NO_IR" : "")
                << (rule.HasFlag(RegRuleFlags::NO_OFDM) ? " NO_OFDM" : "")
                << (rule.HasFlag(RegRuleFlags::NO_HE) ? " NO_HE" : "")
                << (rule.HasFlag(RegRuleFlags::NO_HT40MINUS) ? " NO_HT40MINUS" : "")
                << (rule.HasFlag(RegRuleFlags::NO_HT40PLUS) ? " NO_HT40PLUS" : "")
                << (rule.HasFlag(RegRuleFlags::AUTO_BW) ? " AUTO_BW" : "")
                << (rule.HasFlag(RegRuleFlags::PTP_ONLY) ? " PTP_ONLY" : "")
                << (rule.HasFlag(RegRuleFlags::PTMP_ONLY) ? " PTMP_ONLY" : "")
                << endl;
    }   
}

TEST_CASE("Wifi Channel Test", "[wifi_channels_test]")
{
    DisplayRegulations("CA");
    DisplayRegulations("US");
    DisplayRegulations("JP");
    TestBookwormRegDbLoad();
    TestWifiChannels();
    TestP2pChannels();
}
