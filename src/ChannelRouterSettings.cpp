// Copyright (c) 2026 Robin Davies
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


#include "ChannelRouterSettings.hpp"
#include <stdexcept>

using namespace pipedal;

static uint64_t countChannels(const std::vector<int64_t> &channels)
{
    if (channels.size() != 2)
    {
        throw std::runtime_error("Invalid Channel Router channel configuration.");
    }
    if (channels[0] == -1)
    {
        if (channels[1] == -1)
        {
            return 0;
        }
        else
        {
            return 1;
        }
    }
    else
    {
        if (channels[1] == -1)
        {
            return 1;
        }
        if (channels[0] == channels[1])
        {
            return 1;
        }
        return 2;
    }
}   

uint64_t ChannelRouterSettings::numberOfAudioInputChannels() const
{
    if (!configured_)
    {
        return 2;
    }
    return countChannels(mainInputChannels_);
}
uint64_t ChannelRouterSettings::numberOfAudioOutputChannels() const
{
    if (!configured_)
    {
        return 2;
    }
    return countChannels(mainOutputChannels_);
}

ChannelSelection::ChannelSelection(ChannelRouterSettings&settings)
: mainInputChannels_(settings.mainInputChannels())
, mainOutputChannels_(settings.mainOutputChannels())
, auxInputChannels_(settings.auxInputChannels())
, auxOutputChannels_(settings.auxOutputChannels())
, sendInputChannels_(settings.sendInputChannels())
, sendOutputChannels_(settings.sendOutputChannels())
{
    normalizeChannelSelection(); 
}

static void normalizeInputChannels(std::vector<int64_t>&channels) {
    if (channels.size() == 2) {
        if (channels[0] == -1 && channels[1] == -1) 
        {
            channels.resize(0);
        } else if (channels[0] == channels[1]) {
            channels.resize(1);
        } else if (channels[1] == -1) {
            channels.resize(1);} {
        }
    }
}

static void normalizeOutputChannels(std::vector<int64_t>&channels) {
    normalizeInputChannels(channels);
}

void ChannelSelection::normalizeChannelSelection() {
    normalizeInputChannels(mainInputChannels_);
    normalizeOutputChannels(mainOutputChannels_);
    normalizeInputChannels(auxInputChannels_);
    normalizeOutputChannels(auxOutputChannels_);
    normalizeInputChannels(sendInputChannels_);
    normalizeOutputChannels(sendOutputChannels_);

    // If either aux inputs or outputs are zero, don't do ANY aux processing.
    if (auxInputChannels_.size() == 0) 
    {
        auxOutputChannels_.resize(0);
    } else if (auxOutputChannels_.size() == 0) {
        auxInputChannels_.resize(0);
    }
    // If either main inputs or outputs are empty, add send/receive dummy buffers. (Lv2 plugins shouldn't have to deal with this)
    if (mainInputChannels_.size() == 0) 
    {
        mainInputChannels_.resize(1);
        mainInputChannels_[0] = -1;
    }
    if (mainOutputChannels_.size() == 0) {
        mainInputChannels_.resize(1);
        mainOutputChannels_[0] = -1;
    }
    // Send buffers are what they are. Let the send plugin deal with it. 
}


ChannelRouterSettings::ChannelRouterSettings()
: mainInserts_(Pedalboard::InstanceType::MainInsert)
, auxInserts_(Pedalboard::InstanceType::AuxInsert)
{
    
}


JSON_MAP_BEGIN(ChannelRouterSettings)
JSON_MAP_REFERENCE(ChannelRouterSettings, configured)
JSON_MAP_REFERENCE(ChannelRouterSettings, channelRouterPresetId)
JSON_MAP_REFERENCE(ChannelRouterSettings, mainInputChannels)
JSON_MAP_REFERENCE(ChannelRouterSettings, mainOutputChannels)
JSON_MAP_REFERENCE(ChannelRouterSettings, mainInserts)
JSON_MAP_REFERENCE(ChannelRouterSettings, auxInputChannels)
JSON_MAP_REFERENCE(ChannelRouterSettings, auxOutputChannels)
JSON_MAP_REFERENCE(ChannelRouterSettings, auxInserts)
JSON_MAP_REFERENCE(ChannelRouterSettings, sendInputChannels)
JSON_MAP_REFERENCE(ChannelRouterSettings, sendOutputChannels)   
JSON_MAP_REFERENCE(ChannelRouterSettings, controlValues)
JSON_MAP_END()
