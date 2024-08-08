#pragma once
#include <cstdint>
#include <vector>
#include <string>
#include "EnumFlags.hpp"


namespace pipedal
{

    enum class SecondaryChannelLocation
    {
        None,
        Above,
        Below
    };

    enum class WifiMode
    {
        IEEE80211G,
        IEEE80211A,
        IEEE80211B,
        IEEE80211AD,
        IEEE80211AX,
        IEEE80211ANY,
    };

    enum class WifiBandwidth
    {
        BW20,
        BW40MINUS, // g and a.
        BW40PLUS, 
        BW40, // wifi 6 only. Must come after BW40PLUS, BW40MINUS.
        BW80,
        BW160,
        BW80P80,
        BW2160,
        BW4320,
        BW6480,
        BW8640
    };

    enum class DfsRegion
    {
        Unset = 0,
        Fcc = 1,
        Etsi = 2,
        Japan = 3
    };

    // sync with <linux/nl80211.h
    enum class RegRuleFlags: uint32_t
    {
        NONE = 0,
        NO_OFDM = 1 << 0,
        NO_CCK = 1 << 1,
        NO_INDOOR = 1 << 2,
        NO_OUTDOOR = 1 << 3,
        DFS = 1 << 4,
        PTP_ONLY = 1 << 5,
        PTMP_ONLY = 1 << 6,
        NO_IR = 1 << 7,
        __NO_IBSS = 1 << 8,
        AUTO_BW = 1 << 11,
        IR_CONCURRENT = 1 << 12,
        NO_HT40MINUS = 1 << 13,
        NO_HT40PLUS = 1 << 14,
        NO_80MHZ = 1 << 15,
        NO_160MHZ = 1 << 16,
        NO_HE = 1 << 17,
    };

    struct WifiRule
    {
        RegRuleFlags flags = RegRuleFlags::NONE;

        uint32_t start_freq_khz = 0;
        uint32_t end_freq_khz = 0;
        uint32_t max_bandwidth_khz = 0;
        uint32_t max_antenna_gain = 0;
        uint32_t max_eirp = 0;
        uint32_t dfs_cac_ms = 0;
        uint32_t psd = 0;

        bool HasFlag(RegRuleFlags flag) const {
            return ((uint32_t)flag & (uint32_t)this->flags) != 0;
        }
    };

    struct WifiRegulations
    {

        std::string reg_alpha2;
        DfsRegion dfs_region;
        std::vector<WifiRule> rules;
        const WifiRule*GetRule(int32_t frequencyMhz) const;
    };

    struct WifiChannelInfo
    {
        int channelNumber = -1;
        WifiMode hardwareMode;
        int mhz = 0;
        bool disabled = false;
        bool ir = true;
        bool radarDetection = false;
        std::vector<float> bitrates;
        bool indoorOnly = false;
        bool outdoorsOnly = false;
        bool noHt40Minus = false;
        bool noHt40Plus = false;
        bool no10MHz = false;
        bool no20MHz = false;
        bool no80MHz = false;
        bool no160MHz = false;
        int32_t maxAntennaGain = 0;
        int32_t maxEirp = 0;
        WifiBandwidth bandwidth;
        int32_t regDomain = -1;
    };

    struct WifiP2PChannelInfo : public WifiChannelInfo
    {
    };
    
    struct WifiInfo
    {
        std::string reg_alpha2;
        std::vector<std::string> supportedIfTypes;
        DfsRegion dfsRegion;
        std::vector<WifiChannelInfo> channels;

        const WifiChannelInfo *getChannelInfo(int channel) const;
    };

}
ENABLE_ENUM_FLAGS(pipedal::RegRuleFlags)
