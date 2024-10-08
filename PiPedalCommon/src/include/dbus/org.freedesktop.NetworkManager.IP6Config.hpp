
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_IP6Config_hpp__proxy__H__
#define __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_IP6Config_hpp__proxy__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace freedesktop {
namespace NetworkManager {

class IP6Config_proxy
{
public:
    static constexpr const char* INTERFACE_NAME = "org.freedesktop.NetworkManager.IP6Config";

protected:
    IP6Config_proxy(sdbus::IProxy& proxy)
        : proxy_(proxy)
    {
    }

    ~IP6Config_proxy() = default;

public:
    std::vector<sdbus::Struct<std::vector<uint8_t>, uint32_t, std::vector<uint8_t>>> Addresses()
    {
        return proxy_.getProperty("Addresses").onInterface(INTERFACE_NAME);
    }

    std::vector<std::map<std::string, sdbus::Variant>> AddressData()
    {
        return proxy_.getProperty("AddressData").onInterface(INTERFACE_NAME);
    }

    std::string Gateway()
    {
        return proxy_.getProperty("Gateway").onInterface(INTERFACE_NAME);
    }

    std::vector<sdbus::Struct<std::vector<uint8_t>, uint32_t, std::vector<uint8_t>, uint32_t>> Routes()
    {
        return proxy_.getProperty("Routes").onInterface(INTERFACE_NAME);
    }

    std::vector<std::map<std::string, sdbus::Variant>> RouteData()
    {
        return proxy_.getProperty("RouteData").onInterface(INTERFACE_NAME);
    }

    std::vector<std::vector<uint8_t>> Nameservers()
    {
        return proxy_.getProperty("Nameservers").onInterface(INTERFACE_NAME);
    }

    std::vector<std::string> Domains()
    {
        return proxy_.getProperty("Domains").onInterface(INTERFACE_NAME);
    }

    std::vector<std::string> Searches()
    {
        return proxy_.getProperty("Searches").onInterface(INTERFACE_NAME);
    }

    std::vector<std::string> DnsOptions()
    {
        return proxy_.getProperty("DnsOptions").onInterface(INTERFACE_NAME);
    }

    int32_t DnsPriority()
    {
        return proxy_.getProperty("DnsPriority").onInterface(INTERFACE_NAME);
    }

private:
    sdbus::IProxy& proxy_;
};

}}} // namespaces

#endif
