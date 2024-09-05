#include "NMP2pSettings.hpp"
#include "WifiDirectConfigSettings.hpp" 
#include "ServiceConfiguration.hpp"
#include <stdexcept>
#include "ss.hpp"
#include <fstream>
#include <random>
#include <stdexcept>
#include "WifiRegs.hpp"
#include "ChannelInfo.hpp"
#include "DBusLog.hpp"
#include <sys/stat.h>
#include "ofstream_synced.hpp"

using namespace pipedal;
P2pSettings::P2pSettings(const std::filesystem::path&configDirectoryPath, const std::filesystem::path&varDirectoryPath)
:   configDirectoryPath(configDirectoryPath),
    varDirectoryPath(varDirectoryPath)

{

}
  
static const char*hexDigits = "0123456789abcdef";
std::string MakeBssid()
{
    // "de:a6:32:d4:a1:66"
    std::random_device rDev;
    std::uniform_int_distribution<uint8_t> dist(0,255);
    uint8_t bssid[6];

    for (size_t i = 0; i < 6; ++i)
    {
        bssid[i] = dist(rDev);
    }

    std::stringstream ss;
    bool firstTime = true;

    for (size_t i = 0; i < 6; ++i)
    {
        if (!firstTime)
        {
            ss << ':';
        }
        firstTime = false;
        ss << hexDigits[(bssid[i] >> 4) & 0x0F];
        ss << hexDigits[(bssid[i] >> 0) & 0x0F];
        
    }
    return ss.str();

}
void P2pSettings::Load()
{
    bool configChanged = true;

    // load our json config.
    {
        auto filename = config_filename();
        if (std::filesystem::exists(filename))
        {
            try {
                std::ifstream f;
                f.open(filename);
                if (!f)
                {
                    throw std::runtime_error(SS("Can't open file."));
                }
                try {
                    json_reader reader{f};
                    reader.read(this);
                    configChanged = false;
                } catch (const std::exception &e)
                {
                    throw;
                }
            } catch (const std::exception &e)
            {
                throw std::runtime_error(SS(e.what() << " (" << filename.string() << ")"));
            }
        } 

    }

    if (this->bssid_.length() == 0)
    {
        configChanged = true;
        this->bssid_ = MakeBssid();
    }
    // copy UI settings into our settings.
    WifiDirectConfigSettings wifiDirectSettings;    // settings from the UI.
    wifiDirectSettings.Load();

    valid_ = false;
    if (!wifiDirectSettings.valid_)
    {
        LogError("NMP2PSettings","Load","Wifi Direct Config Settings are not valid.");
        return;
    }
    if (!wifiDirectSettings.enable_)
    {
        LogError("NMP2PSettings","Load","Wifi Direct service is not enabled.");
        return;
    }
    valid_ = true;
    bool updateRegClass = false;
    if (this->country_ != wifiDirectSettings.countryCode_)
    {
        this->country_ = wifiDirectSettings.countryCode_;
        configChanged = true;
        updateRegClass = true;
    }
    {
        int channel = 1;
        std::stringstream ss(wifiDirectSettings.channel_);
        ss >> channel;

        if (this->op_channel_ != channel || this->listen_channel_ != channel)
        {
            updateRegClass = true;
            configChanged = true;
        }
        this->op_channel_ = channel;
        this->listen_channel_ = channel;
        if (updateRegClass || this->listen_reg_class_ == 0 || this->op_reg_class_ == 0)
        {
            // todo: we currently ignore 80khz ac channels (which pi supports)
            // 
            int32_t reg_class = 0;
            if (channel == 0) // don't force a channel
            {
                reg_class = 0;
            } else {
                reg_class = getWifiRegClass(this->country_,channel,40);
                if (reg_class == -1)
                {
                    throw std::runtime_error(SS("Wifi Channel " << channel << " is not allowed in country " << this->country_));
                }
            }
            configChanged |= this->op_reg_class_ != reg_class;
            configChanged |= this->listen_reg_class_ != reg_class;

            this->op_reg_class_ = reg_class;
            this->listen_reg_class_ = reg_class;
        }
    }
    if (updateRegClass)
    {
    }

    if (wifiDirectSettings.hotspotName_ != this->device_name_)
    {
        configChanged = true;
        this->device_name_ = wifiDirectSettings.hotspotName_;
        this->auth_changed_ = true;
    }
    if (this->pin_ != wifiDirectSettings.pin_)
    {
        configChanged = true;
        this->auth_changed_ = true;
        this->pin_ = wifiDirectSettings.pin_;
    }
    if (this->wlan_ != wifiDirectSettings.wlan_)
    {
        configChanged = true;
        this->auth_changed_ = true;
        this->wlan_ = wifiDirectSettings.wlan_;
    }

    ServiceConfiguration serviceConfiguration;
    serviceConfiguration.Load();
    if ((int)serviceConfiguration.server_port != this->web_server_port_)
    {
        configChanged = true;
        this->web_server_port_ = (int)serviceConfiguration.server_port;
    }
    if (this->service_uuid() != serviceConfiguration.uuid)
    {
        configChanged = true;
        this->service_uuid_ = serviceConfiguration.uuid;
    }
    
    if (wifiDirectSettings.pinChanged_)
    {
        wifiDirectSettings.pinChanged_ = false;
        wifiDirectSettings.Save();
    }
    if (configChanged)
    {
        Save();
    }

}

static void openWithPerms(
    pipedal::ofstream_synced &f, 
    const std::filesystem::path &path, 
    std::filesystem::perms perms = 
        std::filesystem::perms::owner_read | std::filesystem::perms::owner_write |
        std::filesystem::perms::group_read | std::filesystem::perms::group_write)
{
    auto directory = path.parent_path();
    if (!std::filesystem::exists(directory))
    {
        std::filesystem::create_directories(directory);
        // open and close to make an existing empty file.
        // close it.
        {
            std::ofstream f;
            f.open(path);
            f.close();
        }
        try {
            // set the perms.
            std::filesystem::permissions(
                path,
                perms,
                std::filesystem::perm_options::replace); 
        } catch (const std::exception&) {
        }
    }

    // open for re3al.
    f.open(path);
}
void P2pSettings::Save()
{
    auto filename = config_filename();
    try {
        pipedal::ofstream_synced f;
        openWithPerms(f,filename);

        if (!f.is_open())
        {
            throw std::runtime_error("Can't write to file.");
        }
        json_writer writer{f,false};
        writer.write(this);
    } catch (const std::exception &e)
    {
        throw std::runtime_error(SS(e.what() << " (" << filename.string() << ")"));
    }
}

int32_t P2pSettings::op_frequency() const
{
    return wifiChannelToFrequency(this->op_channel_);
    
}
JSON_MAP_BEGIN(P2pSettings)
JSON_MAP_REFERENCE(P2pSettings,wlan)
JSON_MAP_REFERENCE(P2pSettings,driver)
JSON_MAP_REFERENCE(P2pSettings,bssid)
JSON_MAP_REFERENCE(P2pSettings,country)
JSON_MAP_REFERENCE(P2pSettings,persistent_reconnect)
JSON_MAP_REFERENCE(P2pSettings,config_methods)
JSON_MAP_REFERENCE(P2pSettings,manufacturer)
JSON_MAP_REFERENCE(P2pSettings,model_name)
JSON_MAP_REFERENCE(P2pSettings,model_number)
JSON_MAP_REFERENCE(P2pSettings,serial_number)
JSON_MAP_REFERENCE(P2pSettings,pin)
JSON_MAP_REFERENCE(P2pSettings,service_uuid)
JSON_MAP_REFERENCE(P2pSettings,listen_channel)
JSON_MAP_REFERENCE(P2pSettings,listen_reg_class)
JSON_MAP_REFERENCE(P2pSettings,op_channel)
JSON_MAP_REFERENCE(P2pSettings,op_reg_class)
JSON_MAP_REFERENCE(P2pSettings,web_server_port)
JSON_MAP_REFERENCE(P2pSettings,device_name)
JSON_MAP_REFERENCE(P2pSettings,auth_changed)


JSON_MAP_END()