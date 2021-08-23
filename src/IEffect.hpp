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

#pragma once 

#include <lv2/urid/urid.h>

namespace pipedal {
    class RealtimeParameterRequest;

    class IEffect {
    public:
        virtual ~IEffect() {}
        virtual long GetInstanceId() const = 0;
        virtual int GetControlIndex(const std::string&symbol) const = 0;
        virtual void SetControl(int index, float value)  = 0;
        virtual float GetControlValue(int index) const = 0;
        virtual void SetBypass(bool enable)  = 0;
        virtual float GetOutputControlValue(int controlIndex) const = 0;


        virtual int GetNumberOfInputAudioPorts() const = 0;
        virtual int GetNumberOfOutputAudioPorts() const = 0;
        virtual float *GetAudioInputBuffer(int index) const = 0;
        virtual float *GetAudioOutputBuffer(int index) const = 0;
        virtual void ResetAtomBuffers() = 0;
        virtual void RequestParameter(LV2_URID uridUri) = 0;
        virtual void GatherParameter(RealtimeParameterRequest*pRequest) = 0;

        virtual std::string AtomToJson(uint8_t*pAtom) = 0;


        virtual void SetAudioOutputBuffer(int index, float*buffer) = 0;

        virtual void Activate() = 0;
        virtual void Deactivate() = 0;
    };
} //namespace