#pragma once

#include <string>
#include "JackServerSettings.hpp"

namespace pipedal {

class ShutdownClient {
    static bool WriteMessage(const char*message);
public:
    static bool CanUseShutdownClient();
    static bool RequestShutdown(bool restart);
    static bool SetJackServerConfiguration(const JackServerSettings & jackServerSettings);
};

} // namespace