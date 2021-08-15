#pragma once

#include "json.hpp"

namespace pipedal
{
    class VuUpdate
    {
    public:
        long instanceId_ = 0;
        long sampleTime_ = 0;
        bool isStereoInput_ = false;
        bool isStereoOutput_ = false;
        float inputMaxValueL_ = 0;
        float inputMaxValueR_ = 0;
        float outputMaxValueL_ = 0;
        float outputMaxValueR_ = 0;

    public:
        void reset() {
            inputMaxValueL_ = 0;
            inputMaxValueR_ = 0;
            outputMaxValueL_ = 0;
            outputMaxValueR_ = 0;
        }
        
        void AccumulateVu(float *value,float *input, uint32_t samples)
        {
                        
            float v = *value;
            for (uint32_t i = 0; i < samples; ++i)
            {
                float t = std::abs(input[i]);
                if (t > v) {
                    v = t;
                }
            }
            *value = v;
        }
        void AccumulateInputs(float* input, uint32_t samples)
        {
            AccumulateVu(&inputMaxValueL_,input,samples);

        }
        void AccumulateInputs(float* inputL, float*inputR, uint32_t samples)
        {
            AccumulateVu(&inputMaxValueL_,inputL,samples);
            AccumulateVu(&inputMaxValueR_,inputR,samples);

        }
        void AccumulateOutputs(float* output, uint32_t samples)
        {
            AccumulateVu(&outputMaxValueL_,output,samples);

        }
        void AccumulateOutputs(float* outputL, float*outputR, uint32_t samples)
        {
            AccumulateVu(&outputMaxValueL_,outputL,samples);
            AccumulateVu(&outputMaxValueR_,outputR,samples);

        }

        DECLARE_JSON_MAP(VuUpdate);
    };
}