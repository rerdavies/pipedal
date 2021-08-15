#include "pch.h"
#include "Banks.hpp"

using namespace pipedal;

JSON_MAP_BEGIN(PresetIndexEntry)
    JSON_MAP_REFERENCE(PresetIndexEntry,instanceId)
    JSON_MAP_REFERENCE(PresetIndexEntry,name)
JSON_MAP_END()



JSON_MAP_BEGIN(PresetIndex)
    JSON_MAP_REFERENCE(PresetIndex,selectedInstanceId)
    JSON_MAP_REFERENCE(PresetIndex,presetChanged)
    JSON_MAP_REFERENCE(PresetIndex,presets)
JSON_MAP_END()


JSON_MAP_BEGIN(BankIndex)
    JSON_MAP_REFERENCE(BankIndex,selectedBank)
    JSON_MAP_REFERENCE(BankIndex,nextInstanceId)
    JSON_MAP_REFERENCE(BankIndex,entries)
JSON_MAP_END()

JSON_MAP_BEGIN(BankIndexEntry)
    JSON_MAP_REFERENCE(BankIndexEntry,instanceId)
    JSON_MAP_REFERENCE(BankIndexEntry,name)
JSON_MAP_END()

JSON_MAP_BEGIN(BankFile)
    JSON_MAP_REFERENCE(BankFile,name)
    JSON_MAP_REFERENCE(BankFile,nextInstanceId)
    JSON_MAP_REFERENCE(BankFile,selectedPreset)
    JSON_MAP_REFERENCE(BankFile,presets)
JSON_MAP_END()

JSON_MAP_BEGIN(BankFileEntry)
    JSON_MAP_REFERENCE(BankFileEntry,instanceId)
    JSON_MAP_REFERENCE(BankFileEntry,preset)
JSON_MAP_END()


