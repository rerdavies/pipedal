#include "pch.h"
#include "VuUpdate.hpp"

using namespace pipedal;


JSON_MAP_BEGIN(VuUpdate)
    JSON_MAP_REFERENCE(VuUpdate,instanceId)
    JSON_MAP_REFERENCE(VuUpdate,sampleTime)
    JSON_MAP_REFERENCE(VuUpdate,isStereoInput)
    JSON_MAP_REFERENCE(VuUpdate,isStereoOutput)
    JSON_MAP_REFERENCE(VuUpdate,inputMaxValueL)
    JSON_MAP_REFERENCE(VuUpdate,inputMaxValueR)
    JSON_MAP_REFERENCE(VuUpdate,outputMaxValueL)
    JSON_MAP_REFERENCE(VuUpdate,outputMaxValueR)
JSON_MAP_END()

