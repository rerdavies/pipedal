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