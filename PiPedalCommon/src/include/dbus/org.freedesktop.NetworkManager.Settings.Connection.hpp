
/*
 * This file was automatically generated by sdbus-c++-xml2cpp; DO NOT EDIT!
 */

#ifndef __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_Settings_Connection_hpp__proxy__H__
#define __sdbuscpp__dbus_hpp_org_freedesktop_NetworkManager_Settings_Connection_hpp__proxy__H__

#include <sdbus-c++/sdbus-c++.h>
#include <string>
#include <tuple>

namespace org {
namespace freedesktop {
namespace NetworkManager {
namespace Settings {

class Connection_proxy
{
public:
    static constexpr const char* INTERFACE_NAME = "org.freedesktop.NetworkManager.Settings.Connection";

protected:
    Connection_proxy(sdbus::IProxy& proxy)
        : proxy_(proxy)
    {
        proxy_.uponSignal("Updated").onInterface(INTERFACE_NAME).call([this](){ this->onUpdated(); });
        proxy_.uponSignal("Removed").onInterface(INTERFACE_NAME).call([this](){ this->onRemoved(); });
    }

    ~Connection_proxy() = default;

    virtual void onUpdated() = 0;
    virtual void onRemoved() = 0;

public:
    void Update(const std::map<std::string, std::map<std::string, sdbus::Variant>>& properties)
    {
        proxy_.callMethod("Update").onInterface(INTERFACE_NAME).withArguments(properties);
    }

    void UpdateUnsaved(const std::map<std::string, std::map<std::string, sdbus::Variant>>& properties)
    {
        proxy_.callMethod("UpdateUnsaved").onInterface(INTERFACE_NAME).withArguments(properties);
    }

    void Delete()
    {
        proxy_.callMethod("Delete").onInterface(INTERFACE_NAME);
    }

    std::map<std::string, std::map<std::string, sdbus::Variant>> GetSettings()
    {
        std::map<std::string, std::map<std::string, sdbus::Variant>> result;
        proxy_.callMethod("GetSettings").onInterface(INTERFACE_NAME).storeResultsTo(result);
        return result;
    }

    std::map<std::string, std::map<std::string, sdbus::Variant>> GetSecrets(const std::string& setting_name)
    {
        std::map<std::string, std::map<std::string, sdbus::Variant>> result;
        proxy_.callMethod("GetSecrets").onInterface(INTERFACE_NAME).withArguments(setting_name).storeResultsTo(result);
        return result;
    }

    void ClearSecrets()
    {
        proxy_.callMethod("ClearSecrets").onInterface(INTERFACE_NAME);
    }

    void Save()
    {
        proxy_.callMethod("Save").onInterface(INTERFACE_NAME);
    }

    std::map<std::string, sdbus::Variant> Update2(const std::map<std::string, std::map<std::string, sdbus::Variant>>& settings, const uint32_t& flags, const std::map<std::string, sdbus::Variant>& args)
    {
        std::map<std::string, sdbus::Variant> result;
        proxy_.callMethod("Update2").onInterface(INTERFACE_NAME).withArguments(settings, flags, args).storeResultsTo(result);
        return result;
    }

public:
    bool Unsaved()
    {
        return proxy_.getProperty("Unsaved").onInterface(INTERFACE_NAME);
    }

    uint32_t Flags()
    {
        return proxy_.getProperty("Flags").onInterface(INTERFACE_NAME);
    }

    std::string Filename()
    {
        return proxy_.getProperty("Filename").onInterface(INTERFACE_NAME);
    }

private:
    sdbus::IProxy& proxy_;
};

}}}} // namespaces

#endif
