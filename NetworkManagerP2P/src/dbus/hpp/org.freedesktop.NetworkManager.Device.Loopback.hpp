
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_Device_Loopback_hpp__proxy__H__
#define __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_Device_Loopback_hpp__proxy__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace freedesktop {
namespace NetworkManager {
namespace Device {

class Loopback_proxy
{
public:
    static constexpr const char* INTERFACE_NAME = "org.freedesktop.NetworkManager.Device.Loopback";

protected:
    Loopback_proxy(sdbus::IProxy& proxy)
        : proxy_(proxy)
    {
    }

    ~Loopback_proxy() = default;

private:
    sdbus::IProxy& proxy_;
};

}}}} // namespaces

#endif