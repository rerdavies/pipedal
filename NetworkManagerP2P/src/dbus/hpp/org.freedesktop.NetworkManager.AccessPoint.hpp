
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_AccessPoint_hpp__proxy__H__
#define __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_AccessPoint_hpp__proxy__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace freedesktop {
namespace NetworkManager {

class AccessPoint_proxy
{
public:
    static constexpr const char* INTERFACE_NAME = "org.freedesktop.NetworkManager.AccessPoint";

protected:
    AccessPoint_proxy(sdbus::IProxy& proxy)
        : proxy_(proxy)
    {
    }

    ~AccessPoint_proxy() = default;

public:
    uint32_t Flags()
    {
        return proxy_.getProperty("Flags").onInterface(INTERFACE_NAME);
    }

    uint32_t WpaFlags()
    {
        return proxy_.getProperty("WpaFlags").onInterface(INTERFACE_NAME);
    }

    uint32_t RsnFlags()
    {
        return proxy_.getProperty("RsnFlags").onInterface(INTERFACE_NAME);
    }

    std::vector<uint8_t> Ssid()
    {
        return proxy_.getProperty("Ssid").onInterface(INTERFACE_NAME);
    }

    uint32_t Frequency()
    {
        return proxy_.getProperty("Frequency").onInterface(INTERFACE_NAME);
    }

    std::string HwAddress()
    {
        return proxy_.getProperty("HwAddress").onInterface(INTERFACE_NAME);
    }

    uint32_t Mode()
    {
        return proxy_.getProperty("Mode").onInterface(INTERFACE_NAME);
    }

    uint32_t MaxBitrate()
    {
        return proxy_.getProperty("MaxBitrate").onInterface(INTERFACE_NAME);
    }

    uint8_t Strength()
    {
        return proxy_.getProperty("Strength").onInterface(INTERFACE_NAME);
    }

    int32_t LastSeen()
    {
        return proxy_.getProperty("LastSeen").onInterface(INTERFACE_NAME);
    }

private:
    sdbus::IProxy& proxy_;
};

}}} // namespaces

#endif
