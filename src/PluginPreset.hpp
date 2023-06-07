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
#include "PiPedalException.hpp"
#include <utility>
#include <map>
#include <memory>
#include "StateInterface.hpp"
#include "Pedalboard.hpp"

namespace pipedal {

class PluginPresetIndexEntry {
public:
    PluginPresetIndexEntry() { }
    PluginPresetIndexEntry(const std::string &pluginUri, const std::string&fileName)
    :   pluginUri_(pluginUri),
        fileName_(fileName)
    {
        
    }
    std::string pluginUri_;
    std::string fileName_;


    DECLARE_JSON_MAP(PluginPresetIndexEntry);

};

class PluginUiPreset {
public:
    std::string label_;
    uint64_t instanceId_;
    DECLARE_JSON_MAP(PluginUiPreset);
};

class PluginUiPresets {
public:
    std::string pluginUri_;
    std::vector<PluginUiPreset> presets_;
    DECLARE_JSON_MAP(PluginUiPresets);
};


class PluginPresetIndex {
public:
    std::vector<PluginPresetIndexEntry> entries_;
    uint64_t nextInstanceId_ = 1;
    DECLARE_JSON_MAP(PluginPresetIndex);

};


class PluginPreset {
public:
    PluginPreset() { }
    PluginPreset(uint64_t instanceId, const std::string&label,const PedalboardItem&pedalboardItem)
    : instanceId_(instanceId)
    , label_(label)
    , state_(pedalboardItem.lv2State())
    { 
        for (auto & controlValue : pedalboardItem.controlValues())
        {
            this->controlValues_[controlValue.key()] = controlValue.value();
        }

    }
    PluginPreset(
        uint64_t instanceId, 
        const std::string&label,
        const std::map<std::string,
        float> & controlValues,
        const Lv2PluginState &state)
    :   instanceId_(instanceId),
        label_(label),
        controlValues_((controlValues)),
        state_(state)
    {
    }

    static PluginPreset MakeLilvPreset(
        uint64_t instanceId,
        const std::string&label,
        const std::map<std::string,float> & controlValues,
        const char*presetUri
    ) {
        PluginPreset preset;
        preset.instanceId_ = instanceId;
        preset.label_ = label;
        preset.lilvPresetUri_ = presetUri;
        preset.controlValues_ = controlValues;
        return preset;
    }   

    uint64_t instanceId_;
    std::string label_;
    std::string lilvPresetUri_;
    std::map<std::string,float> controlValues_;
    Lv2PluginState state_;
    DECLARE_JSON_MAP(PluginPreset);
};

class PluginPresets {
public:
    std::string pluginUri_;
    std::vector<PluginPreset> presets_;
    uint64_t nextInstanceId_ = 1;

    const PluginPreset&GetPreset(uint64_t presetId) {
        for (const auto&preset: presets_)
        {
            if (preset.instanceId_ == presetId)
            {
                return preset;
            }
        }
        throw PiPedalException("Preset id not found.");
    }
    int Find(const std::string &label)
    {
        for (size_t i = 0; i < presets_.size(); ++i)
        {
            if (presets_[i].label_ == label)
            {
                return i;
            }
        }
        return -1;
    }
    int Find(uint64_t instanceId)
    {
        for (size_t i = 0; i < presets_.size(); ++i)
        {
            if (presets_[i].instanceId_ == instanceId)
            {
                return i;
            }
        }
        return -1;
    }

    DECLARE_JSON_MAP(PluginPresets);
};



} // namespace