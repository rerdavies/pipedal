/*
 * MIT License
 * 
 * Copyright (c) 2022 Robin E. R. Davies
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#pragma once

#include "PiPedalHost.hpp"
#include <functional>
#include "json.hpp"


namespace pipedal {
    struct Vst3PluginInfo {
    public:

        std::string filePath_;
        std::string version_;
        std::string uid_;

        Lv2PluginUiInfo pluginInfo_;

        DECLARE_JSON_MAP(Vst3PluginInfo);

    };

    struct  Vst3ProgramListEntry {
        int32_t id;
        std::string name;
    };

    struct Vst3ProgramList {
        std::string name;

        std::vector<Vst3ProgramListEntry> programs;
        
    };

    class Vst3Effect: public IEffect {
    public:
        using Ptr = std::unique_ptr<Vst3Effect>;

        virtual void Prepare(int32_t sampleRate, size_t maxBufferSize,int inputChannels, int outputChannels) = 0;

        virtual void Activate() = 0;
        virtual void Deactivate() = 0;
        virtual void Unprepare() = 0;
        virtual void CheckSync() = 0; // throw if audioProcessor and controller are not sync (test only)

        virtual bool SupportsState() const = 0;
        virtual bool GetState(std::vector<uint8_t> *state) = 0;

		virtual void SetState(const std::vector<uint8_t> state) = 0;

        using ControlChangedHandler = std::function<void(int control,float value)>;
        virtual void SetControlChangedHandler(const ControlChangedHandler& handler) = 0;
        virtual void RemoveControlChangedHandler() = 0;

        virtual std::vector<Vst3ProgramList> GetProgramList(int32_t programListId) = 0;
    public:
        static std::unique_ptr<Vst3Effect> CreateInstance(uint64_t instanceId,const Vst3PluginInfo& info, IHost *pHost);
    };

}