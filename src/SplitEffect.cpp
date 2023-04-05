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
#include "SplitEffect.hpp"

#include "PluginHost.hpp"
#include "Pedalboard.hpp"

using namespace pipedal;


static std::shared_ptr<Lv2PortInfo> MakeBypassPortInfo() {
    std::shared_ptr<Lv2PortInfo> bypassPortInfo = std::make_shared<Lv2PortInfo>();
    bypassPortInfo->symbol("__bypass");
    bypassPortInfo->name("Bypass");
    bypassPortInfo->is_input(true);
    bypassPortInfo->is_control_port(true);
    bypassPortInfo->index(-1);
    bypassPortInfo->min_value(0);
    bypassPortInfo->max_value(1);
    bypassPortInfo->default_value(1);
    bypassPortInfo->toggled_property(true);
    return bypassPortInfo;
}

std::shared_ptr<Lv2PortInfo> g_BypassPortInfo = MakeBypassPortInfo();

// Pseudo-port info used to perform midi port value mapping.
const Lv2PortInfo *pipedal::GetBypassPortInfo()
{
    return g_BypassPortInfo.get();
}
// Just enough to do control value defaulting, and midi value mapping :-/




static Lv2PluginInfo makeSplitterPluginInfo() {
    Lv2PluginInfo result;
    result.uri(SPLIT_PEDALBOARD_ITEM_URI);
    result.name("Split");
    result.author_name("Robin Davies");
    result.comment("Internal only.");
    result.is_valid(true);

    std::shared_ptr<Lv2PortInfo> splitTypeControl= std::make_shared<Lv2PortInfo>();
    splitTypeControl->symbol(SPLIT_SPLITTYPE_KEY);
    splitTypeControl->name("Type");
    splitTypeControl->is_input(true);
    splitTypeControl->is_control_port(true);
    splitTypeControl->index(0);
    splitTypeControl->min_value(0);
    splitTypeControl->max_value(2);
    splitTypeControl->default_value(0);
    splitTypeControl->enumeration_property(true);
    splitTypeControl->scale_points().push_back(
        Lv2ScalePoint(0,"A/B")
    );
    splitTypeControl->scale_points().push_back(
        Lv2ScalePoint(1,"Mix")
    );
    splitTypeControl->scale_points().push_back(
        Lv2ScalePoint(1,"L/R")
    );
    result.ports().push_back(splitTypeControl);


    std::shared_ptr<Lv2PortInfo> abControl= std::make_shared<Lv2PortInfo>();
    abControl->symbol(SPLIT_SELECT_KEY);
    abControl->name("Select");
    abControl->is_input(true);
    abControl->is_control_port(true);
    abControl->index(1);
    abControl->min_value(0);
    abControl->max_value(1);
    abControl->default_value(0);
    abControl->enumeration_property(true);
    abControl->scale_points().push_back(
        Lv2ScalePoint(0,"A")
    );
    abControl->scale_points().push_back(
        Lv2ScalePoint(1,"B")
    );
    result.ports().push_back(abControl);

    std::shared_ptr<Lv2PortInfo> mixControl= std::make_shared<Lv2PortInfo>();
    mixControl->symbol(SPLIT_MIX_KEY);
    mixControl->name("Mix");
    mixControl->is_input(true);
    mixControl->is_control_port(true);
    mixControl->index(2);
    mixControl->min_value(-1);
    mixControl->max_value(1);
    mixControl->default_value(0);

    result.ports().push_back(mixControl);

    std::shared_ptr<Lv2PortInfo> panLControl= std::make_shared<Lv2PortInfo>();
    panLControl->symbol(SPLIT_PANL_KEY);
    panLControl->name("Pan L");
    panLControl->is_input(true);
    panLControl->is_control_port(true);
    panLControl->index(3);
    panLControl->min_value(-1);
    panLControl->max_value(1);
    panLControl->default_value(0);

    result.ports().push_back(panLControl);

    std::shared_ptr<Lv2PortInfo> volLControl= std::make_shared<Lv2PortInfo>();
    volLControl->symbol(SPLIT_VOLL_KEY);
    volLControl->name("Vol L");
    volLControl->is_input(true);
    volLControl->is_control_port(true);
    volLControl->index(4);
    volLControl->min_value(-60);
    volLControl->max_value(12);
    volLControl->default_value(-3);
    volLControl->scale_points().push_back(
        Lv2ScalePoint(-60,"-INF")
    );

    result.ports().push_back(volLControl);

    std::shared_ptr<Lv2PortInfo> panRControl= std::make_shared<Lv2PortInfo>();
    panRControl->symbol(SPLIT_PANR_KEY);
    panRControl->name("Pan R");
    panRControl->is_input(true);
    panRControl->is_control_port(true);
    panRControl->index(5);
    panRControl->min_value(-1);
    panRControl->max_value(1);
    panRControl->default_value(0);

    result.ports().push_back(panRControl);

    std::shared_ptr<Lv2PortInfo> volRControl= std::make_shared<Lv2PortInfo>();
    volRControl->symbol(SPLIT_VOLR_KEY);
    volRControl->name("Vol R");
    volRControl->is_input(true);
    volRControl->is_control_port(true);
    volRControl->index(6);
    volRControl->min_value(-60);
    volRControl->max_value(12);
    volRControl->default_value(-3);
    volRControl->scale_points().push_back(
        Lv2ScalePoint(-60,"-INF")
    );

    result.ports().push_back(volRControl);





    return result;
}


Lv2PluginInfo g_splitterPluginInfo = makeSplitterPluginInfo();

const Lv2PluginInfo * pipedal::GetSplitterPluginInfo() { return &g_splitterPluginInfo; }





