// Copyright (c) 2021 Robin Davies
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
#include "OptionsFeature.hpp"
#include "lv2/atom.lv2/atom.h"
#include "lv2/atom.lv2/util.h"
#include "lv2/log.lv2/log.h"
#include "lv2/log.lv2/logger.h"
#include "lv2/midi.lv2/midi.h"
#include "lv2/urid.lv2/urid.h"
#include "lv2/log.lv2/logger.h"
#include "lv2/uri-map.lv2/uri-map.h"
#include "lv2/atom.lv2/forge.h"
#include "lv2/worker.lv2/worker.h"
#include "lv2/patch.lv2/patch.h"
#include "lv2/parameters.lv2/parameters.h"
#include "lv2/units.lv2/units.h"
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>

#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
using namespace pipedal;

OptionsFeature::OptionsFeature()
{
    feature.URI = LV2_OPTIONS__options;
    feature.data = options;

}

OptionsFeature::~OptionsFeature()
{

}
void OptionsFeature::Prepare(MapFeature&map,double sampleRate, int32_t blockLength, int32_t atomBufferBlockLength)
{
    this->sampleRate = (float)sampleRate;
    this->blockLength = blockLength;
    this->atomBufferBlockLength = atomBufferBlockLength;


    options[0].context = LV2_OPTIONS_INSTANCE;
    options[0].subject = 0;
    options[0].key =  map.GetUrid(LV2_PARAMETERS__sampleRate);
    options[0].size = sizeof(float);
    options[0].type = map.GetUrid(LV2_ATOM__Float);
    options[0].value = &(this->sampleRate);

    options[1].context = LV2_OPTIONS_INSTANCE;
    options[1].subject = 0;
    options[1].key = map.GetUrid(LV2_BUF_SIZE__minBlockLength);
    options[1].size = sizeof(int32_t);
    options[1].type = map.GetUrid(LV2_ATOM__Int);
    options[1].value = &(this->blockLength);

    options[2].context = LV2_OPTIONS_INSTANCE;
    options[2].subject = 0;
    options[2].key = map.GetUrid(LV2_BUF_SIZE__maxBlockLength);
    options[2].size = sizeof(int32_t);
    options[2].type = map.GetUrid(LV2_ATOM__Int);
    options[2].value =  &(this->blockLength);

    options[3].context = LV2_OPTIONS_INSTANCE;
    options[3].subject = 0;
    options[3].key = map.GetUrid(LV2_BUF_SIZE__nominalBlockLength);
    options[3].size = sizeof(int32_t);
    options[3].type = map.GetUrid(LV2_ATOM__Int);
    options[3].value =  &(this->blockLength);

    options[4].context = LV2_OPTIONS_INSTANCE;
    options[4].subject = 0;
    options[4].key = map.GetUrid(LV2_BUF_SIZE__sequenceSize);
    options[4].size = sizeof(int32_t);
    options[4].type = map.GetUrid(LV2_ATOM__Int);
    options[4].value =  &(this->atomBufferBlockLength);;

    options[5].context = LV2_OPTIONS_INSTANCE;
    options[5].subject = 0;
    options[5].key = 0;
    options[5].size = 0;
    options[5].type = 0;
    options[5].value = NULL;
    
}
