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

#pragma once

#include "json.hpp"

namespace pipedal
{
    class VuUpdate
    {
    public:
        uint64_t instanceId_ = 0;
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