#pragma once

#include <string>
#include "JackServerSettings.hpp"
#include "WifiConfigSettings.hpp"

namespace pipedal {

class ShutdownClient {
    static bool WriteMessage(const char*message);
public:
    static bool CanUseShutdownClient();
    static bool RequestShutdown(bool restart);
    static bool SetJackServerConfiguration(const JackServerSettings & jackServerSettings);
    static bool IsOnLocalSubnet(const std::string&fromAddress);
    static void SetWifiConfig(const WifiConfigSettings & settings);
};

} // namespace