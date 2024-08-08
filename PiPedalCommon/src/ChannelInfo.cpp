#include "ChannelInfo.hpp"
#include "WifiRegulations.hpp"
#include "RegDb.hpp"
#include <set>
#include <algorithm>
#include "ss.hpp"

using namespace pipedal;

// To do: no support for ac 80GHz channels (which pi supports).
// Wifi 6 ax channels are explicitly disabled (pi doesn't support them).


struct OpClassChannels
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
static std::vector<OpClassChannels> opClasses = {
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

        // WiFi 5.
        // {WifiMode::IEEE80211A, 128, 36, 177, 4, WifiBandwidth::BW80, true},
        // {WifiMode::IEEE80211A, 129, 36, 177, 4, WifiBandwidth::BW160, true},
        // {WifiMode::IEEE80211A, 130, 36, 177, 4, WifiBandwidth::BW80P80, true},

        // WiFi 6.
    // {WifiMode::IEEE80211AX, 131, 1, 233, 4, WifiBandwidth::BW20, true},
    // {WifiMode::IEEE80211AX, 132, 1, 233, 8, WifiBandwidth::BW40PLUS, true},
    // {WifiMode::IEEE80211AX, 133, 1, 233, 16, WifiBandwidth::BW80, true},
    // {WifiMode::IEEE80211AX, 134, 1, 233, 32, WifiBandwidth::BW160, true},
    // {WifiMode::IEEE80211AX, 135, 1, 233, 16, WifiBandwidth::BW80P80, false},
    // {WifiMode::IEEE80211AX, 136, 2, 2, 4, WifiBandwidth::BW20, false},

    
     /* IEEE Std 802.11ad-2012 and P802.ay/D5.0 60 GHz operating classes.
     * Class 180 has the legacy channels 1-6. Classes 181-183 include
     * channels which implement channel bonding features.
     */
    // {WifiMode::IEEE80211AD, 180, 1, 6, 1, WifiBandwidth::BW2160, true},
    // {WifiMode::IEEE80211AD, 181, 9, 13, 1, WifiBandwidth::BW4320, true},
    // {WifiMode::IEEE80211AD, 182, 17, 20, 1, WifiBandwidth::BW6480, true},
    // {WifiMode::IEEE80211AD, 183, 25, 27, 1, WifiBandwidth::BW8640, true},

};

static int32_t ChannelToFrequency(int32_t channel)
{
    if (channel >= 1 && channel <= 13)
    {
        return 2412 + (channel - 1) * 5;
    }
    if (channel == 14)
    {
        return 2484;
    }
    if (channel >= 36 && channel <= 165)
    {
        return 5180 + (channel - 36) * 5;
    }
    return -1;
}
static bool BandwidthPermitted(int32_t frequency,WifiBandwidth bandwidth, const WifiRule &rule)
{
    int32_t channelWidth = -1;
    switch (bandwidth)
    {
    case WifiBandwidth::BW20:
        channelWidth = 20;
        break;
    case WifiBandwidth::BW40PLUS:
        channelWidth = 40;
        if (rule.HasFlag(RegRuleFlags::NO_HT40PLUS))
            return false;
        break;
    case WifiBandwidth::BW40MINUS:
        if (rule.HasFlag(RegRuleFlags::NO_HT40MINUS))
            return false;
        channelWidth = 40;
        break;
    case WifiBandwidth::BW40: // wifi 6 only. do NOT use for a/g channels.
        channelWidth = 40;
        break;
    case WifiBandwidth::BW80:
        if (rule.HasFlag(RegRuleFlags::NO_80MHZ))
            return false;
        channelWidth = 80;
        break;
    case WifiBandwidth::BW160:
        if (rule.HasFlag(RegRuleFlags::NO_160MHZ))
            return false;
        channelWidth = 160;
        break;
    case WifiBandwidth::BW80P80:
        if (rule.HasFlag(RegRuleFlags::NO_160MHZ))
            return false;
        channelWidth = 160;
        break;
    case WifiBandwidth::BW2160:
        channelWidth = 2160;
        break;
    case WifiBandwidth::BW4320:
        channelWidth = 4320;
        break;
    case WifiBandwidth::BW6480:
        channelWidth = 6480;
        break;
    case WifiBandwidth::BW8640:
        channelWidth = 8640;
        break;

    default:
        return false;
    }
    if (channelWidth * 1000 > rule.max_bandwidth_khz)
    {
        return false;
    }
    int32_t minFrequency = frequency-channelWidth/2;
    int32_t maxFrequency = minFrequency+channelWidth;
    if (bandwidth == WifiBandwidth::BW40PLUS)
    {
        minFrequency = frequency-channelWidth/4;
        maxFrequency = minFrequency+channelWidth;
    } else if (bandwidth == WifiBandwidth::BW40MINUS)
    {
        maxFrequency = frequency+channelWidth/4;
        minFrequency = maxFrequency-channelWidth;
    }
    if (minFrequency*1000 < rule.start_freq_khz || maxFrequency*1000 > rule.end_freq_khz)
    {
        return false;
    }
    return true;
}

WifiInfo pipedal::getWifiInfo(const std::string&countryIso3661)
{
    const RegDb &regDb = RegDb::GetInstance();
    const WifiRegulations &regulations = regDb.getWifiRegulations(countryIso3661);

    WifiInfo info;
    info.reg_alpha2 = regulations.reg_alpha2;
    info.dfsRegion = regulations.dfs_region;


    for (const auto &classMap : opClasses)
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
            if (!classMap.p2p_support)
                continue;

            int32_t frequency = ChannelToFrequency(channel);
            if (frequency == -1)
                continue;

            const auto *rule = regulations.GetRule(frequency);
            if (!rule)
                continue;

            if (rule->HasFlag(
                    RegRuleFlags::NO_IR | RegRuleFlags::DFS | RegRuleFlags::PTP_ONLY | RegRuleFlags::PTMP_ONLY))
            {
                continue;
            }
            if (rule->HasFlag(RegRuleFlags::DFS))
            {
                continue;
            }

            WifiBandwidth bandwidth = classMap.bandwidth;
            if (!BandwidthPermitted(frequency,bandwidth, *rule))
            {
                continue;
            }

            WifiChannelInfo channelInfo;
            channelInfo.channelNumber = channel;
            channelInfo.hardwareMode = classMap.band;
            channelInfo.disabled = false;
            channelInfo.indoorOnly = rule->HasFlag(RegRuleFlags::NO_OUTDOOR);
            channelInfo.outdoorsOnly = rule->HasFlag(RegRuleFlags::NO_INDOOR);
            channelInfo.no10MHz = rule->max_bandwidth_khz < 10 * 1000;
            channelInfo.no20MHz = rule->max_bandwidth_khz < 20 * 1000;
            channelInfo.no80MHz = rule->max_bandwidth_khz < 80 * 1000;
            channelInfo.noHt40Minus = rule->max_bandwidth_khz < 40 * 1000 || rule->HasFlag(RegRuleFlags::NO_HT40MINUS);
            channelInfo.noHt40Plus = rule->max_bandwidth_khz < 40 * 1000 || rule->HasFlag(RegRuleFlags::NO_HT40PLUS);

            channelInfo.mhz = frequency;
            channelInfo.no10MHz = rule->HasFlag(RegRuleFlags::NO_IR);
            channelInfo.regDomain = classMap.regDomain;
            channelInfo.maxAntennaGain = rule->max_antenna_gain;
            channelInfo.maxEirp = rule->max_eirp;
            channelInfo.bandwidth = bandwidth;

            info.channels.push_back(std::move(channelInfo));
        }
    }
    return info;
}


std::vector<int32_t> pipedal::getValidChannels(const std::string&countryIso3661,int32_t maxChannelWidthMhz)
{
    std::set<int32_t> channels;


    WifiBandwidth maxBandwidth;
    if (maxChannelWidthMhz <= 20)
    {
        maxBandwidth = WifiBandwidth::BW20;
    } else if (maxChannelWidthMhz <= 40)
    {
        maxBandwidth = WifiBandwidth::BW40;
    } else if (maxChannelWidthMhz <= 80)
    {
        maxBandwidth = WifiBandwidth::BW80;
    } else // if (maxChannelWidthMhz <= 160)
    {
        maxBandwidth = WifiBandwidth::BW160;
    } 
    RegDb &regDb = RegDb::GetInstance();
    auto info = pipedal::getWifiInfo(countryIso3661);

    for (const auto&channelInfo: info.channels)
    {
        channels.insert(channelInfo.channelNumber);
    }


    std::vector<int32_t> result;
    result.reserve(channels.size());
    for (auto v : channels)
    {
        result.push_back(v);
    }
    std::sort(result.begin(),result.end());
    return result;
}
int32_t pipedal::getWifiRegClass(const std::string &countryIso3661, int32_t channel, int32_t maxChannelWidthMhz)
{
    WifiBandwidth maxBandwidth;
    if (maxChannelWidthMhz <= 20)
    {
        maxBandwidth = WifiBandwidth::BW20;
    } else if (maxChannelWidthMhz <= 40)
    {
        maxBandwidth = WifiBandwidth::BW40;
    } else if (maxChannelWidthMhz <= 80)
    {
        maxBandwidth = WifiBandwidth::BW80;
    } else // if (maxChannelWidthMhz <= 160)
    {
        maxBandwidth = WifiBandwidth::BW160;
    } 
    RegDb &regDb = RegDb::GetInstance();
    auto info = pipedal::getWifiInfo(countryIso3661);

    const WifiChannelInfo *bestChannel = nullptr;
    for (const auto&channelInfo: info.channels)
    {
        if (channelInfo.channelNumber == channel && channelInfo.bandwidth <= maxBandwidth)
        {
            if (bestChannel == nullptr || bestChannel->bandwidth < channelInfo.bandwidth)
            {
                bestChannel = &channelInfo;
            }
            
        }        
    }
    if (bestChannel)
    {
        if (channel >= 1 && channel <= 13) { // alway 20Ghz.
            return 81; 
        }
        return bestChannel->regDomain;
    }
    std::vector<int32_t> valid_channels;
    try {
        valid_channels = getValidChannels(countryIso3661,40);
    } 
    catch (const std::exception& /**/)
    {
        std::stringstream ss;
        ss << "Channel " << channel << " is not permitted in the country " << countryIso3661 << ".";
        throw std::runtime_error(ss.str());

    }

    {
        std::stringstream ss;
        ss << "Channel " << channel << " is not permitted in the country " << countryIso3661 << ". Permitted channels:\n   ";
        bool first = true;
        for (auto channel: valid_channels)
        {
            if (!first) ss << ", ";
            first = false;
            ss << channel;
        }
        ss << ".";
        throw std::runtime_error(ss.str());
    }


    // Hard-coded values lifted from wpa_supplicant sources. I don't really think it respect the regulatory db.

    /* Operating class 81 - 2.4 GHz band channels 1..13 */
    // int32_t regClass = -1;
    // if (channel >= 1 && channel <= 11)
    // {
    //     regClass = 81;
    // }
    // // lower 5 GHZ.
    // if (channel == 36 || channel == 40 || channel == 44 || channel == 48)
    // {
    //     regClass = 115; // 20 Ghz
    // }
    // if (channel == 149 || channel == 153 || channel == 157 || channel == 161)
    // {
    //     regClass = 124; // 20Ghz nomadic.
    // }
    // if (regClass == -1)
    // {
    //     return -1;
    // }
}

