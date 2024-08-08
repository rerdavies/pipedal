#include "WifiRegulations.hpp"

using namespace pipedal;

const WifiRule*WifiRegulations::GetRule(int32_t frequencyMhz) const
{
    int32_t frequencyKhz = frequencyMhz*1000;

    for (const auto&rule: rules) {
        if (frequencyKhz >= rule.start_freq_khz   && frequencyKhz < rule.end_freq_khz)
        {
            return &rule;
        }
    }
    return nullptr;
}
