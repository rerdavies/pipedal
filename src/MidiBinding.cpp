#include "pch.h"
#include "MidiBinding.hpp"


using namespace pipedal;

JSON_MAP_BEGIN(MidiBinding)
    JSON_MAP_REFERENCE(MidiBinding,channel)
    JSON_MAP_REFERENCE(MidiBinding,symbol)
    JSON_MAP_REFERENCE(MidiBinding,bindingType)
    JSON_MAP_REFERENCE(MidiBinding,note)
    JSON_MAP_REFERENCE(MidiBinding,control)
    JSON_MAP_REFERENCE(MidiBinding,minValue)
    JSON_MAP_REFERENCE(MidiBinding,maxValue)
    JSON_MAP_REFERENCE(MidiBinding,rotaryScale)
    JSON_MAP_REFERENCE(MidiBinding,linearControlType)
    JSON_MAP_REFERENCE(MidiBinding,switchControlType)
JSON_MAP_END()

