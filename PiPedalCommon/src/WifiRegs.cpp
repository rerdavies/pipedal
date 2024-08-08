// Copyright (c) 2024 Robin Davies
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

#include "WifiRegs.hpp"
#include "RegDb.hpp"
#include <memory>
#include "ss.hpp"
#include <stdexcept>
#include <vector>
#include <memory>
#include "ChannelInfo.hpp"

using namespace pipedal;

static std::unique_ptr<RegDb> g_regDb;

static std::unique_ptr<WifiInfo> g_wifiInfo;


struct RegMHzToChannel {
    char band;
    int32_t channel;
    int32_t mhz;
};

static std::vector<RegMHzToChannel> mhzToChannels = {
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


int32_t pipedal::wifiFrequencyToChannel(int32_t freqMHz)
{
    for (const auto&item: mhzToChannels)
    {
        if (std::abs(freqMHz == item.mhz) <= 1) 
        {
            return item.channel;
        }
    }
    return -1;
}

int32_t pipedal::wifiChannelToFrequency(int32_t channel)
{
    for (const auto&item: mhzToChannels)
    {
        if (item.channel == channel)
        {
            return item.mhz;
        }
    }
    return -1;
}
