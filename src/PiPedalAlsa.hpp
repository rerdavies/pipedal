// Copyright (c) 2021 Robin Davies
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

#include "json.hpp"

namespace pipedal {
    class AlsaDeviceInfo {
    public:
        int cardId_ = -1;
        std::string id_;
        std::string name_;
        std::string longName_;
        std::vector<uint32_t> sampleRates_;
        uint32_t minBufferSize_ = 0,maxBufferSize_ = 0;
    
        DECLARE_JSON_MAP(AlsaDeviceInfo);

    };

    class PiPedalAlsaDevices {

        std::map<std::string,AlsaDeviceInfo> cachedDevices;

        bool getCachedDevice(const std::string&name, AlsaDeviceInfo*pResult);
        void cacheDevice(const std::string&name, const AlsaDeviceInfo&deviceInfo);
    public:
        
        std::vector<AlsaDeviceInfo> GetAlsaDevices();
    };
}