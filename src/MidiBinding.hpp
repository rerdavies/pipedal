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
#include <cstdint>
#include <string>
#include <vector>
#include <lv2/urid/urid.h>



namespace pipedal {

class MapFeature;

#define GETTER_SETTER_REF(name) \
    const decltype(name##_)& name() const { return name##_;} \
    void name(const decltype(name##_) &value) { name##_ = value; }

#define GETTER_SETTER_VEC(name) \
    decltype(name##_)& name()  { return name##_;}  \
    const decltype(name##_)& name() const  { return name##_;} 

    //void name(decltype(const name##_) &value) { name##_ =  value; } 
    //void name(decltype(const name##_) &&value) { name##_ =  std::move(value); }



#define GETTER_SETTER(name) \
    decltype(name##_) name() const { return name##_;} \
    void name(decltype(name##_) value) { name##_ = value; }



    const int BINDING_TYPE_NONE = 0;
    const int BINDING_TYPE_NOTE = 1;
    const int BINDING_TYPE_CONTROL = 2;

    const int LINEAR_CONTROL_TYPE = 0;
    const int  CIRCULAR_CONTROL_TYPE = 1;

    const int LATCH_CONTROL_TYPE = 0;
    const int MOMENTARY_CONTROL_TYPE = 1;



class MidiBinding {
public: 
private:
    std::string symbol_;
    int channel_ = -1;
    int bindingType_ = BINDING_TYPE_NONE;
    int note_ = 12*4+24;
    int control_ = 1;
    float minValue_ = 0;
    float maxValue_ = 1;
    float rotaryScale_ = 1;
    int linearControlType_ = LINEAR_CONTROL_TYPE;
    int switchControlType_ = LATCH_CONTROL_TYPE;
public:
    static MidiBinding SystemBinding(const std::string&symbol)
    {
        MidiBinding result;
        result.symbol_ = symbol;
        return result;
    }
    bool operator==(const MidiBinding&other) const
    {
        return this->symbol_ == other.symbol_
        && this->channel_ == other.channel_
        && this->bindingType_ == other.bindingType_
        && this->note_ == other.note_ 
        && this->control_ == other.control_
        && this->minValue_ == other.minValue_
        && this->maxValue_ == other.maxValue_
        && this->rotaryScale_ == other.rotaryScale_
        && this->linearControlType_ == other.linearControlType_
        && this->switchControlType_ == other.switchControlType_;

    }
    GETTER_SETTER(channel);
    GETTER_SETTER_REF(symbol);
    GETTER_SETTER(bindingType);
    GETTER_SETTER(note);
    GETTER_SETTER(control);
    GETTER_SETTER(minValue);
    GETTER_SETTER(maxValue);
    GETTER_SETTER(rotaryScale);
    GETTER_SETTER(linearControlType);
    GETTER_SETTER(switchControlType);

    DECLARE_JSON_MAP(MidiBinding);

};


enum class MidiDeviceSelection {
    DeviceAny = 0,
    DeviceNone = 1,
    DeviceList = 2
};

class MidiChannelBinding {
public:
    static constexpr std::int32_t CHANNEL_OMNI = -1;
private:
    int32_t deviceSelection_ = (int32_t)(MidiDeviceSelection::DeviceAny);

    std::vector<std::string> midiDevices_;

    std::vector<LV2_URID> midiDeviceUrids;

    std::int32_t channel_ = CHANNEL_OMNI;
    bool acceptProgramChanges_ = true;
    bool acceptCommonMessages_ = true;
public:
    MidiDeviceSelection deviceSelection() const { return (MidiDeviceSelection)deviceSelection_; }
    void deviceSelection(MidiDeviceSelection value) { deviceSelection_ = (int32_t)value; }
    GETTER_SETTER_VEC(midiDevices);
    GETTER_SETTER(channel);
    GETTER_SETTER(acceptProgramChanges);
    GETTER_SETTER(acceptCommonMessages);

    void Prepare(MapFeature &mapFeature);

    static MidiChannelBinding DefaultForMissingValue();

    bool WantsMidiMessage(uint8_t midi_cc0,uint8_t midi_cc1, uint8_t midi_cc2);
    bool WantProgramChange(uint8_t midi_cc0,uint8_t midi_cc1);

    // must call Prepare first.
    bool WantsDevice(LV2_URID deviceUrid);


    DECLARE_JSON_MAP(MidiChannelBinding);

};
///////////////////////////////////////

inline bool MidiChannelBinding::WantsMidiMessage(uint8_t midi_cc0,uint8_t midi_cc1, uint8_t midi_cc2)
{
    if (midi_cc0 < 0xF0)
    {
        if (channel_ < 0) return true;

        return midi_cc0 & 0x0F == channel_;
    } else {
        return acceptCommonMessages_;
    }
}

inline bool MidiChannelBinding::WantProgramChange(uint8_t midi_cc0,uint8_t midi_cc1)
{
    if (!acceptProgramChanges_)
    {
        return false;
    }
    if (channel_ < 0) return true;
    return midi_cc0 & 0x0F == channel_;
}

inline bool MidiChannelBinding::WantsDevice(LV2_URID deviceUrid)
{
    if (this->midiDeviceUrids.size() == 0) return true;
    for (LV2_URID urid: midiDeviceUrids)
    {
        if (urid == deviceUrid) return true;
    }
    return false;
}

//////////////////////////////////////////


#undef GETTER_SETTER_REF
#undef GETTER_SETTER_VEC
#undef GETTER_SETTER



} // namespace