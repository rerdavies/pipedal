// Copyright (c) 2022 Robin Davies
//
// Permission is hereby granted, free of charge, to any person obtaining a copy of
// this software and associated documentation files (the "Software"), to deal in
// the Software without restriction, including without limitation the rights to
// use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
// the Software, and to permit persons to whom the Software is furnished to do so,
// subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
// FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
// COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
// IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
// CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

#include "pch.h"
#include "catch.hpp"
#include <sstream>
#include <cstdint>
#include <string>
#include "PiPedalException.hpp"
#include <iostream>


#include "SystemConfigFile.hpp"


using namespace pipedal;


TEST_CASE( "SystemConfigFile Test", "[system_config_file_test]" ) {
    std::filesystem::path path("/etc/hostapd/hostapd.conf");

    SystemConfigFile file(path);
    std::string driverName;

    REQUIRE(file.Get("driver") == "nl80211");

    file.Set("ssid","ssid","Name of the access point");
    file.Set("xx","new_feature","A new feature.");
    file.Set("ssid","newSsid");
    file.Set("country_code","FR");
    

    file.Save(std::cout);



}

