// Copyright (c) 2023 Robin Davies
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

#include <lilv/lilv.h>

namespace pipedal {
    class MapFeature;
    class PedalboardItem;
    class Lv2PluginInfo;
    class IEffect;
    class HostWorkerThread;

    class IHost
    {
    public:
        virtual LilvWorld *getWorld() = 0;
        virtual LV2_URID_Map *GetLv2UridMap() = 0;
        virtual MapFeature&GetMapFeature() = 0;
        virtual LV2_URID GetLv2Urid(const char *uri) = 0;
        virtual std::string Lv2UridToString(LV2_URID urid) = 0;

        virtual LV2_Feature *const *GetLv2Features() const = 0;
        virtual double GetSampleRate() const = 0;
        virtual void SetMaxAudioBufferSize(size_t size) = 0;
        virtual size_t GetMaxAudioBufferSize() const = 0;
        virtual size_t GetAtomBufferSize() const = 0;

        virtual bool HasMidiInputChannel() const = 0;
        virtual int GetNumberOfInputAudioChannels() const = 0;
        virtual int GetNumberOfOutputAudioChannels() const = 0;
        virtual std::shared_ptr<Lv2PluginInfo> GetPluginInfo(const std::string &uri) const = 0;
        virtual std::shared_ptr<HostWorkerThread> GetHostWorkerThread() = 0;

        virtual IEffect *CreateEffect(PedalboardItem &pedalboard) = 0;

    };
}