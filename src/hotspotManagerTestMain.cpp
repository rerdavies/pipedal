#include "HotspotManager.hpp"
#include "WifiConfigSettings.hpp"
#include <iostream>
#include "Lv2Log.hpp"
#include "DBusLog.hpp"

using namespace pipedal;

int main(int argc, char**argv)
{
    WifiConfigSettings settings;
    settings.Load();
    if (!settings.valid_ || !settings.hasPassword_)
    {
        settings.valid_ = true;
        settings.channel_ = "1";
        settings.hotspotName_ = "pipedal";
        settings.password_ = "password";
        settings.hasPassword_ = true;
        settings.Save();
    }

    Lv2Log::log_level(LogLevel::Debug);
    SetDBusLogLevel(DBusLogLevel::Trace);

    HotspotManager::ptr hotspotManager = HotspotManager::Create();
    hotspotManager->Open();

    while (true)
    {
        std::string line;
        std::cout << "e=enable,x=disable,q=quit > " << std::endl;;
        std::getline(std::cin,line);
    
        if (line == "e")
        {
            settings.Load();
            settings.enable_ = true;
            settings.Save();
            hotspotManager->Reload();
        } else if (line == "x")
        {
            settings.Load();
            settings.enable_ = false;
            settings.Save();
            hotspotManager->Reload();
        } else if (line == "q") {
            break;
        } else {
            std::cout << "Invalid command." << std::endl;
        }
    }
    hotspotManager->Close();
    hotspotManager = nullptr;




    return EXIT_SUCCESS;
}

