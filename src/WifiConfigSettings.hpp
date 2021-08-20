#pragma once

#include "json.hpp"

namespace pipedal {

    class WifiConfigSettings {
    public:
        bool valid_ = false;
        bool wifiWarningGiven_ = false;
        bool rebootRequired_ = false;
        bool enable_ = false;
        std::string countryCode_ = "US"; // iso 3661
        std::string hotspotName_ = "pipedal";
        bool hasPassword_ = false;
        std::string password_;
        std::string channel_ = "g6";
    public:
        DECLARE_JSON_MAP(WifiConfigSettings);
    };
}