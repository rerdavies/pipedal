#pragma once
#include <map>
#include <string>
#include <ostream>

namespace sdbus {
    class Variant;
}

extern std::ostream&operator<<(std::ostream&s, const sdbus::Variant&v);
extern std::ostream&operator<<(std::ostream&s,const std::map<std::string, sdbus::Variant>& properties);
