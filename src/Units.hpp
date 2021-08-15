#pragma once

#include <string>
#include "json.hpp"

namespace pipedal {

enum class Units  {
    none,
    unknown,
    bar,
    beat,
    bpm,
    cent,
    cm,
    db,
    hz,
    khz,
    km,
    m,
    mhz,
    midiNote,
    min,
    ms,
    pc,
    s,
    semitone12TET,

};

Units UriToUnits(const std::string &uri);
const std::string& UnitsToString(Units units);
Units StringToUnits(const std::string &text);




json_enum_converter<Units> *get_units_enum_converter();



} // namespace.

