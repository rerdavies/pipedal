
#include "pch.h"
#include <string>
#include <net/if.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>  
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/nl80211.h>

#include <linux/nl80211.h>
#include "PiPedalException.hpp"
#include "WifiChannels.hpp"
#include "Lv2Log.hpp"

#include "RegDb.hpp"
/*
   Heavily based on code from iw (1) command.
   https://kernel.googlesource.com/pub/scm/linux/kernel/git/jberg/iw/+/v0.9/COPYING

Copyright (c) 2007, 2008	Johannes Berg
Copyright (c) 2007		Andy Lutomirski
Copyright (c) 2007		Mike Kershaw
Copyright (c) 2008		Luis R. Rodriguez
All rights reserved.

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions
are met:
1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer. 
2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.
3. The name of the author may not be used to endorse or promote products
   derived from this software without specific prior written permission.
THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  
IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED
AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, 
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY
OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF
SUCH DAMAGE.
*/

using namespace pipedal;

struct MHzToChannel {
    char band;
    int channel;
    int mhz;
};

MHzToChannel mhzToChannels[] = {
{'g', 1, 2412},
{'g',2, 2417},
{'g',3, 2422},
{'g',4, 2427},
{'g',5, 2432},
{'g',6, 2437},
{'g',7, 2442},
{'g',8, 2447},
{'g',9, 2452},
{'g',10, 2457},
{'g',11, 2462},
{'g',12, 2467},
{'g',13, 2472},
{'g',14, 2484},
{'a',7, 5035},
{'a',8, 5040},
{'a',9, 5045},
{'a',11, 5055},
{'a',12, 5060},
{'a',16, 5080},
{'a',32, 5160},
{'a',34, 5170},
{'a',36, 5180},
{'a',38, 5190},
{'a',40, 5200},
{'a',42, 5210},
{'a',44, 5220},
{'a',46, 5230},
{'a',48, 5240},
{'a',50, 5250},
{'a',52, 5260},
{'a',54, 5270},
{'a',56, 5280},
{'a',58, 5290},
{'a',60, 5300},
{'a',62, 5310},
{'a',64, 5320},
{'a',68, 5340},
{'a',96, 5480},
{'a',100, 5500},
{'a',102, 5510},
{'a',104, 5520},
{'a',106, 5530},
{'a',108, 5540},
{'a',110, 5550},
{'a',112, 5560},
{'a',114, 5570},
{'a',116, 5580},
{'a',118, 5590},
{'a',120, 5600},
{'a',122, 5610},
{'a',124, 5620},
{'a',126, 5630},
{'a',128, 5640},
{'a',132, 5660},
{'a',134, 5670},
{'a',136, 5680},
{'a',138, 5690},
{'a',140, 5700},
{'a',142, 5710},
{'a',144, 5720},
{'a',149, 5745},
{'a',151, 5755},
{'a',153, 5765},
{'a',155, 5775},
{'a',157, 5785},
{'a',159, 5795},
{'a',161, 5805},
{'a',163, 5815},
{'a',165, 5825},
{'a',167, 5835},
{'a',169, 5845},
{'a',171, 5855},
{'a',173, 5865},
{'a',175, 5875},
{'a',177, 5885},
{'a',180, 5900},
{'a',182, 5910},
{'a',18, 5915},
{'a',184, 5920},
{'a',187, 5935},
{'a',188, 5940},
{'a',189, 5945},
{'a',192, 5960},
{'a',196, 5980},
};


bool MHzToChannelNumber(int mhz, char*pBand, int*pChannel)
{
    for (size_t i = 0; i < sizeof(mhzToChannels)/sizeof(mhzToChannels[0]); ++i)
    {
        if (std::abs(mhz-mhzToChannels[i].mhz) <= 1) {
            *pBand = mhzToChannels[i].band;
            *pChannel = mhzToChannels[i].channel;
            return true;
        }
    }
    return false;
}

static nla_policy *makeFreqPolicy()
{
    nla_policy* result = new nla_policy[NL80211_FREQUENCY_ATTR_MAX + 1];
    memset(result,0,sizeof(nla_policy[NL80211_FREQUENCY_ATTR_MAX + 1]));

    result[NL80211_FREQUENCY_ATTR_FREQ] = {NLA_U32,0,0};
    result[NL80211_FREQUENCY_ATTR_DISABLED] = {NLA_FLAG,0,0};
    result[NL80211_FREQUENCY_ATTR_PASSIVE_SCAN] = {NLA_FLAG,0,0};
    result[NL80211_FREQUENCY_ATTR_NO_IR] = {NLA_FLAG,0,0};
    result[NL80211_FREQUENCY_ATTR_RADAR] = {NLA_FLAG,0,0};
    result[NL80211_FREQUENCY_ATTR_INDOOR_ONLY] = {NLA_FLAG,0,0};
    result[NL80211_FREQUENCY_ATTR_INDOOR_ONLY] = {NLA_FLAG,0,0};
    result[NL80211_FREQUENCY_ATTR_NO_20MHZ] = {NLA_FLAG,0,0};
    result[NL80211_FREQUENCY_ATTR_NO_20MHZ] = {NLA_FLAG,0,0};
	result[NL80211_FREQUENCY_ATTR_NO_HT40_MINUS] = {NLA_FLAG,0,0};
	result[NL80211_FREQUENCY_ATTR_NO_HT40_PLUS] = {NLA_FLAG,0,0};
	result[NL80211_FREQUENCY_ATTR_NO_80MHZ] = {NLA_FLAG,0,0};
	result[NL80211_FREQUENCY_ATTR_NO_160MHZ] = {NLA_FLAG,0,0};
    return result;
}

static struct nla_policy* freq_policy = makeFreqPolicy();


static nla_policy *makeRatePolicy() {
    nla_policy* result = new nla_policy[NL80211_BITRATE_ATTR_MAX + 1];
    memset(result,0,sizeof(nla_policy[NL80211_BITRATE_ATTR_MAX + 1]));
    result[NL80211_BITRATE_ATTR_RATE] = {.type = NLA_U32};
    result[NL80211_BITRATE_ATTR_2GHZ_SHORTPREAMBLE] = {.type = NLA_FLAG};
    return result;
};

static struct nla_policy* rate_policy = makeRatePolicy();

struct IfType {
    enum nl80211_iftype iftype;
    const char*name;
};
static struct IfType ifTypes[] = {
	{NL80211_IFTYPE_UNSPECIFIED,"Unspecified"},
	{NL80211_IFTYPE_ADHOC,"Adhoc"},
	{NL80211_IFTYPE_STATION,"Station"},
	{NL80211_IFTYPE_AP,"AP"},
	{NL80211_IFTYPE_AP_VLAN,"VLAN"},
	{NL80211_IFTYPE_WDS,"WDS"},
	{NL80211_IFTYPE_MONITOR,"Monitor"},
	{NL80211_IFTYPE_MESH_POINT,"Mesh point"},
	{NL80211_IFTYPE_P2P_CLIENT,"Client"},
	{NL80211_IFTYPE_P2P_GO,"P2P Go"},
	{NL80211_IFTYPE_P2P_DEVICE,"P2P"},
	{NL80211_IFTYPE_OCB,"OCB"},
	{NL80211_IFTYPE_NAN,"NAN"},
    
};
const char *iftype_name(enum nl80211_iftype iftype)
{
    for (size_t i = 0; i < sizeof(ifTypes)/sizeof(ifTypes[0]); ++i)
    {
        if (ifTypes[i].iftype == iftype)
        {
            return ifTypes[i].name;
        }
    }
    return "Unknown";
}
static int phy_lookup(const char *name)
{
    char buf[200];
    int fd, pos;
    snprintf(buf, sizeof(buf), "/sys/class/ieee80211/%s/index", name);
    fd = open(buf, O_RDONLY);
    pos = read(fd, buf, sizeof(buf) - 1);
    if (pos < 0)
        return -1;
    buf[pos] = '\0';
    return atoi(buf);
}

static int getWifiName_callback(struct nl_msg *msg, void *arg);
static int getWifiInfo_callback(struct nl_msg *msg, void *arg);

struct WifiChannelInfo
{
    int channelNumber = -1;
    char band = 'g'; // 'g' or 'a'
    int mhz = 0;
    bool disabled = false;
    bool ir = true;
    bool radarDetection = false;
    std::vector<float> bitrates;
    bool indoorOnly = false;
    bool noHt40Minus = false;
    bool noHt40Plus = false;
    bool no10MHz = false;
    bool no20MHz = false;
    bool no80MHz = false;
    bool no160MHz = false;
};

struct WifiBand
{
    int index;
    std::vector<WifiChannelInfo> channels;
};

struct WifiInfo
{
    std::string name;
    std::vector<WifiBand> bands;
    std::vector<std::string> supportedIfTypes;
};
class NetLink
{
private:

    std::vector<WifiBand> result;
    nl_sock*socket = nullptr;
    nl_cache*cache = nullptr;
    genl_family*nl80211 = nullptr;

    int errorCode = -1;

    static int finish_handler(struct nl_msg *msg, void *arg)
    {
        NetLink*this_ = (NetLink*)arg;
        this_->errorCode = 0;
        return NL_SKIP;
    }
    static int ack_handler(struct nl_msg *msg, void *arg)
    {
        NetLink*this_ = (NetLink*)arg;
        this_->errorCode = 0;
        return NL_STOP;
    }

    static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
                void *arg)
    {
        NetLink*this_ = (NetLink*)arg;
        this_->errorCode = err->error;
        return NL_STOP;
    }


    static int info_phy_handler(struct nl_msg *msg, void *arg)
    {
        return ((NetLink *)arg)->InfoPhyHandler(msg, (WifiInfo *)arg);
    }
    int InfoPhyHandler(struct nl_msg *msg, WifiInfo *result)
    {
        struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
        struct genlmsghdr *gnlh = (genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
        struct nlattr *tb_band[NL80211_BAND_ATTR_MAX + 1];
        struct nlattr *tb_freq[NL80211_FREQUENCY_ATTR_MAX + 1];
        struct nlattr *tb_rate[NL80211_BITRATE_ATTR_MAX + 1];
        struct nlattr *nl_band;
        struct nlattr *nl_freq;
        struct nlattr *nl_rate;
        struct nlattr *nl_mode;
        int bandidx = 1;
        int rem_band, rem_freq, rem_rate, rem_mode;
        int open;
        nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
                  genlmsg_attrlen(gnlh, 0), NULL);
        if (!tb_msg[NL80211_ATTR_WIPHY_BANDS])
            return NL_SKIP;
        if (tb_msg[NL80211_ATTR_WIPHY_NAME])
            result->name = nla_get_string(tb_msg[NL80211_ATTR_WIPHY_NAME]);

        nla_for_each_nested(nl_band, tb_msg[NL80211_ATTR_WIPHY_BANDS], rem_band)
        {
            WifiBand band;
            band.index = bandidx;

            bandidx++;
            nla_parse(tb_band, NL80211_BAND_ATTR_MAX, (nlattr *)nla_data(nl_band),
                      nla_len(nl_band), NULL);

            nla_for_each_nested(nl_freq, tb_band[NL80211_BAND_ATTR_FREQS], rem_freq)
            {
                nla_parse(tb_freq, NL80211_FREQUENCY_ATTR_MAX, (nlattr *)nla_data(nl_freq),
                          nla_len(nl_freq), freq_policy);
                if (!tb_freq[NL80211_FREQUENCY_ATTR_FREQ])
                    continue;

                WifiChannelInfo channel;

                channel.mhz = nla_get_u32(tb_freq[NL80211_FREQUENCY_ATTR_FREQ]);
                open = 0;
                if (tb_freq[NL80211_FREQUENCY_ATTR_DISABLED])
                    channel.disabled = true;
                if (tb_freq[NL80211_FREQUENCY_ATTR_NO_IR])
                    channel.ir = false;
                if (tb_freq[NL80211_FREQUENCY_ATTR_RADAR])
                    channel.radarDetection = true;
                if (tb_freq[NL80211_FREQUENCY_ATTR_NO_10MHZ])
                    channel.no10MHz = true;
                if (tb_freq[NL80211_FREQUENCY_ATTR_INDOOR_ONLY])
                    channel.indoorOnly = true;
                if (tb_freq[NL80211_FREQUENCY_ATTR_NO_20MHZ])
                    channel.no20MHz = true;
                if (tb_freq[NL80211_FREQUENCY_ATTR_NO_HT40_MINUS])
                    channel.noHt40Minus = true;
                if (tb_freq[NL80211_FREQUENCY_ATTR_NO_HT40_PLUS])
                    channel.noHt40Plus = true;
                if (tb_freq[NL80211_FREQUENCY_ATTR_NO_80MHZ])
                    channel.no80MHz = true;
                if (tb_freq[NL80211_FREQUENCY_ATTR_NO_160MHZ])
                    channel.no160MHz = true;
                
                if (MHzToChannelNumber(channel.mhz,&channel.band, &channel.channelNumber))
                {
                    band.channels.push_back(std::move(channel));
                } else {
                    // throw PiPedalException("Frequency not recognized.");
                }
            }
            nla_for_each_nested(nl_rate, tb_band[NL80211_BAND_ATTR_RATES], rem_rate)
            {
                nla_parse(tb_rate, NL80211_BITRATE_ATTR_MAX, (nlattr *)nla_data(nl_rate),
                          nla_len(nl_rate), rate_policy);
                if (!tb_rate[NL80211_BITRATE_ATTR_RATE])
                    continue;
                //channel.bitRates.push_back(0.1f * nla_get_u32(tb_rate[NL80211_BITRATE_ATTR_RATE]);
            }
            result->bands.push_back(std::move(band));
        }
        if (!tb_msg[NL80211_ATTR_SUPPORTED_IFTYPES])
            return NL_SKIP;

        nla_for_each_nested(nl_mode, tb_msg[NL80211_ATTR_SUPPORTED_IFTYPES], rem_mode)
            result->supportedIfTypes.push_back(iftype_name((nl80211_iftype)(nl_mode->nla_type)));
        return NL_SKIP;
    }

public:
    ~NetLink()
    {
        if (this->cache)
        {
            nl_cache_free(cache);
            cache = nullptr;
        }
        if (nl80211 != nullptr)
        {
            genl_family_put(nl80211); // (release reference)
            nl80211 = nullptr;
        }
        if (socket)
        {
            nl_close(socket);
            nl_socket_free(socket);
            socket = nullptr;
        }
    }
    NetLink()
    {
        socket = nl_socket_alloc();
        if (!socket)
        {
            throw PiPedalException("Can't allocate netlink socket.");
        }
        nl_socket_set_buffer_size(socket, 8192, 8192);

        if (genl_connect(socket))
        {
            throw PiPedalException("Failed to connect to netlink socket.");
        }
        if (genl_ctrl_alloc_cache(this->socket,&this->cache) != 0) {
            throw PiPedalException("Failed to allocate generic netlink cache.");
        }
        this->nl80211 = genl_ctrl_search_by_name(this->cache, "nl80211");
        if (!this->nl80211) {
            throw PiPedalException("nl80211 family not found.");
        }
    }
    void GetAvailableChannels(const char *phyName, WifiInfo *result)
    {

        struct nl_msg *msg = nullptr;
        struct nl_cb*cb = nullptr;
	    int devidx = 0;
	    int err;

        devidx = phy_lookup(phyName);
        try {
        

        msg = nlmsg_alloc();
        if (!msg)
        {
            throw PiPedalException("failed to allocate netlink message");
        }
        cb = nl_cb_alloc(NL_CB_DEFAULT);
        if (!cb)
        {
            throw PiPedalException("failed to allocate netlink callbacks");
        }
        genlmsg_put(msg, 0, 0, genl_family_get_id(this->nl80211), 0,
                    0, NL80211_CMD_GET_WIPHY, 0);
        NLA_PUT_U32(msg, NL80211_ATTR_WIPHY, devidx);

        this->errorCode = 0;
        nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, info_phy_handler, result);

        nl_cb_err(cb, NL_CB_CUSTOM, error_handler, this);
        nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, this);
        nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, this);

        err = nl_send_auto_complete(this->socket, msg);
        
        if (err < 0)
            throw PiPedalException("Command failed.");
        nl_recvmsgs(this->socket, cb);
        nlmsg_free(msg);
        if (this->errorCode != 0)
        {
            throw PiPedalException("Command failed.");
        }
        nl_cb_put(cb);
        return;
    nla_put_failure: 
        throw PiPedalException("NLA Put failure.");
    } catch (const std::exception&e)
        {
            if (cb != nullptr)
            {
                nl_cb_put(cb);
                cb = nullptr;
            }
            if (msg != nullptr)
            {
                nlmsg_free(msg);
                msg = nullptr;
            }
            throw;
        }
    }
};

WifiInfo getWifiInfo(const char *phyName)
{
    int index = phy_lookup(phyName);
    if (index == -1)
    {
        throw PiPedalArgumentException("No such device.");
    }

    NetLink netLink;
    WifiInfo result;
    netLink.GetAvailableChannels(phyName,&result);
    return result;
}

const char*allowOutdoorsLocales[] = {
    "US","TW","SG",

};

static bool allowOutdoors(const char*countryCode)
{
    for (size_t i = 0; i < sizeof(allowOutdoorsLocales)/sizeof(allowOutdoorsLocales[0]); ++i)
    {
        if (strcmp(allowOutdoorsLocales[i],countryCode) == 0) return true;
    }
    return false;
}

static bool firstTime = true;
static std::unique_ptr<RegDb> g_regDb;
std::vector<WifiChannel> pipedal::getWifiChannels(const char*countryIso3661)
{
    // Use the wifi physical device to get a list of channels
    // that the hardware supports.
    // The override the hardware's regulation flags using regulation.bin
    // database info for the selected country code.
    // If the regulation.bin database doesn't have a matching record,
    // just pass the data that the wifi reported.

    if (firstTime) {
        firstTime = false;
        try {
            g_regDb = std::make_unique<RegDb>();
        } catch (const std::exception& e)
        {
            std::stringstream s;
            s << "Failed to open the Wi-Fi regulations.bin database. " << e.what();
            Lv2Log::error(s.str());
        }
    }
    std::vector<WifiChannel> result;

    WifiInfo wifiInfo = getWifiInfo("phy0");
    std::unique_ptr<CrdbRules> crdbRules = nullptr;
    if (g_regDb) {
        crdbRules = std::unique_ptr<CrdbRules>(g_regDb->GetCrdbRules(countryIso3661));
        if (!crdbRules) {
            crdbRules = std::unique_ptr<CrdbRules>(g_regDb->GetCrdbRules("00")); // unset default.
        }
    }
    for (auto &band : wifiInfo.bands)
    {
        for (auto &channel: band.channels) {
            if (crdbRules)
            {
                FrequencyRule crdbRule;
                if (crdbRules->GetFrequencyRule(channel.mhz,&crdbRule))
                {
                    channel.disabled = false;
                    channel.radarDetection = crdbRule.hasFlag(RegRuleFlags::DFS);
                    channel.ir = !crdbRule.hasFlag(RegRuleFlags::NO_IR);
                    channel.indoorOnly = crdbRule.hasFlag(RegRuleFlags::NO_OUTDOOR);
                    if (crdbRule.hasFlag(RegRuleFlags::PTP_ONLY)
                    || crdbRule.hasFlag(RegRuleFlags::PTMP_ONLY))
                    {
                        channel.disabled = true;

                    }
                } else {
                    channel.disabled = true;
                }
            }
            if ((!channel.disabled) && (!channel.radarDetection) && channel.ir)
            {
                WifiChannel ch;
                std::stringstream s;
                s << channel.band << channel.channelNumber;
                ch.channelId_ = s.str();

                std::stringstream t;
                t << channel.channelNumber;
                if (channel.band == 'a') 
                    t << " (5GHz)";
                else 
                    t << " (2.4GHz)";
                
                if (channel.indoorOnly) {
                    t << " Indoor only";
                } 
                ch.channelName_ = t.str();
                result.push_back(ch);
            }
        }
    }
    return result;
}


JSON_MAP_BEGIN(WifiChannel)
    JSON_MAP_REFERENCE(WifiChannel,channelId)
    JSON_MAP_REFERENCE(WifiChannel,channelName)
JSON_MAP_END()


