#include "pch.h"
#include "Presets.hpp"

using namespace pipedal;

JSON_MAP_BEGIN(PedalPreset)
    JSON_MAP_REFERENCE(PedalPreset,instanceId)
    JSON_MAP_REFERENCE(PedalPreset,values)
JSON_MAP_END()

JSON_MAP_BEGIN(PedalBoardPreset)
    JSON_MAP_REFERENCE(PedalBoardPreset,instanceId)
    JSON_MAP_REFERENCE(PedalBoardPreset,displayName)
    JSON_MAP_REFERENCE(PedalBoardPreset,values)
JSON_MAP_END()


JSON_MAP_BEGIN(PedalBoardPresets)
    JSON_MAP_REFERENCE(PedalBoardPresets,nextInstanceId)
    JSON_MAP_REFERENCE(PedalBoardPresets,currentPreset)
    JSON_MAP_REFERENCE(PedalBoardPresets,presets)
JSON_MAP_END()



