#include "PluginPreset.hpp"

using namespace pipedal;

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
    JSON_MAP_REFERENCE(PluginPreset,state)
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

