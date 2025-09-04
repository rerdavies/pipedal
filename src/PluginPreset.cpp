#include "PluginPreset.hpp"

using namespace pipedal;


static std::string GetUnversionedLabel(const std::string label)
{
    size_t pos = label.length();
    if (pos != 0 && label.at(pos-1) == ')')
    {
        --pos;
        while (true)
        {
            if (pos == 0) 
            {
                return label; // not versioned. return the original.
            }
            --pos;
            char c = label[pos];
            if (c >= '0' && c <= '9') 
            {
                continue; // eat the number.
            }
            else if (c == '(')
            {
                return label.substr(0,pos);

            } else {
                return label;  // failure. return the unmodified original
            }
        }
    }
    return label;
}

bool PluginPreset::equals(const PluginPreset&other) const
{
    // everything EXCEPT instanceId (since preset may be from another PluginPresets collection)
    return label_ == other.label_ &&
        lilvPresetUri_ == other.lilvPresetUri_ &&
        controlValues_ == other.controlValues_ &&
        state_ == other.state_;
}

void PluginPresets::MergePreset(const PluginPreset&preset)
{
    int existingPresetIndex = Find(preset.label_);
    if (existingPresetIndex != -1)
    {
        if (this->presets_[existingPresetIndex].equals(preset))
        {
            // an exact duplicate. discard the new preset.
            return;
        }
        std::string baseLabel = GetUnversionedLabel(preset.label_);

        for (int i = 1; i < 1000; ++i)
        {
            std::string newLabel = SS(baseLabel << "(" << i << ")");
            if (Find(newLabel) == -1)
            {
                PluginPreset versionedPreset = preset;
                versionedPreset.label_ = newLabel;
                MergePreset(versionedPreset);
                return;
            }
        }
        // A thousand duplicates?! Heck. just add a duplicate label. :-/
    }
    this->presets_.push_back(preset);
    this->presets_.back().instanceId_ = this->nextInstanceId_++;
    
}


JSON_MAP_BEGIN(PluginUiPreset)
    JSON_MAP_REFERENCE(PluginUiPreset,label)
    JSON_MAP_REFERENCE(PluginUiPreset,instanceId)
JSON_MAP_END()

JSON_MAP_BEGIN(PluginUiPresets)
    JSON_MAP_REFERENCE(PluginUiPresets,pluginUri)
    JSON_MAP_REFERENCE(PluginUiPresets,presets)
JSON_MAP_END()

JSON_MAP_BEGIN(PluginPresetIndexEntry)
    JSON_MAP_REFERENCE(PluginPresetIndexEntry,pluginUri)
    JSON_MAP_REFERENCE(PluginPresetIndexEntry,fileName)
JSON_MAP_END()

JSON_MAP_BEGIN(PluginPresetIndex)
    JSON_MAP_REFERENCE(PluginPresetIndex,entries)
    JSON_MAP_REFERENCE(PluginPresetIndex,nextInstanceId)
JSON_MAP_END()


JSON_MAP_BEGIN(PluginPreset)
    JSON_MAP_REFERENCE(PluginPreset,instanceId)
    JSON_MAP_REFERENCE(PluginPreset,label)
    JSON_MAP_REFERENCE(PluginPreset,controlValues)
    JSON_MAP_REFERENCE(PluginPreset,pathProperties)
    JSON_MAP_REFERENCE(PluginPreset,state)
    JSON_MAP_REFERENCE(PluginPreset,lilvPresetUri)
JSON_MAP_END()

JSON_MAP_BEGIN(PluginPresets)
    JSON_MAP_REFERENCE(PluginPresets,pluginUri)
    JSON_MAP_REFERENCE(PluginPresets,presets)
    JSON_MAP_REFERENCE(PluginPresets,nextInstanceId)
JSON_MAP_END()



// JSON_MAP_BEGIN(PluginPreset)
//     JSON_MAP_REFERENCE(PluginPreset,presetUri)
//     JSON_MAP_REFERENCE(PluginPreset,name)
//     JSON_MAP_REFERENCE(PluginPreset,factoryPreset)
//     JSON_MAP_REFERENCE(PluginPreset,modified)
//     JSON_MAP_REFERENCE(PluginPreset,selected)
// JSON_MAP_END()

