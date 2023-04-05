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
#include <string>
#include "PluginHost.hpp"

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