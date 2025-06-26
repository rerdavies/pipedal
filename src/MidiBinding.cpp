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

#include "MidiBinding.hpp"
#include "MapFeature.hpp"


using namespace pipedal;

MidiChannelBinding MidiChannelBinding::DefaultForMissingValue()
{
    MidiChannelBinding result {};

    return result;
}


void MidiChannelBinding::Prepare(MapFeature &mapFeature)
{
    midiDeviceUrids.resize(0);
    midiDeviceUrids.reserve(midiDevices_.size());

    for (const auto&midiDevice: midiDevices_)
    {
        midiDeviceUrids.push_back(mapFeature.GetUrid(midiDevice.c_str()));
    }
}


JSON_MAP_BEGIN(MidiChannelBinding)
    JSON_MAP_REFERENCE(MidiChannelBinding,deviceSelection)
    JSON_MAP_REFERENCE(MidiChannelBinding,midiDevices)
    JSON_MAP_REFERENCE(MidiChannelBinding,channel)
    JSON_MAP_REFERENCE(MidiChannelBinding,acceptProgramChanges)
    JSON_MAP_REFERENCE(MidiChannelBinding,acceptCommonMessages)
JSON_MAP_END()


JSON_MAP_BEGIN(MidiBinding)
    JSON_MAP_REFERENCE(MidiBinding,channel)
    JSON_MAP_REFERENCE(MidiBinding,symbol)
    JSON_MAP_REFERENCE(MidiBinding,bindingType)
    JSON_MAP_REFERENCE(MidiBinding,note)
    JSON_MAP_REFERENCE(MidiBinding,control)
    JSON_MAP_REFERENCE(MidiBinding,minControlValue)
    JSON_MAP_REFERENCE(MidiBinding,maxControlValue)
    JSON_MAP_REFERENCE(MidiBinding,minValue)
    JSON_MAP_REFERENCE(MidiBinding,maxValue)
    JSON_MAP_REFERENCE(MidiBinding,rotaryScale)
    JSON_MAP_REFERENCE(MidiBinding,linearControlType)
    JSON_MAP_REFERENCE(MidiBinding,switchControlType)
JSON_MAP_END()

