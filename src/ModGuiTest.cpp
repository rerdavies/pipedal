// Copyright (c) 2025 Robin Davies
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
#include <iostream>
#include <atomic>
#include <thread>
#include "ModTemplateGenerator.hpp"

#include "PiPedalModel.hpp"

using namespace pipedal;
using namespace std;

class PluginHostTest {

public:
    static void TestInit()
    {
        PiPedalModel model;
        model.GetPluginHost().LoadLilv();
        const auto&plugins = model.GetPluginHost().GetPlugins();
        REQUIRE(!plugins.empty());
        size_t modGuicount = 0;
        for (const auto&plugin : plugins)
        {
            REQUIRE(plugin != nullptr);

            if (plugin->modGui())
            {
                modGuicount++;
                const auto&modGui = plugin->modGui();
                REQUIRE(!modGui->iconTemplate().empty());
                REQUIRE(modGui->javascript().empty());
                REQUIRE(!modGui->stylesheet().empty());
                REQUIRE(!modGui->screenshot().empty());
                REQUIRE(!modGui->thumbnail().empty());
            }
        }
    }   
};

TEST_CASE("ModGui Init Test", "[mod_gui_init]")
{
    PluginHostTest::TestInit();
}

TEST_CASE("ModGui Templates", "[mod_gui_templates]")
{

    json_variant data = json_variant::make_object();
    data["name"] = "Test Plugin";
    data["uri"] = "http://example.com/test-plugin";
    data["version"] = "1.0.0";
    data["author"] = "Test Author";
    data["cn"] = "14323";
    json_variant items = json_variant::make_array();
    for (size_t i = 0; i < 3; ++i)
    {
        json_variant item = json_variant::make_object();
        item["label"] = "Item " + std::to_string(i + 1);
        items.as_array()->push_back(item);
        
    }
    data["items"] = items;  

    data["inputs"] = json_variant::make_object();
    data["inputs"]["input1"] = "Input 1";
    data["inputs"]["input2"] = "Input 2";

    data["_cn"] = "?cn=1234";
    data["_cns"] = "_toobamp.lv2_ToobAmp__1234";

    std::string templateString = R"(
<div>
    <h1>{{name}}</h1>
    <p>URI: {{uri}}</p>
    <p>Version: {{version}}</p>
    <p>Author: {{author}}</p>
    <ul>
        {{#items}}
        <li id="item-{{cn}}">{{label}}</li>
        {{/items}}
    </ul>
    <h2>{{inputs.input1}}</h2>
    <h2>{{inputs.input2}}</h2>
    <h2>{{inputs.input3}}</h2>
    <img src="img/helpIcon.png{{{cn}}}" class="{{{cns}}}_help_icon" alt="Help Icon" />
</div>
    )";
    auto result = GenerateFromTemplateString(templateString, data);

    cout << result;
    cout << endl;
}


