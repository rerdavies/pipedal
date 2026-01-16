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

static std::shared_ptr<Lv2PortInfo> MakeBypassPortInfo()
{
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

static Lv2PluginInfo makeSplitterPluginInfo()
{
    Lv2PluginInfo result;
    result.uri(SPLIT_PEDALBOARD_ITEM_URI);
    result.name("Split");
    result.author_name("Robin Davies");
    result.comment("Internal only.");
    result.is_valid(true);

    std::shared_ptr<Lv2PortInfo> splitTypeControl = std::make_shared<Lv2PortInfo>();
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
        Lv2ScalePoint(0, "A/B"));
    splitTypeControl->scale_points().push_back(
        Lv2ScalePoint(1, "Mix"));
    splitTypeControl->scale_points().push_back(
        Lv2ScalePoint(1, "L/R"));
    result.ports().push_back(splitTypeControl);

    std::shared_ptr<Lv2PortInfo> abControl = std::make_shared<Lv2PortInfo>();
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
        Lv2ScalePoint(0, "A"));
    abControl->scale_points().push_back(
        Lv2ScalePoint(1, "B"));
    result.ports().push_back(abControl);

    std::shared_ptr<Lv2PortInfo> mixControl = std::make_shared<Lv2PortInfo>();
    mixControl->symbol(SPLIT_MIX_KEY);
    mixControl->name("Mix");
    mixControl->is_input(true);
    mixControl->is_control_port(true);
    mixControl->index(2);
    mixControl->min_value(-1);
    mixControl->max_value(1);
    mixControl->default_value(0);

    result.ports().push_back(mixControl);

    std::shared_ptr<Lv2PortInfo> panLControl = std::make_shared<Lv2PortInfo>();
    panLControl->symbol(SPLIT_PANL_KEY);
    panLControl->name("Pan L");
    panLControl->is_input(true);
    panLControl->is_control_port(true);
    panLControl->index(3);
    panLControl->min_value(-1);
    panLControl->max_value(1);
    panLControl->default_value(0);

    result.ports().push_back(panLControl);

    std::shared_ptr<Lv2PortInfo> volLControl = std::make_shared<Lv2PortInfo>();
    volLControl->symbol(SPLIT_VOLL_KEY);
    volLControl->name("Vol L");
    volLControl->is_input(true);
    volLControl->is_control_port(true);
    volLControl->index(4);
    volLControl->min_value(-60);
    volLControl->max_value(12);
    volLControl->default_value(0);
    volLControl->scale_points().push_back(
        Lv2ScalePoint(-60, "-INF"));

    result.ports().push_back(volLControl);

    std::shared_ptr<Lv2PortInfo> panRControl = std::make_shared<Lv2PortInfo>();
    panRControl->symbol(SPLIT_PANR_KEY);
    panRControl->name("Pan R");
    panRControl->is_input(true);
    panRControl->is_control_port(true);
    panRControl->index(5);
    panRControl->min_value(-1);
    panRControl->max_value(1);
    panRControl->default_value(0);

    result.ports().push_back(panRControl);

    std::shared_ptr<Lv2PortInfo> volRControl = std::make_shared<Lv2PortInfo>();
    volRControl->symbol(SPLIT_VOLR_KEY);
    volRControl->name("Vol R");
    volRControl->is_input(true);
    volRControl->is_control_port(true);
    volRControl->index(6);
    volRControl->min_value(-60);
    volRControl->max_value(12);
    volRControl->default_value(0);
    volRControl->scale_points().push_back(
        Lv2ScalePoint(-60, "-INF"));

    result.ports().push_back(volRControl);

    return result;
}

Lv2PluginInfo::ptr g_splitterPluginInfo = std::make_shared<Lv2PluginInfo>(makeSplitterPluginInfo());

Lv2PluginInfo::ptr pipedal::GetSplitterPluginInfo() { return g_splitterPluginInfo; }

SplitEffect::SplitEffect(
    uint64_t instanceId,
    double sampleRate,
    const std::vector<float *> &inputs)
    : instanceId(instanceId), inputs(inputs), sampleRate(sampleRate)

{
    controlIndex["splitType"] = SPLIT_TYPE_CTL;
    controlIndex["select"] = SELECT_CTL;
    controlIndex["mix"] = MIX_CTL;
    controlIndex["panL"] = PANL_CTL;
    controlIndex["volL"]  = VOLL_CTL;
    controlIndex["panR"] = PANR_CTL;
    controlIndex["volR"] = VOLR_CTL;

    Lv2PluginInfo::ptr puginInfo = g_splitterPluginInfo;

    defaultInputControlValues.resize(MAX_INPUT_CONTROL);
    for (auto&port:  puginInfo->ports())
    {   
        if (port->is_control_port()  && port->is_input())
        {
            auto index = port->index();
            if (index < defaultInputControlValues.size()) {
                defaultInputControlValues[index] = port->default_value();
            } 
        }
    } 
}

uint64_t SplitEffect::GetMaxInputControl() const  { return MAX_INPUT_CONTROL;}
bool SplitEffect::IsInputControl(uint64_t index) const 
{
    return index >= 0 && index < MAX_INPUT_CONTROL;
}
float SplitEffect::GetDefaultInputControlValue(uint64_t index) const
{
    if (index < 0 || index >= MAX_INPUT_CONTROL) return 0;
    return defaultInputControlValues[index];
}


int SplitEffect::GetControlIndex(const std::string &symbol) const
{
    auto i = controlIndex.find(symbol);
    if (i == controlIndex.end())
    {
        return -1;
    }
    return i->second;
}

void SplitEffect::Activate()
{
    activated = true;
    updateMixFunction();
    snapToMixTarget();

    mixBottomInputs.clear();
    mixTopInputs.clear();

    if (outputBuffers.size() == 1)
    {
        mixTopInputs.push_back(this->topOutputs[0]);
        mixBottomInputs.push_back(this->bottomOutputs[0]);
    }
    else
    {
        if (this->topOutputs.size() == 1)
        {
            mixTopInputs.push_back(topOutputs[0]);
            mixTopInputs.push_back(topOutputs[0]);
        }
        else
        {
            mixTopInputs.push_back(topOutputs[0]);
            mixTopInputs.push_back(topOutputs[1]);
        }
        if (this->bottomOutputs.size() == 1)
        {
            mixBottomInputs.push_back(bottomOutputs[0]);
            mixBottomInputs.push_back(bottomOutputs[0]);
        }
        else
        {
            mixBottomInputs.push_back(bottomOutputs[0]);
            mixBottomInputs.push_back(bottomOutputs[1]);
        }
    }
}
void SplitEffect::Deactivate()
{
    activated = false;
}

void SplitEffect::SetChainBuffers(
    const std::vector<float *> &topInputs,
    const std::vector<float *> &bottomInputs,
    const std::vector<float *> &topOutputs,
    const std::vector<float *> &bottomOutputs,
    bool forceStereo)
{
    this->topInputs = topInputs;
    this->bottomInputs = bottomInputs;
    this->topOutputs = topOutputs;
    this->bottomOutputs = bottomOutputs;

    if (forceStereo)
    {
        numberOfOutputPorts = 2;
    } else {
        if (topOutputs.size() > 1 || bottomOutputs.size() > 1)
        {
            numberOfOutputPorts = 2;
        } else {
            numberOfOutputPorts = 1;
        }
    }
    if (this->topOutputs.size() == 1 && numberOfOutputPorts != 1)
    {
        this->topOutputs.push_back(this->topOutputs[0]);
    }
    if (this->bottomOutputs.size() == 1 && numberOfOutputPorts != 1)
    {
        this->bottomOutputs.push_back(this->bottomOutputs[0]);
    }
    outputBuffers.resize(numberOfOutputPorts);
}

void SplitEffect::snapToMixTarget()
{
    this->blendLTop = targetBlendLTop;
    this->blendRTop = targetBlendRTop;
    this->blendRBottom = targetBlendRBottom;
    this->blendLBottom = targetBlendLBottom;
    this->blendFadeSamples = 0;
    this->blendDxLTop = 0;
    this->blendDxRTop = 0;
    this->blendDxLBottom = 0;
    this->blendDxRBottom = 0;
}

void SplitEffect::mixToTarget()
{
    uint32_t transitionSamples = (uint32_t)(this->sampleRate * MIX_TRANSITION_TIME_S);
    if (transitionSamples < 1)
        transitionSamples = 1;
    double dxScale = 1.0 / transitionSamples;
    this->blendFadeSamples = transitionSamples;
    this->blendDxLTop = dxScale * (this->targetBlendLTop - this->blendLTop);
    this->blendDxRTop = dxScale * (this->targetBlendRTop - this->blendRTop);
    this->blendDxLBottom = dxScale * (this->targetBlendLBottom - this->blendLBottom);
    this->blendDxRBottom = dxScale * (this->targetBlendRBottom - this->blendRBottom);
}

void SplitEffect::mixTo(float value)
{
    float blend = (value + 1) * 0.5f;
    this->targetBlendRTop = this->targetBlendLTop = 1 - blend;
    this->targetBlendLBottom = this->targetBlendRBottom = blend;
    mixToTarget();
}
static inline void applyPan(float pan, float*left, float*right) {
    float panClipped = pan;
    if (panClipped < -1) panClipped = -1;
    if (panClipped > 1) panClipped = 1;
    panClipped = (panClipped + 1) * 0.5f; // 0..1
    // linear panning law.
    *left = 1.0f - panClipped;
    *right = panClipped;
    
        // float panClipped = pan;
    // if (panClipped < -1) panClipped = -1;
    // if (panClipped > 1) panClipped = 1;
    // if (pan <= 0) {
    //     *left = 1.0f;
    //     *right = 1.0f + panClipped;
    // } else {
    //     *left = 1.0f - panClipped;
    //     *right = 1.0f;
    // }
    // return panClipped;

}

void SplitEffect::mixTo(float panL, float volL, float panR, float volR)
{
    float aTop = (volL <= SPLIT_DB_MIN) ? 0 : db2a(volL);
    float aBottom = (volR <= SPLIT_DB_MIN) ? 0 : db2a(volR);
    if (this->outputBuffers.size() == 1)
    {
        // ignore pan. The R values actually have no effect.
        this->targetBlendLTop = this->targetBlendRTop = aTop;
        this->targetBlendLBottom = this->targetBlendRBottom = aBottom;
    }
    else
    {
        float panTopL, panTopR;
        applyPan(panL,&panTopL, &panTopR);
        float panBottomL, panBottomR;
        applyPan(panR,&panBottomL, &panBottomR);

        this->targetBlendLTop = panTopL * aTop;
        this->targetBlendRTop = panTopR*aTop;
        this->targetBlendLBottom = panBottomL * aBottom;
         this->targetBlendRBottom = panBottomR * aBottom;
    }
    mixToTarget();
}
float SplitEffect::GetControlValue(int portIndex) const
{
    switch (portIndex)
    {
    case -1: /* (bypass) */
        return 1;
    case SPLIT_TYPE_CTL:
    {
        return (int)(this->splitType);
    }
    case SELECT_CTL:
    {
        return selectA ? 0 : 1;
    }
    case MIX_CTL:
        return mix;
    case PANL_CTL:
        return this->panL;
    case VOLL_CTL:
        return this->volL;
    case PANR_CTL:
        return this->panR;
    case VOLR_CTL:
        return this->volR;
    default:
        throw PiPedalArgumentException("Invalid argument");
    }
}



void SplitEffect::SetControl(int index, float value)
{
    switch (index)
    {
    case -1:    /* (bypass) */
        return; // no can bypass.
    case SPLIT_TYPE_CTL:
    {
        SplitType t = valueToSplitType(value);
        if (splitType != t)
        {
            splitType = t;
            updateMixFunction();
        }
        break;
    }
    case SELECT_CTL:
    {
        bool t = value == 0;
        if (selectA != t)
        {
            selectA = t;

            if (splitType == SplitType::Ab)
            {
                mixTo(selectA ? -1 : 1);
            }
        }
        break;
    }
    case MIX_CTL:
        mix = value;
        if (splitType == SplitType::Mix)
        {
            mixTo(value);
        }
        break;
    case PANL_CTL:
        panL = value;
        if (splitType == SplitType::Lr)
        {
            mixTo(panL, volL, panR, volR);
        }
        break;
    case VOLL_CTL:
        volL = value;
        if (splitType == SplitType::Lr)
        {
            mixTo(panL, volL, panR, volR);
        }
        break;
    case PANR_CTL:
        panR = value;
        if (splitType == SplitType::Lr)
        {
            mixTo(panL, volL, panR, volR);
        }
        break;
    case VOLR_CTL:
        volR = value;
        if (splitType == SplitType::Lr)
        {
            mixTo(panL, volL, panR, volR);
        }
        break;
    }
}
