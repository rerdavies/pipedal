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

#include <lv2/urid/urid.h>
#include <lv2/atom/atom.h>


namespace pipedal {
    class RealtimePatchPropertyRequest;
    class RealtimeRingBufferWriter;
    class Lv2PluginState;

    class IEffect {
    public:
        virtual ~IEffect() {}
        virtual uint64_t GetInstanceId() const = 0;
        virtual bool IsLv2Effect() const = 0;
        virtual uint64_t GetMaxInputControl() const = 0;
        virtual bool IsInputControl(uint64_t index) const = 0;
        virtual float GetDefaultInputControlValue(uint64_t index) const = 0;

        virtual int GetControlIndex(const std::string&symbol) const = 0;
        virtual void SetControl(int index, float value)  = 0;

        virtual void SetPatchProperty(LV2_URID uridUri, size_t size, LV2_Atom *value) = 0;
        virtual void RequestPatchProperty(LV2_URID uridUri) = 0;
        virtual void RequestAllPathPatchProperties() = 0;


        virtual float GetControlValue(int index) const = 0;
        virtual void SetBypass(bool enable)  = 0;
        virtual float GetOutputControlValue(int controlIndex) const = 0;

        virtual int GetNumberOfInputAudioPorts() const = 0; // as declared
        virtual int GetNumberOfOutputAudioPorts() const = 0; // as declared.

        virtual int GetNumberOfInputAudioBuffers() const = 0; // may be different if plugin has zero inputs.
        virtual int GetNumberOfOutputAudioBuffers() const = 0; // may be different if plugin has zero inputs.
        virtual float *GetAudioInputBuffer(int index) const = 0;
        virtual float *GetAudioOutputBuffer(int index) const = 0;
        virtual void ResetAtomBuffers() = 0;

        virtual bool GetRequestStateChangedNotification() const = 0;
        virtual void SetRequestStateChangedNotification(bool value) = 0;

        virtual void PrepareNoInputEffect(int numberOfInputs, size_t maxBufferSize) = 0;

        virtual void SetAudioInputBuffer(int index, float *buffer) = 0;
        virtual void SetAudioOutputBuffer(int index, float*buffer) = 0;

        virtual void Activate() = 0;

        virtual void Run(uint32_t samples, RealtimeRingBufferWriter *realtimeRingBufferWriter) = 0;

        virtual void Deactivate() = 0;

        virtual bool IsVst3() const = 0;
        virtual bool GetLv2State(Lv2PluginState*state) = 0;
        virtual void SetLv2State(Lv2PluginState&state) = 0;
        
        virtual bool HasErrorMessage() const = 0;
        virtual const char*TakeErrorMessage()  = 0;
    };
} //namespace