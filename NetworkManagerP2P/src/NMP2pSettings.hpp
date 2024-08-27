#pragma once
#include <string>
#include <vector>
#include <filesystem>
#include "json.hpp"


class P2pSettings {
    // adapter between nm p2p settings, and legacy p2p                                                                      
public:
    P2pSettings(
        const std::filesystem::path&configDirectory = "/etc/pipedal/config", 
        const std::filesystem::path&varDirectory = "/var/pipedal/config" );

    void Load();
    void Save();

private:
    bool valid_ = false;
    std::filesystem::path configDirectoryPath;
    std::filesystem::path varDirectoryPath;
    bool auth_changed_ = true;
    std::vector<uint8_t> device_type_{ 
        0x00,0x01, 0x00,0x50,0xF2,0x04,  0x00,0x02 /*"1-0050F204-2";*/ 
    };
    std::string wlan_ = "wlan0";
    std::string driver_ = "nl80211";
    std::string bssid_ = "";    
    std::string country_ = "US";
    bool persistent_reconnect_ = true;
    std::vector<std::string> config_methods_ = {"display" };
    std::string manufacturer_ = "PiPedal Project";
    std::string model_name_ = "PiPedal";
    std::string model_number_ = "3";    
    std::string serial_number_ = "1";
    std::string pin_ = "12345678";
    std::string service_uuid_ = "233e7c60-9f06-40c5-a53a-875279a44751"; 
    int32_t op_reg_class_ = 81;
    int32_t op_channel_ = 1; 
    int32_t listen_channel_ = 1;
    int32_t listen_reg_class_ = 81; 
    int web_server_port_ = 80; 
    std::string device_name_ = "PiPedalS5";
public:
    bool valid() const { return valid_; }
    std::string device_name() const { return device_name_; }

    const std::vector<uint8_t>& device_type() const  { return device_type_; }
    bool auth_changed() const { return auth_changed_; }
    void auth_changed(bool value) { auth_changed_ = value; }
    std::string wlan() const { return wlan_; }

    std::string driver() const { return driver_; }
    std::string wpas_config_file() const { 
        // "/etc/pipedal/config/wpa_supplicant/wpa_supplicant-pipedal.conf"; }
        return (configDirectoryPath / "wpa_supplicant" / "wpa_supplicant-pipedal.conf").string();
    };

    std::string dnsmasq_config_file() const { 
        // "/etc/pipedal/config/wpa_supplicant/wpa_supplicant-pipedal.conf"; }
        return (configDirectoryPath / "dnsmasq" / "dnsmasq-pipedal-nm.conf").string();
    };
    std::filesystem::path config_filename() const { 
        // "/etc/pipedal/config/wpa_supplicant/wpa_supplicant-pipedal.conf"; }
        return (varDirectoryPath / "NetworkManagerP2P.json").string();
    };

    const std::string& bssid() const { return bssid_; }
    std::string country() const { return country_; }
    bool persistent_reconnect() const  { return persistent_reconnect_; }  
    uint8_t p2p_go_intent() const  { return 15; }
    //const std::vector<std::string>& config_methods() const { return config_methods_;} 
    const std::string& manufacturer() const { return manufacturer_;  }
    const std::string& model_name() const { return model_name_;  }
    const std::string& model_number() const { return model_number_;  }
    const std::string& serial_number() const {return serial_number_;   }
    const std::string& pin() const { return pin_; }

    const std::string &service_uuid() const { return service_uuid_; }
    int32_t op_reg_class() { return op_reg_class_;  }
    int32_t op_channel() { return op_channel_;  } // Should be social channel 1, 6 or 11.
    int32_t op_frequency() const;

    int32_t listen_reg_class() { return listen_reg_class_; }
    int32_t listen_channel() { return listen_channel_;  /*ch1*/ } // . Should be social channel 1, 6 or 11.

    int web_server_port() { return web_server_port_; }
public:
    DECLARE_JSON_MAP(P2pSettings);
};