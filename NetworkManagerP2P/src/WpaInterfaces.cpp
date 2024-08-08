#include "WpaInterfaces.hpp"
#include "ss.hpp"
#include "DBusLog.hpp"

void Interface_proxy_withevents::EventTrace(const char *methodName, const std::string &message)
{
    ::LogTrace(proxy.getObjectPath(), methodName, message);
}

void Interface_proxy_withevents::onScanDone(const bool &success)
{
    this->EventTrace("onScanDone", SS("ScanDone " << success));
}

void Interface_proxy_withevents::onBSSAdded(const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties)
{
    // too chatty
    // this->LogTrace("onBSSAdded", "");
}

void Interface_proxy_withevents::onBSSRemoved(const sdbus::ObjectPath &path)
{
    // too chatty.
    // this->LogTrace("onBSSRemoved", "");
}

void Interface_proxy_withevents::onBlobAdded(const std::string &name)
{
    this->EventTrace("onBlobAdded", "");
}

void Interface_proxy_withevents::onBlobRemoved(const std::string &name)
{
    this->EventTrace("onBlobRemoved", "");
}

void Interface_proxy_withevents::onNetworkAdded(const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties)
{
    std::stringstream ss;
    ss << properties;
    EventTrace("onNetworkAdded",
               SS(path.c_str() << ss.str()));
    OnNetworkAdded.fire(path, properties);
}

void Interface_proxy_withevents::onNetworkRemoved(const sdbus::ObjectPath &path)
{
    EventTrace("onNetworkRemoved",
               path);
    OnNetworkRemoved.fire(path);
}

void Interface_proxy_withevents::onNetworkSelected(const sdbus::ObjectPath &path)
{
    EventTrace("onNetworkSelected",
               path);
    OnNetworkSelected.fire(path);
}

void Interface_proxy_withevents::onPropertiesChanged(const std::map<std::string, sdbus::Variant> &properties)
{
}

void Interface_proxy_withevents::onProbeRequest(const std::map<std::string, sdbus::Variant> &args)
{
}

void Interface_proxy_withevents::onCertification(const std::map<std::string, sdbus::Variant> &certification)
{
    EventTrace("onCertification",
               "");
}

void Interface_proxy_withevents::onEAP(const std::string &status, const std::string &parameter)
{
    EventTrace("onEAP",
               SS("status: " << status << " parameter: " << parameter));
}

void Interface_proxy_withevents::onStaAuthorized(const std::string &name)
{
    EventTrace("onStaAuthorized",
               SS("name: " << name));
    OnStaAuthorized.fire(name);
}

void Interface_proxy_withevents::onStaDeauthorized(const std::string &name)
{
    EventTrace("onStaDeauthorized",
               SS("name: " << name));
    OnStaDeauthorized.fire(name);
}

void Interface_proxy_withevents::onStationAdded(const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties)
{
    std::stringstream ss;
    ss << properties;
    EventTrace("onStationAdded",
               SS("path: " << path.c_str() << " properties: " << ss.str()));
    OnStationAdded.fire(path, properties);
}

void Interface_proxy_withevents::onStationRemoved(const sdbus::ObjectPath &path)
{
    EventTrace("onStationRemoved",
               SS("path: " << path.c_str()));
    OnStationRemoved.fire(path);
}

void Interface_proxy_withevents::onNetworkRequest(const sdbus::ObjectPath &path, const std::string &field, const std::string &text)
{
    EventTrace("onNetworkRequest",
               SS("path: " << path.c_str() << " field: " << field << " text: " << text));
}

void Interface_proxy_withevents::onInterworkingAPAdded(const sdbus::ObjectPath &bss, const sdbus::ObjectPath &cred, const std::map<std::string, sdbus::Variant> &properties)
{
    EventTrace("onInterworkingAPAdded",
               "");
}

void Interface_proxy_withevents::onInterworkingSelectDone()
{
    EventTrace("onInterworkingSelectDone",
               "");
}

void WpaP2PDevice::onDeviceFound(const sdbus::ObjectPath &path)
{
    EventTrace("onDeviceFound",
               path.c_str());
}
void WpaP2PDevice::onDeviceFoundProperties(const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties)
{
    if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
        std::stringstream ss;
        ss << properties;
        EventTrace("onDeviceFoundProperties",
                   [this, &path, &properties]()
                   {
                       return std::string();
                       // return  SS("path: " << path.c_str() << "properties: " << ss.str())
                   });
    }
}
void WpaP2PDevice::onDeviceLost(const sdbus::ObjectPath &path)
{
    EventTrace("onDeviceLost",
               path.c_str());
}
void WpaP2PDevice::onProvisionDiscoveryResponseDisplayPin(const sdbus::ObjectPath &peer_object, const std::string &pin)
{
    EventTrace("onProvisionDiscoveryResponseDisplayPin",
               SS("peer: " << peer_object.c_str()));
}
void WpaP2PDevice::onProvisionDiscoveryRequestEnterPin(const sdbus::ObjectPath &peer_object)
{
    EventTrace("onProvisionDiscoveryRequestEnterPin",
               SS("peer: " << peer_object.c_str()));
    OnProvisionDiscoveryEnterPin.fire(peer_object);
}
void WpaP2PDevice::onProvisionDiscoveryResponseEnterPin(const sdbus::ObjectPath &peer_object)
{
    EventTrace("onProvisionDiscoveryResponseEnterPin",
               SS("peer: " << peer_object.c_str()));
}
void WpaP2PDevice::onProvisionDiscoveryPBCRequest(const sdbus::ObjectPath &peer_object)
{
    EventTrace("onProvisionDiscoveryPBCRequest",
               SS("peer: " << peer_object.c_str()));
}
void WpaP2PDevice::onProvisionDiscoveryPBCResponse(const sdbus::ObjectPath &peer_object)
{
    EventTrace("onProvisionDiscoveryPBCRequest",
               SS("peer: " << peer_object.c_str()));
}
void WpaP2PDevice::onInvitationResult(const std::map<std::string, sdbus::Variant> &invite_result)
{
    if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
        std::stringstream ss;
        ss << invite_result;
        EventTrace("onInvitationResult",
                   SS("invite_result: " << ss.str()));
    }
    OnInvitationResult.fire(invite_result);
}
void WpaP2PDevice::onGroupFinished(const std::map<std::string, sdbus::Variant> &properties)
{
    if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
        std::stringstream ss;
        ss << properties;
        EventTrace("onGroupFinished",
                   SS("properties: " << ss.str()));

    }
    OnGroupFinished.fire(properties);
}
void WpaP2PDevice::onServiceDiscoveryRequest(const std::map<std::string, sdbus::Variant> &sd_request)
{
    if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
        std::stringstream ss;
        ss << sd_request;
        EventTrace("onServiceDiscoveryRequest",
                   SS("properties: " << ss.str()));

    }
    OnServiceDiscoveryRequest.fire(sd_request);
}
void WpaP2PDevice::onServiceDiscoveryResponse(const std::map<std::string, sdbus::Variant> &sd_response)
{
    EventTrace("onServiceDiscoveryResponse", "");
}
void WpaP2PDevice::onWpsFailed(const std::string &name, const std::map<std::string, sdbus::Variant> &args)
{
    EventTrace("onWpsFailed",
               SS("name: " << name));
    OnWpsFailed.fire(name, args);
}
void WpaP2PDevice::onInvitationReceived(const std::map<std::string, sdbus::Variant> &properties)
{
    if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
        std::stringstream s;
        s << properties;
        EventTrace("onInvitationReceived", SS("properties: " << s.str()));
    }
    OnInvitationReceived.fire(properties);
}

void WpaWPS::onPropertiesChanged(const std::map<std::string, sdbus::Variant> &properties) {}

void WpaWPS::onCredentials(const std::map<std::string, sdbus::Variant> &credentials)
{
    if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
        std::stringstream s;
        s << credentials;
        LogTrace(getObjectPath(), "onCredentials", SS("credentials: " << s.str()));
    }
}

void WpaP2PDevice::onGroupStarted(const std::map<std::string, sdbus::Variant> &properties)
{
    if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
        std::stringstream s;
        s << properties;
        LogTrace(this->getObjectPath(), "onGroupStarted", SS("properties: " << s.str()));
    }
    OnGroupStarted.fire(properties);
}
void WpaP2PDevice::onGroupFormationFailure(const std::string &reason)
{
    LogTrace(this->getObjectPath(), "onGroupFormationFailure", SS("reason:" << reason));
    OnGroupFormationFailure.fire(reason);
}
void WpaP2PDevice::onGONegotiationSuccess(const std::map<std::string, sdbus::Variant> &properties)
{
    if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
        std::stringstream s;
        s << properties;
        LogTrace(this->getObjectPath(), "onGONegotiationSuccess", SS("properties: " << s.str()));
    }
    OnGONegotiationSuccess.fire(properties);
}
void WpaP2PDevice::onGONegotiationFailure(const std::map<std::string, sdbus::Variant> &properties)
{
if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
        std::stringstream s;
    s << properties;

    LogTrace(this->getObjectPath(), "onGONegotiationFailure", SS("properties: " << s.str()));
    }
    OnGONegotiationFailure.fire(properties);
}
void WpaP2PDevice::onGONegotiationRequest(const sdbus::ObjectPath &path, const uint16_t &dev_passwd_id, const uint8_t &device_go_intent)
{
if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
    
    LogTrace(this->getObjectPath(), "onGONegotiationRequest", SS(" path: " << path.c_str() << " dev_passwd_id: " << dev_passwd_id << " device_go_intent: " << (int32_t)device_go_intent));
    }
}

void WpaP2PDevice::onProvisionDiscoveryRequestDisplayPin(const sdbus::ObjectPath &peer_object, const std::string &pin)
{
    if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
    
    EventTrace("onProvisionDiscoveryRequestDisplayPin", SS("peer: " << peer_object.c_str() << " pin: " << pin));
    }
    OnProvisionDiscoveryRequestDisplayPin.fire(peer_object, pin);
}

void WpaP2PDevice::onPersistentGroupAdded(const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties)
{
if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
        std::stringstream s;
    s << properties;
    std::string propertiesString = s.str();
    EventTrace("onPersistentGroupAdded", SS("path: " << path.c_str() << " properties: " << propertiesString));
    }
    OnPersistentGroupAdded.fire(path, properties);
}
void WpaP2PDevice::onPersistentGroupRemoved(const sdbus::ObjectPath &path)
{
    EventTrace("onPersistentGroupRemoved", SS("path: " << path.c_str()));
    OnPersistentGroupRemoved.fire(path);
}

void WpaP2PDevice::onFindStopped()
{
    EventTrace("onFindStopped", "");
}

void WpaP2PDevice::onProvisionDiscoveryFailure(const sdbus::ObjectPath &peer_object, const int32_t &status)
{
if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
        EventTrace("onProvisionDiscoveryFailure", SS("object: " << peer_object << " status: " << status));
    }
}

void WpaWPS::onEvent(const std::string &name, const std::map<std::string, sdbus::Variant> &args)
{
    if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
    std::stringstream ss;
    ss << args;
    LogTrace(getObjectPath(), "onEvent", SS("object: " << name << " status: " << ss.str()));
    }
}

void WpaSupplicant::onInterfaceAdded(const sdbus::ObjectPath &path, const std::map<std::string, sdbus::Variant> &properties)
{
    if (IsDBusLoggingEnabled(DBusLogLevel::Trace))
    {
    
    std::stringstream ss;
    ss << properties;
    LogTrace("onInterfaceAdded",
             SS("path: " << path.c_str()
                //<< " properties: {" << ss.str() << "}"
                ));
    }

    OnInterfaceAdded.fire(path, properties);
}
void WpaSupplicant::onInterfaceRemoved(const sdbus::ObjectPath &path)
{
    LogTrace("onInterfaceRemoved",
             SS("path: " << path.c_str()));
    OnInterfaceRemoved.fire(path);
}

void WpaP2PDevice::EventTrace(const char *eventName, const std::function<std::string()> &getText)
{
    ::LogTrace(this->getObjectPath(), eventName, getText);
}
