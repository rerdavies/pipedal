#pragma once 
#include <cmath>

namespace pipedal {

    const double DB0=-96;

    inline float db2a(float db)
    {
        return std::pow(10,db/20);
    }
    inline float a2db(float amplitude)
    {
        if (amplitude == 0) return DB0;
        return 20*std::log10(std::abs(amplitude));
    }

} // namespace