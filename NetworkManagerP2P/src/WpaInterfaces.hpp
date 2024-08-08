#pragma once
#include "dbus/hpp/fi.w1.wpa_supplicant1.hpp"
#include "dbus/hpp/f1.w1.wpa_supplicant1.Interfaces.hpp"
#include "dbus/hpp/f1.w1.wpa_supplicant1.Peer.hpp"
#include "dbus/hpp/f1.w1.wpa_supplicant1.Group.hpp"
#include "dbus/hpp/org.freedesktop.DBus.Properties.hpp"
#include "DBusLog.hpp"
#include "DBusEvent.hpp"
#include "DBusVariantHelper.hpp"
#include <memory>
#include <iostream>

class Interface_proxy_withevents : public fi::w1::wpa_supplicant1::Interface_proxy
{
public:
    using base = fi::w1::wpa_supplicant1::Interface_proxy;

    Interface_proxy_withevents(sdbus::IProxy &proxy)
        : base(proxy), proxy(proxy)
    {
    }

    DBusEvent<const sdbus::ObjectPath &, const std::map<std::string, sdbus::Variant> &>
        OnNetworkAdded;

    DBusEvent<const sdbus::ObjectPath &> OnNetworkRemoved;
    DBusEvent<const sdbus::ObjectPath &> OnNetworkSelected;
    DBusEvent<const sdbus::ObjectPath &, const std::map<std::string, sdbus::Variant> &> OnStationAdded;
    DBusEvent<const sdbus::ObjectPath &> OnStationRemoved;
    DBusEvent<const std::string &> OnStaAuthorized;
    DBusEvent<const std::string &> OnStaDeauthorized;

    DBusEvent<const sdbus::ObjectPath &, const std::map<std::string, sdbus::Variant> &> OnInterworkingAPAdded;
    DBusEvent<> OnInterworkingSelectDone;

protected:
    void EventTrace(const char *methodName, const std::string &message);
private:
    sdbus::IProxy &proxy;

protected:
    virtual void onScanDone(const bool &success);
    virtual void onBSSAdded(const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties);
    virtual void onBSSRemoved(const sdbus::ObjectPath &path);
    virtual void onBlobAdded(const std::string &name);
    virtual void onBlobRemoved(const std::string &name);
    virtual void onNetworkAdded(const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties);
    virtual void onNetworkRemoved(const sdbus::ObjectPath &path);
    virtual void onNetworkSelected(const sdbus::ObjectPath &path);
    virtual void onPropertiesChanged(const std::map<std::string, sdbus::Variant> &properties);
    virtual void onProbeRequest(const std::map<std::string, sdbus::Variant> &args);
    virtual void onCertification(const std::map<std::string, sdbus::Variant> &certification);
    virtual void onEAP(const std::string &status, const std::string &parameter);
    virtual void onStaAuthorized(const std::string &name);
    virtual void onStaDeauthorized(const std::string &name);
    virtual void onStationAdded(const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties);
    virtual void onStationRemoved(const sdbus::ObjectPath &path);
    virtual void onNetworkRequest(const sdbus::ObjectPath &path, const std::string &field, const std::string &text);
    virtual void onInterworkingAPAdded(const sdbus::ObjectPath &bss, const sdbus::ObjectPath &cred, const std::map<std::string, sdbus::Variant> &properties);
    virtual void onInterworkingSelectDone();
};

class WpaInterface
    : public sdbus::ProxyInterfaces<
          Interface_proxy_withevents>
{
public:
    using self = WpaInterface;
    using ptr = std::unique_ptr<self>;
    static ptr Create(sdbus::IConnection &connection, const sdbus::ObjectPath &dbusPath) { return std::make_unique<self>(connection, dbusPath); }

    WpaInterface(sdbus::IConnection &connection, const sdbus::ObjectPath &dbusPath)
        : ProxyInterfaces(connection, "fi.w1.wpa_supplicant1", dbusPath)
    {
        registerProxy();
    }
    virtual ~WpaInterface()
    {
        unregisterProxy();
    }

protected:
    void Trace(const char *method, const std::string &msg)
    {
        ::LogTrace(this->getObjectPath(), method, msg);
    }
};

class WpaPersistentGroup
    : public sdbus::ProxyInterfaces<
          fi::w1::wpa_supplicant1::PersistentGroup_proxy>
{
public:
    using self = WpaPersistentGroup;
    using ptr = std::unique_ptr<self>;

    static ptr Create(sdbus::IConnection &connection, const sdbus::ObjectPath &path) { return std::make_unique<self>(connection, path); }

    WpaPersistentGroup(sdbus::IConnection &connection, const sdbus::ObjectPath &objectPath)
        : ProxyInterfaces(connection, "fi.w1.wpa_supplicant1", objectPath)
    {
    }
};

class WpaWPS
    : public sdbus::ProxyInterfaces<
          fi::w1::wpa_supplicant1::Interface::WPS_proxy>
{
public:
    using self = WpaWPS;
    using ptr = std::unique_ptr<self>;
    static ptr Create(sdbus::IProxy &proxy) { return std::make_unique<self>(proxy.getObjectPath()); }
    static ptr Create(const std::string &path) { return std::make_unique<self>(path); }
    static ptr Create(sdbus::IConnection &connection, const std::string &path) { return std::make_unique<self>(connection, path); }

    WpaWPS(std::string objectPath)
        : ProxyInterfaces("fi.w1.wpa_supplicant1", objectPath)
    {
    }
    WpaWPS(sdbus::IConnection &connection, std::string objectPath)
        : ProxyInterfaces(connection, "fi.w1.wpa_supplicant1", objectPath)
    {
    }

protected:
    virtual void onEvent(const std::string &name, const std::map<std::string, sdbus::Variant> &args);
    virtual void onCredentials(const std::map<std::string, sdbus::Variant> &credentials);
    virtual void onPropertiesChanged(const std::map<std::string, sdbus::Variant> &properties);
};

class WpaPeer
    : public sdbus::ProxyInterfaces<
          fi::w1::wpa_supplicant1::Peer_proxy>
{
public:
    using self = WpaPeer;
    using ptr = std::unique_ptr<self>;
    static ptr Create(sdbus::IConnection &connection, const std::string &dbusPath) { return std::make_unique<self>(connection, dbusPath); }

    WpaPeer(sdbus::IConnection &connection, const std::string &dbusPath)
        : ProxyInterfaces(connection, "fi.w1.wpa_supplicant1", dbusPath)
    {
        registerProxy();
    }
    virtual ~WpaPeer()
    {
        unregisterProxy();
    }

private:
    virtual void onPropertiesChanged(const std::map<std::string, sdbus::Variant> &properties) {}
};

class WpaP2PDevice
    : public sdbus::ProxyInterfaces<
          fi::w1::wpa_supplicant1::Interface::P2PDevice_proxy,
          Interface_proxy_withevents>
{
public:
    using self = WpaP2PDevice;
    using ptr = std::unique_ptr<self>;
    static ptr Create(const std::string &dbusPath) { return std::make_unique<self>(dbusPath); }
    static ptr Create(sdbus::IConnection &connection, const std::string &dbusPath) { return std::make_unique<self>(connection, dbusPath); }

    WpaP2PDevice(const std::string &dbusPath)
        : ProxyInterfaces("fi.w1.wpa_supplicant1", dbusPath)
    {
        registerProxy();
    }
    WpaP2PDevice(sdbus::IConnection &connection, const std::string &dbusPath)
        : ProxyInterfaces(connection, "fi.w1.wpa_supplicant1", dbusPath)
    {
        registerProxy();
    }
    virtual ~WpaP2PDevice()
    {
        unregisterProxy();
    }
    void Disconnect()
    {
        fi::w1::wpa_supplicant1::Interface::P2PDevice_proxy::Disconnect();
    }

public:
    DBusEvent<const sdbus::ObjectPath &> OnProvisionDiscoveryEnterPin;
    DBusEvent<const sdbus::ObjectPath &, const std::string &> OnProvisionDiscoveryRequestDisplayPin;
    DBusEvent<const sdbus::ObjectPath, const std::map<std::string, sdbus::Variant> &> OnPersistentGroupAdded;
    DBusEvent<const sdbus::ObjectPath &> OnPersistentGroupRemoved;
    DBusEvent<const std::map<std::string, sdbus::Variant> &> OnGroupStarted;
    DBusEvent<const std::map<std::string, sdbus::Variant> &> OnGroupFinished;
    DBusEvent<const std::map<std::string, sdbus::Variant> &> OnServiceDiscoveryRequest;

    DBusEvent<const std::string &> OnGroupFormationFailure;
    DBusEvent<const std::map<std::string, sdbus::Variant> &> OnGONegotiationSuccess;
    DBusEvent<const std::map<std::string, sdbus::Variant> &> OnGONegotiationFailure;
    DBusEvent<const std::string &, const std::map<std::string, sdbus::Variant>> OnWpsFailed;
    DBusEvent<const std::map<std::string, sdbus::Variant>> OnInvitationResult;
    DBusEvent<const std::map<std::string, sdbus::Variant> &> OnInvitationReceived;

private:
    void EventTrace(const char *methodName, const std::string &message)
    {
        Interface_proxy_withevents::EventTrace(methodName,message);
    }

    void EventTrace(const char *eventName, const std::function<std::string()> &getText);
protected:
    virtual void onDeviceFound(const sdbus::ObjectPath &path);
    virtual void onDeviceFoundProperties(const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties);
    virtual void onDeviceLost(const sdbus::ObjectPath &path);
    virtual void onFindStopped();
    virtual void onProvisionDiscoveryRequestDisplayPin(const sdbus::ObjectPath &peer_object, const std::string &pin);
    virtual void onProvisionDiscoveryResponseDisplayPin(const sdbus::ObjectPath &peer_object, const std::string &pin);
    virtual void onProvisionDiscoveryRequestEnterPin(const sdbus::ObjectPath &peer_object);
    virtual void onProvisionDiscoveryResponseEnterPin(const sdbus::ObjectPath &peer_object);
    virtual void onProvisionDiscoveryPBCRequest(const sdbus::ObjectPath &peer_object);
    virtual void onProvisionDiscoveryPBCResponse(const sdbus::ObjectPath &peer_object);
    virtual void onProvisionDiscoveryFailure(const sdbus::ObjectPath &peer_object, const int32_t &status);

    virtual void onGroupStarted(const std::map<std::string, sdbus::Variant> &properties);
    virtual void onGroupFormationFailure(const std::string &reason);
    virtual void onGONegotiationSuccess(const std::map<std::string, sdbus::Variant> &properties);
    virtual void onGONegotiationFailure(const std::map<std::string, sdbus::Variant> &properties);
    virtual void onGONegotiationRequest(const sdbus::ObjectPath &path, const uint16_t &dev_passwd_id, const uint8_t &device_go_intent);
    virtual void onInvitationResult(const std::map<std::string, sdbus::Variant> &invite_result);
    virtual void onGroupFinished(const std::map<std::string, sdbus::Variant> &properties);
    virtual void onServiceDiscoveryRequest(const std::map<std::string, sdbus::Variant> &sd_request);
    virtual void onServiceDiscoveryResponse(const std::map<std::string, sdbus::Variant> &sd_response);
    virtual void onPersistentGroupAdded(const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties);
    virtual void onPersistentGroupRemoved(const sdbus::ObjectPath &path);
    virtual void onWpsFailed(const std::string &name, const std::map<std::string, sdbus::Variant> &args);
    virtual void onInvitationReceived(const std::map<std::string, sdbus::Variant> &properties);

};

class WpaGroup
    : public sdbus::ProxyInterfaces<fi::w1::wpa_supplicant1::Group_proxy>,
      public DBusLog
{
public:
    using self = WpaGroup;
    using ptr = std::unique_ptr<self>;
    static ptr Create(sdbus::IConnection &connection, const sdbus::ObjectPath &path) { return std::make_unique<self>(connection, path); }

    WpaGroup(sdbus::IConnection &connection, const sdbus::ObjectPath &path)
        : ProxyInterfaces(connection, "fi.w1.wpa_supplicant1", path), DBusLog(path)
    {
        registerProxy();
    }
    virtual ~WpaGroup()
    {
        unregisterProxy();
    }
    DBusEvent<const sdbus::ObjectPath &> OnPeerJoined;
    DBusEvent<const sdbus::ObjectPath &> OnPeerDisconnected;

private:
    virtual void onPeerJoined(const sdbus::ObjectPath &peer)
    {
        OnPeerJoined.fire(peer);
    }
    virtual void onPeerDisconnected(const sdbus::ObjectPath &peer)
    {
        OnPeerDisconnected.fire(peer);
    }
};

class WpaSupplicant
    : public sdbus::ProxyInterfaces<fi::w1::wpa_supplicant1_proxy, org::freedesktop::DBus::Properties_proxy>,
      public DBusLog
{
public:
    using self = WpaSupplicant;
    using ptr = std::unique_ptr<self>;
    static ptr Create() { return std::make_unique<self>(); }
    static ptr Create(sdbus::IConnection &connection) { return std::make_unique<self>(connection); }

    WpaSupplicant()
        : ProxyInterfaces("fi.w1.wpa_supplicant1", "/fi/w1/wpa_supplicant1"),
          DBusLog("fi/w1/WpaSupplicant1")
    {
        registerProxy();
    }
    WpaSupplicant(sdbus::IConnection &connection)
        : ProxyInterfaces(connection, "fi.w1.wpa_supplicant1", "/fi/w1/wpa_supplicant1"),
          DBusLog("fi/w1/WpaSupplicant1")
    {
        registerProxy();
    }
    virtual ~WpaSupplicant()
    {
        unregisterProxy();
    }
    DBusEvent<const sdbus::ObjectPath &, const std::map<std::string, sdbus::Variant> &> OnInterfaceAdded;
    DBusEvent<const sdbus::ObjectPath &> OnInterfaceRemoved;

private:
    virtual void onInterfaceAdded(const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties);
    virtual void onInterfaceRemoved(const sdbus::ObjectPath &path);
    virtual void onPropertiesChanged(const std::map<std::string, sdbus::Variant> &properties) {}
};
