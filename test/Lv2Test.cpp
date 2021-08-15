#include "pch.h"
#include "catch.hpp"
#include <string>
#include "Lv2Host.hpp"

using namespace piddle;

TEST_CASE( "Lv2 Plugins", "[lv2_plugins]" ) {

    Lv2Host lv2Host;

    lv2Host.Load();


    {
        json_writer writer (std::cerr);
       writer.write((lv2Host.GetPlugins()));
    }
    

}

TEST_CASE( "Lv2 UI Plugins", "[lv2_ui_plugins]" ) {

    Lv2Host lv2Host;

    lv2Host.Load();

    {
        json_writer writer (std::cerr);
       writer.write((lv2Host.GetUiPlugins()));
    }
    

}

TEST_CASE( "Lv2 Classes Plugins", "[lv2_classes]" ) {

    Lv2Host lv2Host;

    lv2Host.Load();

    auto rootPlugin = lv2Host.GetLv2PluginClass();
    {
        json_writer writer (std::cerr);
        writer.write(rootPlugin);
    }
   

}