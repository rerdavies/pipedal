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
        bool supportsCapture_ = false;
        bool supportsPlayback_ = false;
        bool captureBusy_ = false;
        bool playbackBusy_ = false;

        bool isDummyDevice() const {
            return id_.starts_with("dummy:");
        }

        DECLARE_JSON_MAP(AlsaDeviceInfo);

    };
    class AlsaMidiDeviceInfo {
    public:
        enum Direction {
            None = 0,
            In = 1,
            Out = 2,
            InOut = 3
        };
        AlsaMidiDeviceInfo() { }
        AlsaMidiDeviceInfo(const char*name, const char*description);
        AlsaMidiDeviceInfo(const char*name, const char*description, int card, int device, int subdevice);
        std::string name_;
        std::string description_;

        // non-serialized.
        int card_ = -1;
        int device_ = -1;
        int subdevice_ = -1;
        bool isVirtual_ = false;
        int subDevices_ = -1;
        Direction direction_ = Direction::None;

        DECLARE_JSON_MAP(AlsaMidiDeviceInfo);

    };

    class PiPedalAlsaDevices {

        std::map<std::string,AlsaDeviceInfo> cachedDevices;

        bool getCachedDevice(const std::string&name, AlsaDeviceInfo*pResult);
        void cacheDevice(const std::string&name, const AlsaDeviceInfo&deviceInfo);
    public:
        
        std::vector<AlsaDeviceInfo> GetAlsaDevices();
    };
    // we use ALSA sequencers now instead of ALSA rawmidi devices.
    // Used by test suite to verify migration behaviour.
    std::vector<AlsaMidiDeviceInfo> LegacyGetAlsaMidiInputDevices();
    std::vector<AlsaMidiDeviceInfo> LegacyGetAlsaMidiOutputDevices();

}