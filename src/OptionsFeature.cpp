#include "pch.h"
#include "OptionsFeature.hpp"

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