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

#include "IEffect.hpp"
#include "PiPedalException.hpp"
#include "PiPedalMath.hpp"
#include <assert.h>

namespace pipedal
{

    enum class SplitType
    {
        Ab = 0,
        Mix = 1,
        Lr = 2
    };
    class Lv2PluginInfo;
    class Lv2PortInfo;

    const Lv2PluginInfo *GetSplitterPluginInfo();

    const Lv2PortInfo *GetBypassPortInfo();


    const double SPLIT_DB_MIN = -60;

    inline SplitType valueToSplitType(float value)
    {
        switch ((int)value)
        {
        case 0:
            return SplitType::Ab;
        case 1:
            return SplitType::Mix;
        case 2:
            return SplitType::Lr;
        default:
            return SplitType::Ab;
        }
    };

    class SplitEffect : public IEffect
    {
    private:


        virtual void SetAudioInputBuffer(int index, float *buffer) 
        {
            assert(false); // not used.
        }
        virtual void Run(uint32_t samples, RealtimeRingBufferWriter *realtimeRingBufferWriter) 
        {
            assert(false); // not used.
        }


        const double MIX_TRANSITION_TIME_S = 0.1;
        double sampleRate;
        std::vector<float *> inputs;
        std::vector<float *> topInputs;
        std::vector<float *> bottomInputs;
        std::vector<float *> topOutputs;
        std::vector<float *> bottomOutputs;

        std::vector<float *> mixTopInputs;
        std::vector<float *> mixBottomInputs;

        std::vector<float *> outputBuffers;
        int numberOfOutputPorts;

        SplitType splitType = SplitType::Ab;
        float mix = 0;
        float panL = 0;
        float volL = -3;
        float panR = 0;
        float volR = -3;

        float currentMix = 0;

        float targetBlendLTop = 0.5;
        float targetBlendRTop = 0.5;
        float targetBlendLBottom = 0.5;
        float targetBlendRBottom = 0.5;

        float blendLTop = 0.5;
        float blendRTop = 0.5;
        float blendLBottom = 0.5;
        float blendRBottom = 0.5;

        float blendDxLTop = 0;
        float blendDxRTop = 0;
        float blendDxLBottom = 0;
        float blendDxRBottom = 0;

        int32_t blendFadeSamples;

        bool selectA = true;

        bool activated = false;

        using MixFunction = void (SplitEffect::*)(uint32_t frames);

        MixFunction preAbTop;
        MixFunction preAbBottom;

        void Copy(float *input, float *output, uint32_t frames)
        {
            for (int i = 0; i < frames; ++i)
            {
                output[i] = input[i];
            }
        }

        void abTopMonoMono(uint32_t frames)
        {
            Copy(this->inputs[0], this->topInputs[0], frames);
        }
        void abTopMonoStereo(uint32_t frames)
        {
            Copy(this->inputs[0], this->topInputs[0], frames);
            Copy(this->inputs[0], this->topInputs[1], frames);
        }
        void abTopStereoStereo(uint32_t frames)
        {
            Copy(this->inputs[0], this->topInputs[0], frames);
            Copy(this->inputs[1], this->topInputs[1], frames);
        }

        void abBottomMonoMono(uint32_t frames)
        {
            Copy(this->inputs[0], this->bottomInputs[0], frames);
        }
        void abBottomMonoStereo(uint32_t frames)
        {
            Copy(this->inputs[0], this->bottomInputs[0], frames);
            Copy(this->inputs[0], this->bottomInputs[1], frames);
        }
        void abBottomStereoStereo(uint32_t frames)
        {
            Copy(this->inputs[0], this->bottomInputs[0], frames);
            Copy(this->inputs[1], this->bottomInputs[1], frames);
        }
        void lrTopMonoMono(uint32_t frames)
        {
            Copy(this->inputs[0], this->topInputs[0], frames);
        }
        void lrTopMonoStereo(uint32_t frames)
        {
            Copy(this->inputs[0], this->topInputs[0], frames);
            Copy(this->inputs[0], this->topInputs[1], frames);
        }
        void lrTopStereoStereo(uint32_t frames)
        {
            Copy(this->inputs[0], this->topInputs[0], frames);
            Copy(this->inputs[0], this->topInputs[1], frames);
        }

        void lrBottomMonoMono(uint32_t frames)
        {
            Copy(this->inputs[0], this->bottomInputs[0], frames);
        }
        void lrBottomMonoStereo(uint32_t frames)
        {
            Copy(this->inputs[0], this->bottomInputs[0], frames);
            Copy(this->inputs[0], this->bottomInputs[1], frames);
        }
        void lrBottomStereoStereo(uint32_t frames)
        {
            Copy(this->inputs[1], this->bottomInputs[0], frames);
            Copy(this->inputs[1], this->bottomInputs[1], frames);
        }

        void updateMixFunction()
        {
            if (activated)
            {

                // Input Mix Functions.
                if (splitType != SplitType::Lr)
                {
                    if (this->inputs.size() == 1)
                    {
                        if (this->topInputs.size() == 1)
                            this->preAbTop = &SplitEffect::abTopMonoMono;
                        else
                        {
                            this->preAbTop = &SplitEffect::abTopMonoStereo;
                        }
                        if (this->bottomInputs.size() == 1)
                        {
                            this->preAbBottom = &SplitEffect::abBottomMonoMono;
                        }
                        else
                        {
                            this->preAbBottom = &SplitEffect::abBottomMonoStereo;
                        }
                    }
                    else
                    {
                        if (this->topInputs.size() == 1)
                        {
                            this->preAbTop == &SplitEffect::abTopMonoMono;
                        }
                        else
                        {
                            this->preAbTop = &SplitEffect::abTopStereoStereo;
                        }
                        if (this->bottomInputs.size() == 1)
                        {
                            this->preAbBottom == &SplitEffect::abBottomMonoMono;
                        }
                        else
                        {
                            this->preAbBottom = &SplitEffect::abBottomStereoStereo;
                        }
                    }
                }
                else
                {
                    if (this->inputs.size() == 1)
                    {
                        if (this->topInputs.size() == 1)
                            this->preAbTop = &SplitEffect::lrTopMonoMono;
                        else
                        {
                            this->preAbTop = &SplitEffect::lrTopMonoStereo;
                        }
                        if (this->bottomInputs.size() == 1)
                        {
                            this->preAbBottom = &SplitEffect::lrBottomMonoMono;
                        }
                        else
                        {
                            this->preAbBottom = &SplitEffect::lrBottomMonoStereo;
                        }
                    }
                    else
                    {
                        if (this->topInputs.size() == 1)
                        {
                            this->preAbTop == &SplitEffect::lrTopMonoMono;
                        }
                        else
                        {
                            this->preAbTop = &SplitEffect::lrTopStereoStereo;
                        }
                        if (this->bottomInputs.size() == 1)
                        {
                            this->preAbBottom == &SplitEffect::lrBottomMonoMono;
                        }
                        else
                        {
                            this->preAbBottom = &SplitEffect::lrBottomStereoStereo;
                        }
                    }
                }
                if (splitType == SplitType::Ab)
                {
                    mixTo(selectA ? -1 : 1);
                }
                else if (splitType == SplitType::Mix)
                {
                    mixTo(mix);
                }
                else
                {
                    mixTo(panL, volL, panR, volR);
                }
            }
        }
        uint64_t instanceId;

        virtual uint8_t *GetAtomInputBuffer() { return nullptr; }
        virtual uint8_t *GetAtomOutputBuffer() { return nullptr; }
        virtual void RequestPatchProperty(LV2_URID uridUri) {}
        virtual void GatherPatchProperties(RealtimePatchPropertyRequest *pRequest) {}
        virtual std::string AtomToJson(uint8_t *pAtom) { return ""; }
        virtual std::string GetAtomObjectType(uint8_t*pData) { return "not implemented";}
        virtual bool GetLv2State(Lv2PluginState*state) { return false; }
        virtual bool HasErrorMessage() const { return false; }
        const char* TakeErrorMessage() { return ""; }
        virtual bool IsVst3() const { return false; }

    public:
        SplitEffect(
            uint64_t instanceId,
            double sampleRate,
            const std::vector<float *> &inputs)
            : instanceId(instanceId), inputs(inputs), sampleRate(sampleRate)

        {
        }
        ~SplitEffect()
        {
        }


        virtual uint64_t GetInstanceId() const
        {
            return instanceId;
        }
        void Activate()
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
        void Deactivate()
        {
            activated = false;
        }

        void SetChainBuffers(
            const std::vector<float *> &topInputs,
            const std::vector<float *> &bottomInputs,
            const std::vector<float *> &topOutputs,
            const std::vector<float *> &bottomOutputs)
        {
            this->topInputs = topInputs;
            this->bottomInputs = bottomInputs;
            this->topOutputs = topOutputs;
            this->bottomOutputs = bottomOutputs;

            numberOfOutputPorts = topOutputs.size() > 1 || bottomOutputs.size() > 1 ? 2 : 1;
            outputBuffers.resize(numberOfOutputPorts);
        }
        virtual void ResetAtomBuffers() {}
        virtual int GetControlIndex(const std::string &symbol) const
        {
            if (symbol == "splitType")
                return 0;
            if (symbol == "select")
                return 1;
            if (symbol == "mix")
                return 2;
            return -1;
        }

        void snapToMixTarget()
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
        void mixToTarget()
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

        void mixTo(float value)
        {
            float blend = (value + 1) * 0.5f;
            this->targetBlendRTop = this->targetBlendLTop = 1 - blend;
            this->targetBlendLBottom = this->targetBlendLBottom = blend;
            mixToTarget();
        }
        void mixTo(float panL, float volL, float panR, float volR)
        {
            float aTop = (volL <= SPLIT_DB_MIN) ? 0 : db2a(volL);
            float aBottom = (volL <= SPLIT_DB_MIN) ? 0 : db2a(volR);
            if (this->outputBuffers.size() == 1)
            {
                // ignore pan. The R values actually have no effect.
                this->targetBlendLTop = this->targetBlendRTop = aTop;
                this->targetBlendLBottom = this->targetBlendRBottom = aBottom;
            }
            else
            {
                float blendTop = (panL + 1) * 0.5;
                float blendBottom = (panR + 1) * 0.5;
                this->targetBlendLTop = (1 - blendTop) * aTop;
                this->targetBlendRTop = (blendTop)*aTop;
                this->targetBlendLBottom = (1 - blendBottom) * aBottom;
                this->targetBlendRBottom = (blendBottom)*aBottom;
            }
            mixToTarget();
        }
        virtual void SetBypass(bool enabled)
        {
            throw PiPedalArgumentException("Not implmented. Should not have been called.");
        }

        virtual float GetOutputControlValue(int index) const {
            return GetControlValue(index);
        }

        virtual float GetControlValue(int portIndex) const
        {
            switch (portIndex)
            {
            case -1:  /* (bypass) */
                return 1;
            case 0:
            {
                return (int)(this->splitType);
            }
            case 1:
            {
                return selectA ? 0 : 1;
            }
            case 2:
                return mix;
            case 3:
                return this->panL;
            case 4:
                return this->volL;
            case 5:
                return this->panR;
            case 6:
                return this->volR;
            default:
                throw PiPedalArgumentException("Invalid argument");
            }
        }
        virtual void SetControl(int index, float value)
        {
            switch (index)
            {
            case -1: /* (bypass) */
                return; // no can bypass.
            case 0:
            {
                SplitType t = valueToSplitType(value);
                if (splitType != t)
                {
                    splitType = t;
                    updateMixFunction();
                }
                break;
            }
            case 1:
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
            case 2:
                mix = value;
                if (splitType == SplitType::Mix)
                {
                    mixTo(value);
                }
                break;
            case 3:
                panL = value;
                if (splitType == SplitType::Lr)
                {
                    mixTo(panL, volL, panR, volR);
                }
                break;
            case 4:
                volL = value;
                if (splitType == SplitType::Lr)
                {
                    mixTo(panL, volL, panR, volR);
                }
                break;
            case 5:
                panR = value;
                if (splitType == SplitType::Lr)
                {
                    mixTo(panL, volL, panR, volR);
                }
                break;
            case 6:
                volR = value;
                if (splitType == SplitType::Lr)
                {
                    mixTo(panL, volL, panR, volR);
                }
                break;
            }
        }

        virtual int GetNumberOfOutputAudioPorts() const
        {
            return numberOfOutputPorts;
        }
        virtual int GetNumberOfInputAudioPorts() const
        {
            return (int)this->inputs.size();
        }
        virtual float *GetAudioInputBuffer(int index) const
        {
            return inputs[index];
        }
        virtual float *GetAudioOutputBuffer(int index) const
        {
            return this->outputBuffers[index];
        }

        virtual void SetAudioOutputBuffer(int index, float *buffer)
        {
            outputBuffers[index] = buffer;
        }
        void PreMix(uint32_t frames)
        {
            (this->*preAbTop)(frames);
            (this->*preAbBottom)(frames);
        }

        void PostMixMono(uint32_t frames)
        {
            uint32_t ix = 0;

            float *top = this->mixTopInputs[0];
            float *bottom = this->mixBottomInputs[0];
            float *output = this->outputBuffers[0];

            while (frames)
            {
                if (this->blendFadeSamples != 0)
                {
                    uint32_t framesThisTime = this->blendFadeSamples < frames ? this->blendFadeSamples : frames;
                    for (uint32_t i = 0; i < framesThisTime; ++i)
                    {
                        output[ix] = blendLBottom * bottom[ix] + blendLTop * top[ix];
                        ++ix;

                        this->blendLTop += this->blendDxLTop;
                        this->blendLBottom += this->blendDxLBottom;
                    }
                    this->blendFadeSamples -= framesThisTime;
                    frames -= framesThisTime;
                    if (blendFadeSamples == 0)
                    {
                        this->blendLTop = this->targetBlendLTop;
                        this->blendLBottom = this->targetBlendLBottom;
                        this->blendDxLTop = this->blendDxLBottom = 0;
                    }
                }
                else
                {
                    float blendTop = this->blendLTop;
                    float blendBottom = this->blendLBottom;
                    while (frames--)
                    {
                        output[ix] = blendBottom * bottom[ix] + blendTop * top[ix];
                        ++ix;
                    }
                    return;
                }
            }
        }

        void PostMixStereo(uint32_t frames)
        {
            uint32_t ix = 0;

            float *top = this->mixTopInputs[0];
            float *bottom = this->mixBottomInputs[0];
            float *output = this->outputBuffers[0];

            float *topR = this->mixTopInputs[1];
            float *bottomR = this->mixBottomInputs[1];
            float *outputR = this->outputBuffers[1];

            while (frames != 0)
            {
                if (this->blendFadeSamples != 0)
                {
                    uint32_t framesThisTime = this->blendFadeSamples < frames ? this->blendFadeSamples : frames;
                    for (int i = 0; i < framesThisTime; ++i)
                    {
                        output[ix] = blendLBottom * bottom[ix] + blendLTop * top[ix];
                        outputR[ix] = blendRBottom * bottomR[ix] + blendRTop * topR[ix];
                        ++ix;

                        this->blendLTop += this->blendDxLTop;
                        this->blendRTop += this->blendDxRTop;
                        this->blendLBottom += this->blendDxLBottom;
                        this->blendRBottom += this->blendDxRBottom;
                    }
                    blendFadeSamples -= framesThisTime;
                    frames -= framesThisTime;
                    if (blendFadeSamples == 0)
                    {
                        this->blendLTop = this->targetBlendLTop;
                        this->blendRTop = this->targetBlendRTop;
                        this->blendLBottom = this->targetBlendLBottom;
                        this->blendRBottom = this->targetBlendRBottom;
                        this->blendDxLTop = this->blendDxRTop = this->blendDxLBottom = this->blendDxRBottom = 0;
                    }
                }
                else
                {
                    float blendLTop = this->blendLTop;
                    float blendRTop = this->blendRTop;
                    float blendLBottom = this->blendLBottom;
                    float blendRBottom = this->blendRBottom;

                    for (int i = 0; i < frames; ++i)
                    {
                        output[ix] = blendLBottom * bottom[ix] + blendLTop * top[ix];
                        outputR[ix] = blendRBottom * bottomR[ix] + blendRTop * topR[ix];
                        ++ix;
                    }
                    return;
                }
            }
        }
        void PostMix(uint32_t frames)
        {
            if (this->outputBuffers.size() == 1)
            {
                PostMixMono(frames);
            }
            else
            {
                PostMixStereo(frames);
            }
        }
    };

} // namespace