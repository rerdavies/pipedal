#include "pch.h"
#include "WifiConfigSettings.hpp"

using namespace pipedal;

JSON_MAP_BEGIN(WifiConfigSettings)
    JSON_MAP_REFERENCE(WifiConfigSettings,valid)
    JSON_MAP_REFERENCE(WifiConfigSettings,wifiWarningGiven)
    JSON_MAP_REFERENCE(WifiConfigSettings,rebootRequired)
    JSON_MAP_REFERENCE(WifiConfigSettings,enable)
    JSON_MAP_REFERENCE(WifiConfigSettings,hotspotName)
    JSON_MAP_REFERENCE(WifiConfigSettings,hasPassword)
    JSON_MAP_REFERENCE(WifiConfigSettings,password)
    JSON_MAP_REFERENCE(WifiConfigSettings,countryCode)
    JSON_MAP_REFERENCE(WifiConfigSettings,channel)
JSON_MAP_END()

