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
