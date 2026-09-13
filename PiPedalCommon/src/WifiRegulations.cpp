#include "WifiRegulations.hpp"

using namespace pipedal;

const WifiRule*WifiRegulations::GetRule(int32_t frequencyMhz) const
{
    uint32_t frequencyKhz = (uint32_t)frequencyMhz*1000;

    for (const auto&rule: rules) {
        if (frequencyKhz >= rule.start_freq_khz   && frequencyKhz < rule.end_freq_khz)
        {
            return &rule;
        }
    }
    return nullptr;
}
