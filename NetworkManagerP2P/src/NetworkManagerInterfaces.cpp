#include "NetworkManagerInterfaces.hpp"
#include "ss.hpp"

using namespace impl;

namespace impl
{
    std::string NetworkManagerStateToString(uint32_t state)
    {
        // STUB: Not clear which state enum we're getting.
        std::stringstream s;
        s << state;
        return s.str();
    }

    std::string NetworkManagerDeviceStateToString(uint32_t state)
    {
#define DEVICE_CASE(name, value) \
    case value:                  \
        return #name;

        switch (state)
        {
            DEVICE_CASE(NM_DEVICE_STATE_UNKNOWN, 0)
            DEVICE_CASE(NM_DEVICE_STATE_UNMANAGED, 10)
            DEVICE_CASE(NM_DEVICE_STATE_UNAVAILABLE, 20)
            DEVICE_CASE(NM_DEVICE_STATE_DISCONNECTED, 30)
            DEVICE_CASE(NM_DEVICE_STATE_PREPARE, 40)
            DEVICE_CASE(NM_DEVICE_STATE_CONFIG, 50)
            DEVICE_CASE(NM_DEVICE_STATE_NEED_AUTH, 60)
            DEVICE_CASE(NM_DEVICE_STATE_IP_CONFIG, 70)
            DEVICE_CASE(NM_DEVICE_STATE_IP_CHECK, 80)
            DEVICE_CASE(NM_DEVICE_STATE_SECONDARIES, 90)
            DEVICE_CASE(NM_DEVICE_STATE_ACTIVATED, 100)
            DEVICE_CASE(NM_DEVICE_STATE_DEACTIVATING, 110)
            DEVICE_CASE(NM_DEVICE_STATE_FAILED, 120)
        default:
        {
            std::stringstream s;
            s << state;
            return s.str();
        }
        }
    }
}
