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

#pragma once

#include <stdint.h>
#include <vector>
#include <map>
#include "WifiRegulations.hpp"
#include <filesystem>

namespace pipedal
{



    // struct FrequencyRule
    // {
    //     uint32_t minFrequencyKHz;
    //     uint32_t maxFrequencyKhz;
    //     uint32_t maximumAntennaGain; // in mBiu (100*dBi)
    //     uint32_t maximumEirp;        // in mBm (100*dBm)
    //     RegRuleFlags flags;

    //     bool hasFlag(RegRuleFlags flag) const { return (((int)flag) & (int)this->flags) != 0; }
    // };


    class RegDb
    {
    private:
        friend class CrdbRules;
        uint8_t *pData;
        
    public:
        static RegDb&GetInstance();

        RegDb();
        RegDb(const std::filesystem::path&path); // for testing only.
        ~RegDb();

        const std::vector<WifiRegulations>&Regulations() const { return regulations;}
 
        bool IsValid() const { return isValid; }

        const WifiRegulations&getWifiRegulations(const std::string&countryIso3661) const;

        std::map<std::string,std::string> getRegulatoryDomains(const std::filesystem::path&namesFile="/etc/pipedal/config/iso_codes.json") const;
    private:
        bool isValid = false;

        std::vector<uint8_t> data;
        void *ptr() { return (void *)&(data[0]); }
        void Load19();
        void Load20();

        std::vector<WifiRegulations> regulations;
    };

}