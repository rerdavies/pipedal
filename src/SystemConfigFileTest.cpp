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

static std::string GetTestFile()
{
    std::stringstream s;
    s
        << "interface=wlan0" << std::endl
        << "driver=nl80211" << std::endl
        << "country_code=CA" << std::endl
        << "" << std::endl
        << "# Wi-Fi features" << std::endl
        << "ieee80211n=1" << std::endl
        << "ieee80211d=1" << std::endl
        << "ieee80211ac=1" << std::endl
        << "ht_capab=[HT40][SHORT-GI-20][DSSS_CCK-40]" << std::endl
        << "" << std::endl
        << "# Authentication options" << std::endl
        << "wmm_enabled=1" << std::endl
        << "auth_algs=1" << std::endl
        << "wpa=2" << std::endl
        << "wpa_pairwise=TKIP CCMP" << std::endl
        << "wpa_ptk_rekey=600" << std::endl
        << "rsn_pairwise=CCMP" << std::endl
        << "" << std::endl
        << "# Access point configuration" << std::endl
        << "ssid=pipedal" << std::endl
        << "hw_mode=g" << std::endl
        << "channel=8" << std::endl
        << "wpa_passphrase=PASSWORD" << std::endl;

    return s.str();
}
static std::string GetExpectedFile()
{
    std::stringstream s;
    s
        << "interface=wlan0" << std::endl
        << "driver=nl80211" << std::endl
        << "country_code=FR" << std::endl
        << "" << std::endl
        << "# Wi-Fi features" << std::endl
        << "ieee80211n=1" << std::endl
        << "ieee80211d=1" << std::endl
        << "ieee80211ac=1" << std::endl
        << "ht_capab=[HT40][SHORT-GI-20][DSSS_CCK-40]" << std::endl
        << "" << std::endl
        << "# Authentication options" << std::endl
        << "wmm_enabled=1" << std::endl
        << "auth_algs=1" << std::endl
        << "wpa=2" << std::endl
        << "wpa_pairwise=TKIP CCMP" << std::endl
        << "wpa_ptk_rekey=600" << std::endl
        << "rsn_pairwise=CCMP" << std::endl
        << "" << std::endl
        << "# Access point configuration" << std::endl
        << "ssid=newSsid" << std::endl
        << "hw_mode=g" << std::endl
        << "channel=8" << std::endl
        << "wpa_passphrase=PASSWORD" << std::endl
        << "" << std::endl
        << "# A new feature." << std::endl
        << "xx=new_feature" << std::endl;
    return s.str();
}

TEST_CASE("SystemConfigFile Test", "[system_config_file_test]")
{

    std::stringstream testFile(GetTestFile());

    SystemConfigFile file;
    file.Load(testFile);

    REQUIRE(file.Get("driver") == "nl80211");

    file.Set("ssid", "ssid", "Name of the access point");
    file.Set("xx", "new_feature", "A new feature.");
    file.Set("ssid", "newSsid");
    file.Set("country_code", "FR");

    std::stringstream outputFile;
    file.Save(outputFile);

    std::string stringOutput = outputFile.str();
    std::cout << stringOutput;

    std::string expected = GetExpectedFile();
    REQUIRE(stringOutput == expected);
}
