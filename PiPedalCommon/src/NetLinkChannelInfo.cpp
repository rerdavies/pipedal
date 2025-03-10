/*
Copyright (c) 2007, 2008	Johannes Berg
Copyright (c) 2007		Andy Lutomirski
Copyright (c) 2007		Mike Kershaw
Copyright (c) 2008		Luis R. Rodriguez
Copyright (c) 2024      Robin E. R. Davies
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
/*
   Heavily based on code borrowed from iw (1) command.
   https://kernel.googlesource.com/pub/scm/linux/kernel/git/jberg/iw/+/v0.9/COPYING
*/

// code lifed from WPA_SUPPLICANT
/*
Copyright (c) 2002-2019, Jouni Malinen <j@w1.fi>

This software may be distributed, used, and modified under the terms of
BSD license:

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are
met:

1. Redistributions of source code must retain the above copyright
   notice, this list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright
   notice, this list of conditions and the following disclaimer in the
   documentation and/or other materials provided with the distribution.

3. Neither the name(s) of the above-listed copyright holder(s) nor the
   names of its contributors may be used to endorse or promote products
   derived from this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
"AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*/

#include "NetLinkChannelInfo.hpp"
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <net/if.h>
#include <netlink/netlink.h>
#include <netlink/genl/genl.h>
#include <netlink/genl/family.h>
#include <netlink/genl/ctrl.h>
#include <netlink/msg.h>
#include <netlink/attr.h>
#include <linux/nl80211.h>
#include <stdexcept>
#include "ss.hpp"

using namespace pipedal;

namespace pipedal::impl
{
    class NetLink
    {
    private:
        bool responseReceived = false;
        bool ackReceived = false;
        WifiInfo info;
        WifiRegulations regulations;
        nl_sock *socket = nullptr;
        nl_cache *cache = nullptr;
        genl_family *nl80211 = nullptr;

        int errorCode = -1;

        static int finish_handler(struct nl_msg *msg, void *arg);
        static int ack_handler(struct nl_msg *msg, void *arg);

        static int error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
                                 void *arg);

        static int info_handler(struct nl_msg *msg, void *arg);
        static int reg_handler(struct nl_msg *msg, void *arg);
        int InfoHandler(struct nl_msg *msg);
        int RegHandler(struct nl_msg *msg);

    public:
        ~NetLink();
        NetLink();

        WifiInfo GetAvailableChannels(const char *phyName, const char *countryregode);
        WifiRegulations GetRegulations(const char *phyName, const char *countryCode);
    };

}

using namespace pipedal::impl;


struct ChannelInfo
{
    int32_t freq;
    WifiBandwidth chanWidth;
    SecondaryChannelLocation secondaryChannelLocation;
    int32_t op_class;
    int32_t channel;
    WifiMode hardwareMode;
};

bool ieeeChannelInfo(unsigned int freq,
                     WifiBandwidth chanwidth,
                     ChannelInfo *channelInfo);

struct NlMHzToChannel
{
    char band;
    int channel;
    int mhz;
};

static std::vector<NlMHzToChannel> mhzToChannels = {
    {'g', 1, 2412},
    {'g', 2, 2417},
    {'g', 3, 2422},
    {'g', 4, 2427},
    {'g', 5, 2432},
    {'g', 6, 2437},
    {'g', 7, 2442},
    {'g', 8, 2447},
    {'g', 9, 2452},
    {'g', 10, 2457},
    {'g', 11, 2462},
    {'g', 12, 2467},
    {'g', 13, 2472},
    {'g', 14, 2484},
    {'a', 7, 5035},
    {'a', 8, 5040},
    {'a', 9, 5045},
    {'a', 11, 5055},
    {'a', 12, 5060},
    {'a', 16, 5080},
    {'a', 32, 5160},
    {'a', 34, 5170},
    {'a', 36, 5180},
    {'a', 38, 5190},
    {'a', 40, 5200},
    {'a', 42, 5210},
    {'a', 44, 5220},
    {'a', 46, 5230},
    {'a', 48, 5240},
    {'a', 50, 5250},
    {'a', 52, 5260},
    {'a', 54, 5270},
    {'a', 56, 5280},
    {'a', 58, 5290},
    {'a', 60, 5300},
    {'a', 62, 5310},
    {'a', 64, 5320},
    {'a', 68, 5340},
    {'a', 96, 5480},
    {'a', 100, 5500},
    {'a', 102, 5510},
    {'a', 104, 5520},
    {'a', 106, 5530},
    {'a', 108, 5540},
    {'a', 110, 5550},
    {'a', 112, 5560},
    {'a', 114, 5570},
    {'a', 116, 5580},
    {'a', 118, 5590},
    {'a', 120, 5600},
    {'a', 122, 5610},
    {'a', 124, 5620},
    {'a', 126, 5630},
    {'a', 128, 5640},
    {'a', 132, 5660},
    {'a', 134, 5670},
    {'a', 136, 5680},
    {'a', 138, 5690},
    {'a', 140, 5700},
    {'a', 142, 5710},
    {'a', 144, 5720},
    {'a', 149, 5745},
    {'a', 151, 5755},
    {'a', 153, 5765},
    {'a', 155, 5775},
    {'a', 157, 5785},
    {'a', 159, 5795},
    {'a', 161, 5805},
    {'a', 163, 5815},
    {'a', 165, 5825},
    {'a', 167, 5835},
    {'a', 169, 5845},
    {'a', 171, 5855},
    {'a', 173, 5865},
    {'a', 175, 5875},
    {'a', 177, 5885},
    {'a', 180, 5900},
    {'a', 182, 5910},
    {'a', 183, 5915},
    {'a', 184, 5920},
    {'a', 187, 5935},
    {'a', 188, 5940},
    {'a', 189, 5945},
    {'a', 192, 5960},
    {'a', 196, 5980},
};

static bool MHzToChannelNumber(int mhz, char *pBand, int *pChannel)
{
    for (size_t i = 0; i < mhzToChannels.size(); ++i)
    {
        if (std::abs(mhz - mhzToChannels[i].mhz) <= 1)
        {
            *pBand = mhzToChannels[i].band;
            *pChannel = mhzToChannels[i].channel;
            return true;
        }
    }
    return false;
}

static nla_policy *makeFreqPolicy()
{
    nla_policy *result = new nla_policy[NL80211_FREQUENCY_ATTR_MAX + 1];
    memset(result, 0, sizeof(nla_policy[NL80211_FREQUENCY_ATTR_MAX + 1]));

    result[NL80211_FREQUENCY_ATTR_FREQ] = {NLA_U32, 0, 0};
    result[NL80211_FREQUENCY_ATTR_DISABLED] = {NLA_FLAG, 0, 0};
    result[NL80211_FREQUENCY_ATTR_PASSIVE_SCAN] = {NLA_FLAG, 0, 0};
    result[NL80211_FREQUENCY_ATTR_NO_IR] = {NLA_FLAG, 0, 0};
    result[NL80211_FREQUENCY_ATTR_RADAR] = {NLA_FLAG, 0, 0};
    result[NL80211_FREQUENCY_ATTR_INDOOR_ONLY] = {NLA_FLAG, 0, 0};
    result[NL80211_FREQUENCY_ATTR_INDOOR_ONLY] = {NLA_FLAG, 0, 0};
    result[NL80211_FREQUENCY_ATTR_NO_20MHZ] = {NLA_FLAG, 0, 0};
    result[NL80211_FREQUENCY_ATTR_NO_20MHZ] = {NLA_FLAG, 0, 0};
    result[NL80211_FREQUENCY_ATTR_NO_HT40_MINUS] = {NLA_FLAG, 0, 0};
    result[NL80211_FREQUENCY_ATTR_NO_HT40_PLUS] = {NLA_FLAG, 0, 0};
    result[NL80211_FREQUENCY_ATTR_NO_80MHZ] = {NLA_FLAG, 0, 0};
    result[NL80211_FREQUENCY_ATTR_NO_160MHZ] = {NLA_FLAG, 0, 0};
    return result;
}

static struct nla_policy *freq_policy = makeFreqPolicy();

static nla_policy *makeRatePolicy()
{
    nla_policy *result = new nla_policy[NL80211_BITRATE_ATTR_MAX + 1];
    memset(result, 0, sizeof(nla_policy[NL80211_BITRATE_ATTR_MAX + 1]));
    result[NL80211_BITRATE_ATTR_RATE] = {.type = NLA_U32};
    result[NL80211_BITRATE_ATTR_2GHZ_SHORTPREAMBLE] = {.type = NLA_FLAG};
    return result;
};

static struct nla_policy *rate_policy = makeRatePolicy();

struct IfType
{
    enum nl80211_iftype iftype;
    const char *name;
};
static struct IfType ifTypes[] = {
    {NL80211_IFTYPE_UNSPECIFIED, "Unspecified"},
    {NL80211_IFTYPE_ADHOC, "Adhoc"},
    {NL80211_IFTYPE_STATION, "Station"},
    {NL80211_IFTYPE_AP, "AP"},
    {NL80211_IFTYPE_AP_VLAN, "VLAN"},
    {NL80211_IFTYPE_WDS, "WDS"},
    {NL80211_IFTYPE_MONITOR, "Monitor"},
    {NL80211_IFTYPE_MESH_POINT, "Mesh point"},
    {NL80211_IFTYPE_P2P_CLIENT, "Client"},
    {NL80211_IFTYPE_P2P_GO, "P2P Go"},
    {NL80211_IFTYPE_P2P_DEVICE, "P2P"},
    {NL80211_IFTYPE_OCB, "OCB"},
    {NL80211_IFTYPE_NAN, "NAN"},

};
static const char *iftype_name(enum nl80211_iftype iftype)
{
    for (size_t i = 0; i < sizeof(ifTypes) / sizeof(ifTypes[0]); ++i)
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

WifiInfo pipedal::getWifiInfo(const char *phyName, const char *isoCountryCode)
{
    int index = phy_lookup(phyName);
    try
    {
        if (index == -1)
        {
            throw std::runtime_error("No such device.");
        }

        NetLink netLink;
        netLink.GetRegulations(phyName, isoCountryCode);
        WifiInfo result = netLink.GetAvailableChannels(phyName, isoCountryCode);
        return result;
    }
    catch (const std::exception &e)
    {
        throw std::runtime_error(SS("Failed to get Wifi info from nl80211. " << e.what()));
    }
}

const char *allowOutdoorsLocales[] = {
    "US",
    "TW",
    "SG",

};

static bool allowOutdoors(const char *countryCode)
{
    for (size_t i = 0; i < sizeof(allowOutdoorsLocales) / sizeof(allowOutdoorsLocales[0]); ++i)
    {
        if (strcmp(allowOutdoorsLocales[i], countryCode) == 0)
            return true;
    }
    return false;
}

int NetLink::finish_handler(struct nl_msg *msg, void *arg)
{
    NetLink *this_ = (NetLink *)arg;
    this_->responseReceived = true;
    return NL_SKIP;
}
int NetLink::ack_handler(struct nl_msg *msg, void *arg)
{
    NetLink *this_ = (NetLink *)arg;
    this_->ackReceived = true;
    return NL_STOP;
}

int NetLink::error_handler(struct sockaddr_nl *nla, struct nlmsgerr *err,
                           void *arg)
{
    NetLink *this_ = (NetLink *)arg;
    this_->errorCode = err->error;
    this_->responseReceived = true;

    return NL_STOP;
}

int NetLink::reg_handler(struct nl_msg *msg, void *arg)
{
    return ((NetLink *)arg)->RegHandler(msg);
}

int NetLink::info_handler(struct nl_msg *msg, void *arg)
{
    return ((NetLink *)arg)->InfoHandler(msg);
}
int NetLink::InfoHandler(struct nl_msg *msg)
{
    this->responseReceived = true;
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
    // if (tb_msg[NL80211_ATTR_WIPHY_NAME])
    //     this->info.name = nla_get_string(tb_msg[NL80211_ATTR_WIPHY_NAME]);
    nla_for_each_nested(nl_band, tb_msg[NL80211_ATTR_WIPHY_BANDS], rem_band)
    {
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
            channel.ir = true;
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

            ChannelInfo channelInfo;
            if (ieeeChannelInfo(channel.mhz,
                                WifiBandwidth::BW20,
                                &channelInfo))
            {
                channel.hardwareMode = channelInfo.hardwareMode;
                channel.channelNumber = channelInfo.channel;

                this->info.channels.push_back(std::move(channel));
            }
            else
            {
                // throw std::runtime_error("Frequency not recognized.");
            }
        }
        nla_for_each_nested(nl_rate, tb_band[NL80211_BAND_ATTR_RATES], rem_rate)
        {
            nla_parse(tb_rate, NL80211_BITRATE_ATTR_MAX, (nlattr *)nla_data(nl_rate),
                      nla_len(nl_rate), rate_policy);
            if (!tb_rate[NL80211_BITRATE_ATTR_RATE])
                continue;
            // channel.bitRates.push_back(0.1f * nla_get_u32(tb_rate[NL80211_BITRATE_ATTR_RATE]);
        }
    }
    if (!tb_msg[NL80211_ATTR_SUPPORTED_IFTYPES])
        return NL_SKIP;

    nla_for_each_nested(nl_mode, tb_msg[NL80211_ATTR_SUPPORTED_IFTYPES], rem_mode)
        this->info.supportedIfTypes.push_back(iftype_name((nl80211_iftype)(nl_mode->nla_type)));
    return NL_SKIP;
}
int NetLink::RegHandler(struct nl_msg *msg)
{
    this->responseReceived = true;

    struct nlattr *tb_msg[NL80211_ATTR_MAX + 1];
    struct genlmsghdr *gnlh = (genlmsghdr *)nlmsg_data(nlmsg_hdr(msg));
    struct nlattr *tb_rule[NL80211_REG_RULE_ATTR_MAX + 1];
    struct nlattr *tb_freq[NL80211_FREQUENCY_ATTR_MAX + 1];
    struct nlattr *tb_rate[NL80211_BITRATE_ATTR_MAX + 1];
    struct nlattr *nl_rule;
    struct nlattr *nl_freq;
    struct nlattr *nl_rate;
    struct nlattr *nl_mode;
    int ruleidx = 1;
    int rem_rules, rem_freq, rem_rate, rem_mode;
    int open;
    nla_parse(tb_msg, NL80211_ATTR_MAX, genlmsg_attrdata(gnlh, 0),
              genlmsg_attrlen(gnlh, 0), NULL);
    if (!tb_msg[NL80211_ATTR_REG_RULES])
        return NL_SKIP;
    if (tb_msg[NL80211_ATTR_REG_ALPHA2])
        this->regulations.reg_alpha2 = nla_get_string(tb_msg[NL80211_ATTR_REG_ALPHA2]);
    nla_for_each_nested(nl_rule, tb_msg[NL80211_ATTR_REG_RULES], rem_rules)
    {
        WifiRule rule;

        nla_parse(tb_rule, NL80211_REG_RULE_ATTR_MAX, (nlattr *)nla_data(nl_rule),
                  nla_len(nl_rule), NULL);
        rule.flags = (RegRuleFlags)(nla_get_u32(tb_rule[NL80211_ATTR_REG_RULE_FLAGS]));
        rule.start_freq_khz = nla_get_u32(tb_rule[NL80211_ATTR_FREQ_RANGE_START]);
        rule.end_freq_khz = nla_get_u32(tb_rule[NL80211_ATTR_FREQ_RANGE_END]);
        rule.max_bandwidth_khz = nla_get_u32(tb_rule[NL80211_ATTR_FREQ_RANGE_MAX_BW]);
        rule.max_antenna_gain = nla_get_u32(tb_rule[NL80211_ATTR_POWER_RULE_MAX_ANT_GAIN]);
        rule.max_eirp = nla_get_u32(tb_rule[NL80211_ATTR_POWER_RULE_MAX_EIRP]);
        rule.dfs_cac_ms = nla_get_u32(tb_rule[NL80211_ATTR_DFS_CAC_TIME]);
        // not available in bookworm
        // if (tb_rule[NL80211_ATTR_POWER_RULE_PSD])
        // {
        //     rule.psd = nla_get_s8(tb_rule[NL80211_ATTR_POWER_RULE_PSD]);
        // };

        this->regulations.rules.push_back(rule);
    }
    return NL_SKIP;
}

NetLink::~NetLink()
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

NetLink::NetLink()
{
    socket = nl_socket_alloc();
    if (!socket)
    {
        throw std::runtime_error("Can't allocate netlink socket.");
    }
    nl_socket_set_buffer_size(socket, 8192, 8192);

    if (genl_connect(socket))
    {
        throw std::runtime_error("Failed to connect to netlink socket.");
    }
    if (genl_ctrl_alloc_cache(this->socket, &this->cache) != 0)
    {
        throw std::runtime_error("Failed to allocate generic netlink cache.");
    }
    this->nl80211 = genl_ctrl_search_by_name(this->cache, "nl80211");
    if (!this->nl80211)
    {
        throw std::runtime_error("nl80211 family not found.");
    }
}

WifiRegulations NetLink::GetRegulations(const char *phyName, const char *countryCode)
{

    struct nl_msg *msg = nullptr;
    struct nl_cb *cb = nullptr;
    int devidx = 0;
    int err;

    devidx = phy_lookup(phyName);
    try
    {

        msg = nlmsg_alloc();
        if (!msg)
        {
            throw std::runtime_error("failed to allocate netlink message");
        }
        cb = nl_cb_alloc(NL_CB_DEFAULT);
        if (!cb)
        {
            throw std::runtime_error("failed to allocate netlink callbacks");
        }
        genlmsg_put(msg, 0, 0, genl_family_get_id(this->nl80211), 0,
                    0, NL80211_CMD_GET_REG, 0);
        NLA_PUT_U32(msg, NL80211_ATTR_WIPHY, devidx);
        // NLA_PUT_STRING(msg, NL80211_ATTR_REG_ALPHA2, countryCode);

        nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, reg_handler, this);

        nl_cb_err(cb, NL_CB_CUSTOM, error_handler, this);
        nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, this);
        nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, this);

        this->errorCode = 0;

        err = nl_send_auto(this->socket, msg);
        msg = nullptr;

        if (err < 0)
            throw std::runtime_error("Send failed.");
        responseReceived = false;
        ackReceived = false;
        while (!responseReceived || !ackReceived)
        {
            err = nl_recvmsgs_report(this->socket, cb);
            if (err < 0)
            {
                throw std::runtime_error("Receive failed.");
            }
        }
        nlmsg_free(msg);
        msg = nullptr;
        if (this->errorCode != 0)
        {
            throw std::runtime_error("Command failed.");
        }
        nl_cb_put(cb);
        cb = nullptr;
        return this->regulations;
    nla_put_failure:
        throw std::runtime_error("NLA Put failure.");
    }
    catch (const std::exception &e)
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

WifiInfo NetLink::GetAvailableChannels(const char *phyName, const char *countryCode)
{

    struct nl_msg *msg = nullptr;
    struct nl_cb *cb = nullptr;
    int devidx = 0;
    int err;

    devidx = phy_lookup(phyName);
    try
    {

        msg = nlmsg_alloc();
        if (!msg)
        {
            throw std::runtime_error("failed to allocate netlink message");
        }
        cb = nl_cb_alloc(NL_CB_DEFAULT);
        if (!cb)
        {
            throw std::runtime_error("failed to allocate netlink callbacks");
        }
        genlmsg_put(msg, 0, 0, genl_family_get_id(this->nl80211), 0,
                    0, NL80211_CMD_GET_WIPHY, 0);
        NLA_PUT_U32(msg, NL80211_ATTR_WIPHY, devidx);
        // NLA_PUT_STRING(msg, NL80211_ATTR_REG_ALPHA2, countryCode);

        this->errorCode = 0;
        nl_cb_set(cb, NL_CB_VALID, NL_CB_CUSTOM, info_handler, this);

        nl_cb_err(cb, NL_CB_CUSTOM, error_handler, this);
        nl_cb_set(cb, NL_CB_FINISH, NL_CB_CUSTOM, finish_handler, this);
        nl_cb_set(cb, NL_CB_ACK, NL_CB_CUSTOM, ack_handler, this);

        err = nl_send_auto(this->socket, msg);

        if (err < 0)
            throw std::runtime_error("Command failed.");
        responseReceived = false;

        ackReceived = false;
        while (!responseReceived || !ackReceived)
        {
            err = nl_recvmsgs_report(this->socket, cb);

            if (err < 0)
                throw std::runtime_error("Receive failed.");
        }

        nlmsg_free(msg);
        msg = nullptr;
        if (this->errorCode != 0)
        {
            throw std::runtime_error("Command failed.");
        }
        nl_cb_put(cb);
        cb = nullptr;
        return info;
    nla_put_failure:
        throw std::runtime_error("NLA Put failure.");
    }
    catch (const std::exception &e)
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

const WifiChannelInfo *WifiInfo::getChannelInfo(int channel) const
{
    for (const auto &band : bands)
    {
        for (const auto &channelInfo : band.channels)
        {
            if (channelInfo.channelNumber == channel)
            {
                return &channelInfo;
            }
        }
    }
    return nullptr;
}

struct P2POpClassMap
{
    WifiMode band;
    int32_t regDomain;
    int32_t firstChannel;
    int32_t lastChannel;
    int32_t increment;
    WifiBandwidth bandwidth;
    bool p2p_support;
};
// from wpa_supplicant.
static std::vector<P2POpClassMap> global_op_class = {
    {WifiMode::IEEE80211G, 81, 1, 13, 1, WifiBandwidth::BW20, true},
    {WifiMode::IEEE80211G, 82, 14, 14, 1, WifiBandwidth::BW20, false},

    /* Do not enable HT40 on 2.4 GHz for P2P use for now */
    {WifiMode::IEEE80211G, 83, 1, 9, 1, WifiBandwidth::BW40PLUS, false},
    {WifiMode::IEEE80211G, 84, 5, 13, 1, WifiBandwidth::BW40MINUS, false},

    {WifiMode::IEEE80211A, 115, 36, 48, 4, WifiBandwidth::BW20, true},
    {WifiMode::IEEE80211A, 116, 36, 44, 8, WifiBandwidth::BW40PLUS, true},
    {WifiMode::IEEE80211A, 117, 40, 48, 8, WifiBandwidth::BW40MINUS, true},
    {WifiMode::IEEE80211A, 118, 52, 64, 4, WifiBandwidth::BW20, false},
    {WifiMode::IEEE80211A, 119, 52, 60, 8, WifiBandwidth::BW40PLUS, false},
    {WifiMode::IEEE80211A, 120, 56, 64, 8, WifiBandwidth::BW40MINUS, false},
    {WifiMode::IEEE80211A, 121, 100, 140, 4, WifiBandwidth::BW20, false},
    {WifiMode::IEEE80211A, 122, 100, 132, 8, WifiBandwidth::BW40PLUS, false},
    {WifiMode::IEEE80211A, 123, 104, 136, 8, WifiBandwidth::BW40MINUS, false},
    {WifiMode::IEEE80211A, 124, 149, 161, 4, WifiBandwidth::BW20, true},
    {WifiMode::IEEE80211A, 125, 149, 177, 4, WifiBandwidth::BW20, true},
    {WifiMode::IEEE80211A, 126, 149, 173, 8, WifiBandwidth::BW40PLUS, true},
    {WifiMode::IEEE80211A, 127, 153, 177, 8, WifiBandwidth::BW40MINUS, true},

    /*
     * IEEE P802.11ax/D8.0 (WIFI 6)Table E-4 actually talks about channel center
     * frequency index 42, 58, 106, 122, 138, 155, 171 with channel spacing
     * of 80 MHz, but currently use the following definition for simplicity
     * (these center frequencies are not actual channels, which makes
     * wpas_p2p_verify_channel() fail). wpas_p2p_verify_80mhz() should take
     * care of removing invalid channels.
     */
    {WifiMode::IEEE80211AX, 128, 36, 177, 4, WifiBandwidth::BW80, true},
    {WifiMode::IEEE80211AX, 129, 36, 177, 4, WifiBandwidth::BW160, true},
    {WifiMode::IEEE80211AX, 130, 36, 177, 4, WifiBandwidth::BW80P80, true},
    {WifiMode::IEEE80211AX, 131, 1, 233, 4, WifiBandwidth::BW20, true},
    {WifiMode::IEEE80211AX, 132, 1, 233, 8, WifiBandwidth::BW40, true},
    {WifiMode::IEEE80211AX, 133, 1, 233, 16, WifiBandwidth::BW80, true},
    {WifiMode::IEEE80211AX, 134, 1, 233, 32, WifiBandwidth::BW160, true},
    {WifiMode::IEEE80211AX, 135, 1, 233, 16, WifiBandwidth::BW80P80, false},
    {WifiMode::IEEE80211AX, 136, 2, 2, 4, WifiBandwidth::BW20, false},

    /*
     * IEEE Std 802.11ad-2012 and P802.ay/D5.0 60 GHz operating classes.
     * Class 180 has the legacy channels 1-6. Classes 181-183 include
     * channels which implement channel bonding features.
     */
    {WifiMode::IEEE80211AD, 180, 1, 6, 1, WifiBandwidth::BW2160, true},
    {WifiMode::IEEE80211AD, 181, 9, 13, 1, WifiBandwidth::BW4320, true},
    {WifiMode::IEEE80211AD, 182, 17, 20, 1, WifiBandwidth::BW6480, true},
    {WifiMode::IEEE80211AD, 183, 25, 27, 1, WifiBandwidth::BW8640, true},

    /* Keep the operating class 130 as the last entry as a workaround for
     * the OneHundredAndThirty Delimiter value used in the Supported
     * Operating Classes element to indicate the end of the Operating
     * Classes field. */
};

std::vector<WifiP2PChannelInfo> pipedal::getWifiP2pInfo(const char *phyName, const char *isoCountryCode)
{
    WifiInfo info = getWifiInfo(phyName, isoCountryCode);
    std::vector<WifiP2PChannelInfo> result;

    for (const P2POpClassMap &classMap : global_op_class)
    {
        if (!classMap.p2p_support)
        {
            continue;
        }
        if (classMap.band != WifiMode::IEEE80211A && classMap.band != WifiMode::IEEE80211G)
        {
            continue;
        }
        for (auto channel = classMap.firstChannel; channel <= classMap.lastChannel; channel += classMap.increment)
        {
            const auto *channelInfo = info.getChannelInfo(channel);
            if (!channelInfo)
                continue;
            WifiP2PChannelInfo t;
            // slice copy the channelInfo
            (WifiChannelInfo &)t = *channelInfo;

            ChannelInfo p2pInfo;
            if (ieeeChannelInfo(channelInfo->mhz,
                                classMap.bandwidth,
                                &p2pInfo))
            {
                switch (p2pInfo.hardwareMode)
                {
                case WifiMode::IEEE80211A:
                case WifiMode::IEEE80211B:
                case WifiMode::IEEE80211G:
                    t.regDomain = p2pInfo.op_class;
                    result.push_back(std::move(t));
                    break;
                }
            }
            else
            {
                // throw std::runtime_error("Frequency not recognized.");
            }
            t.regDomain = classMap.regDomain;
            result.push_back(t);
        }
    }
    return result;
}

/**
 * ieee80211_freq_to_channel_ext - Convert frequency into channel info
 * for HT40, VHT, and HE. DFS channels are not covered.
 * @freq: Frequency (MHz) to convert
 * @sec_channel: 0 = non-HT40, 1 = sec. channel above, -1 = sec. channel below
 * @chanwidth: VHT/EDMG channel width (CHANWIDTH_*)
 * @op_class: Buffer for returning operating class
 * @channel: Buffer for returning channel number
 * Returns: hw_mode on success, NUM_HOSTAPD_MODES on failure
 */

bool ieeeChannelInfo(unsigned int freq,
                     WifiBandwidth chanwidth,
                     ChannelInfo *channelInfo)
{

    // convert to the convetions used by original code.
    SecondaryChannelLocation sec_channel = SecondaryChannelLocation::None;
    if (chanwidth == WifiBandwidth::BW40MINUS)
    {
        chanwidth = WifiBandwidth::BW40;
        sec_channel = SecondaryChannelLocation::Below;
    }
    else if (chanwidth == WifiBandwidth::BW40PLUS)
    {
        chanwidth = WifiBandwidth::BW40;
        sec_channel = SecondaryChannelLocation::Above;
    }

    // based on wpa_supplicatn ieee801_11_common.c ieee80211_freq_to_channel_ext
    int32_t vht_opclass;

    /* TODO: more operating classes */

    channelInfo->freq = freq;
    channelInfo->chanWidth = chanwidth;

    if (freq >= 2412 && freq <= 2472)
    {
        if ((freq - 2407) % 5)
            return false;

        /* 2.407 GHz, channels 1..13 */
        if (sec_channel == SecondaryChannelLocation::Above)
            channelInfo->op_class = 83;
        else if (sec_channel == SecondaryChannelLocation::Below)
            channelInfo->op_class = 84;
        else
            channelInfo->op_class = 81;

        channelInfo->channel = (freq - 2407) / 5;
        channelInfo->hardwareMode = WifiMode::IEEE80211G;
        return true;
    }

    if (freq == 2484)
    {
        if (sec_channel != SecondaryChannelLocation::None || chanwidth > WifiBandwidth::BW40)
            return false;

        channelInfo->op_class = 82; /* channel 14 */
        channelInfo->channel = 14;

        channelInfo->hardwareMode = WifiMode::IEEE80211B;
        return true;
    }

    if (freq >= 4900 && freq < 5000)
    {
        if ((freq - 4000) % 5)
            return false;
        channelInfo->channel = (freq - 4000) / 5;
        channelInfo->op_class = 0; /* TODO */
        channelInfo->hardwareMode = WifiMode::IEEE80211A;
        return false;
    }

    switch (chanwidth)
    {
    case WifiBandwidth::BW80:
        vht_opclass = 128;
        break;
    case WifiBandwidth::BW160:
        vht_opclass = 129;
        break;
    case WifiBandwidth::BW80P80:
        vht_opclass = 130;
        break;
    default:
        vht_opclass = 0;
        break;
    }

    /* 5 GHz, channels 36..48 */
    if (freq >= 5180 && freq <= 5240)
    {
        if ((freq - 5000) % 5)
            return false;

        if (vht_opclass)
            channelInfo->op_class = vht_opclass;
        else if (sec_channel == SecondaryChannelLocation::Above)
            channelInfo->op_class = 116;
        else if (sec_channel == SecondaryChannelLocation::Below)
            channelInfo->op_class = 117;
        else
            channelInfo->op_class = 115;

        channelInfo->channel = (freq - 5000) / 5;

        channelInfo->hardwareMode = WifiMode::IEEE80211A;
        return true;
    }

    /* 5 GHz, channels 52..64 */
    if (freq >= 5260 && freq <= 5320)
    {
        if ((freq - 5000) % 5)
            return false;

        if (vht_opclass)
            channelInfo->op_class = vht_opclass;
        else if (sec_channel == SecondaryChannelLocation::Above)
            channelInfo->op_class = 119;
        else if (sec_channel == SecondaryChannelLocation::Below)
            channelInfo->op_class = 120;
        else
            channelInfo->op_class = 118;

        channelInfo->channel = (freq - 5000) / 5;

        channelInfo->hardwareMode = WifiMode::IEEE80211A;
        return true;
    }

    /* 5 GHz, channels 149..177 */
    if (freq >= 5745 && freq <= 5885)
    {
        if ((freq - 5000) % 5)
            return false;

        if (vht_opclass)
            channelInfo->op_class = vht_opclass;
        else if (sec_channel == SecondaryChannelLocation::Above)
            channelInfo->op_class = 126;
        else if (sec_channel == SecondaryChannelLocation::Below)
            channelInfo->op_class = 127;
        else if (freq <= 5805)
            channelInfo->op_class = 124;
        else
            channelInfo->op_class = 125;

        channelInfo->channel = (freq - 5000) / 5;

        channelInfo->hardwareMode = WifiMode::IEEE80211A;
        return true;
    }

    /* 5 GHz, channels 100..144 */
    if (freq >= 5500 && freq <= 5720)
    {
        if ((freq - 5000) % 5)
            return false;

        if (vht_opclass)
            channelInfo->op_class = vht_opclass;
        else if (sec_channel == SecondaryChannelLocation::Above)
            channelInfo->op_class = 122;
        else if (sec_channel == SecondaryChannelLocation::Below)
            channelInfo->op_class = 123;
        else
            channelInfo->op_class = 121;

        channelInfo->channel = (freq - 5000) / 5;

        channelInfo->hardwareMode = WifiMode::IEEE80211A;
        return true;
    }

    if (freq >= 5000 && freq < 5900)
    {
        if ((freq - 5000) % 5)
            return false;
        channelInfo->channel = (freq - 5000) / 5;
        channelInfo->op_class = 0; /* TODO */
        channelInfo->hardwareMode = WifiMode::IEEE80211A;
        return false; /* TODO */
    }

    if (freq > 5950 && freq <= 7115)
    {
        if ((freq - 5950) % 5)
            return false;

        switch (chanwidth)
        {
        case WifiBandwidth::BW80:
            channelInfo->op_class = 133;
            break;
        case WifiBandwidth::BW160:
            channelInfo->op_class = 134;
            break;
        case WifiBandwidth::BW80P80:
            channelInfo->op_class = 135;
            break;
        default:
            if (sec_channel == SecondaryChannelLocation::None)
                channelInfo->op_class = 132;
            else
                channelInfo->op_class = 131;
            break;
        }

        channelInfo->channel = (freq - 5950) / 5;
        channelInfo->hardwareMode = WifiMode::IEEE80211A;
        return true;
    }

    if (freq == 5935)
    {
        channelInfo->op_class = 136;
        channelInfo->channel = (freq - 5925) / 5;
        channelInfo->hardwareMode = WifiMode::IEEE80211A;
        return true;
    }

    /* 56.16 GHz, channel 1..6 */
    if (freq >= 56160 + 2160 * 1 && freq <= 56160 + 2160 * 6)
    {
        if (sec_channel != SecondaryChannelLocation::None)
            return false;

        switch (chanwidth)
        {
        case WifiBandwidth::BW20:
        case WifiBandwidth::BW40:
        case WifiBandwidth::BW2160:
            channelInfo->channel = (freq - 56160) / 2160;
            channelInfo->op_class = 180;
            break;
        case WifiBandwidth::BW4320:
            /* EDMG channels 9 - 13 */
            if (freq > 56160 + 2160 * 5)
                return false;

            channelInfo->channel = (freq - 56160) / 2160 + 8;
            channelInfo->op_class = 181;
            break;
        case WifiBandwidth::BW6480:
            /* EDMG channels 17 - 20 */
            if (freq > 56160 + 2160 * 4)
                return false;

            channelInfo->channel = (freq - 56160) / 2160 + 16;
            channelInfo->op_class = 182;
            break;
        case WifiBandwidth::BW8640:
            /* EDMG channels 25 - 27 */
            if (freq > 56160 + 2160 * 3)
                return false;

            channelInfo->channel = (freq - 56160) / 2160 + 24;
            channelInfo->op_class = 183;
            break;
        default:
            return false;
        }

        channelInfo->hardwareMode = WifiMode::IEEE80211AD;
        return true;
    }

    return false;
}
// * @NL80211_RRF_NO_OFDM: OFDM modulation not allowed
//  * @NL80211_RRF_NO_CCK: CCK modulation not allowed
//  * @NL80211_RRF_NO_INDOOR: indoor operation not allowed
//  * @NL80211_RRF_NO_OUTDOOR: outdoor operation not allowed
//  * @NL80211_RRF_DFS: DFS support is required to be used
//  * @NL80211_RRF_PTP_ONLY: this is only for Point To Point links
//  * @NL80211_RRF_PTMP_ONLY: this is only for Point To Multi Point links
//  * @NL80211_RRF_NO_IR: no mechanisms that initiate radiation are allowed,
