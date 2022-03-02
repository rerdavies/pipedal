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

#include "pch.h"
#include "Units.hpp"
#include "PiPedalException.hpp"
#include <algorithm>

#include  "lv2/lv2plug.in/ns/extensions/units/units.h"


using namespace pipedal;

Units UriToUnits(const std::string &uri);


#define CASE(name) \
   { Units::name, #name },


std::map<Units,std::string> unitsToStringMap = 
{
        CASE(none)
        CASE(unknown)

        CASE(bar)
        CASE(beat)
        CASE(bpm)
        CASE(cent)
        CASE(cm)
        CASE(db)
        CASE(hz)
        CASE(khz)
        CASE(km)
        CASE(m)
        CASE(mhz)
        CASE(midiNote)
        CASE(min)
        CASE(ms)
        CASE(pc)
        CASE(s)
        CASE(semitone12TET)

};

const std::string& pipedal::UnitsToString(Units units)
{
    return unitsToStringMap[units];
}
#undef CASE

#define CASE(name) \
     { #name, Units::name},

std::map<std::string,Units> unitMap = {
        CASE(none)
        CASE(unknown)
        CASE(bar)
        CASE(beat)
        CASE(bpm)
        CASE(cent)
        CASE(cm)
        CASE(db)
        CASE(hz)
        CASE(khz)
        CASE(km)
        CASE(m)
        CASE(mhz)
        CASE(midiNote)
        CASE(min)
        CASE(ms)
        CASE(pc)
        CASE(s)
        CASE(semitone12TET)
};
#undef CASE

#define CASE(name) \
     { LV2_UNITS__##name, Units::name},



std::map<std::string,Units> uriToUnitsMap = {
        CASE(bar)
        CASE(beat)
        CASE(bpm)
        CASE(cent)
        CASE(cm)
        CASE(db)
        CASE(hz)
        CASE(khz)
        CASE(km)
        CASE(m)
        CASE(mhz)
        CASE(midiNote)
        CASE(min)
        CASE(ms)
        CASE(pc)
        CASE(s)
        CASE(semitone12TET)
};


Units pipedal::StringToUnits(const std::string &text)
{
    return unitMap[text];
}

Units pipedal::UriToUnits(const std::string &text)
{
    if (text.length() == 0) return Units::none;
    auto result = uriToUnitsMap[text];
    if (result == Units::none) return Units::unknown;
    return result;

}



class UnitsEnumConverter: public json_enum_converter<Units> {
public:
    virtual Units fromString(const std::string&value) const
    {
        return StringToUnits(value);
    }
    virtual const std::string& toString(Units value) const
    {
        return UnitsToString(value);
    }
    

} g_units_converter;


json_enum_converter<Units> *pipedal::get_units_enum_converter()
{
    return &g_units_converter;
}
