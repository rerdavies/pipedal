#pragma once
#include "json.hpp"

namespace  pipedal {
    
class PiPedalVersion {
private:
    std::string server_;
    std::string serverVersion_;
    std::string operatingSystem_;
    std::string osVersion_;
    bool debug_;

public:
    PiPedalVersion();
    ~PiPedalVersion() = default;


    DECLARE_JSON_MAP(PiPedalVersion);
};

} // namepace pipedal