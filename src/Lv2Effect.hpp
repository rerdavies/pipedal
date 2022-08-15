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

#include "PiPedalHost.hpp"
#include "PedalBoard.hpp"
#include <lilv/lilv.h>
#include "BufferPool.hpp"

#include "IEffect.hpp"
#include "Worker.hpp"
#include "lv2/patch.lv2/patch.h"
#include "lv2/log.lv2/log.h"
#include "lv2/log.lv2/logger.h"
#include "lv2/lv2plug.in/ns/extensions/units/units.h"
#include "lv2/atom.lv2/forge.h"


namespace pipedal
{

    class RealtimeRingBufferWriter;    

    class Lv2Effect : public IEffect
    {
    private:
        IHost *pHost = nullptr;
        int numberOfInputs = 0;
        int numberOfOutputs = 0;
        int numberOfMidiInputs = 0;

        std::unique_ptr<Worker> worker;
        LilvInstance *pInstance;
        std::shared_ptr<Lv2PluginInfo> info;
        std::vector<float> controlValues;
        std::vector<int> inputAudioPortIndices;
        std::vector<int> outputAudioPortIndices;

        std::vector<int> inputAtomPortIndices;
        std::vector<int> outputAtomPortIndices;

        std::vector<int> inputMidiPortIndices;
        std::vector<int> outputMidiPortIndices;

        std::vector<int> midiInputIndices;

        std::vector<float *> inputAudioBuffers;
        std::vector<float *> outputAudioBuffers;

        std::vector<char *> inputAtomBuffers;
        std::vector<char *> outputAtomBuffers;
        std::vector<LV2_Feature *> features;
        LV2_Feature *work_schedule_feature = nullptr;

        virtual std::string GetUri() const { return info->uri(); }

        std::vector<const Lv2PortInfo *> realtimePortInfo;
        void PreparePortIndices();
        void ConnectControlPorts();
        void AssignUnconnectedPorts();

        LV2_Atom_Forge inputForgeRt;
        LV2_Atom_Forge_Frame input_frame;

        LV2_Atom_Forge outputForgeRt;


        class Uris
        {
        public:
            Uris(IHost *pHost)
            {
                atom_Blank = pHost->GetLv2Urid(LV2_ATOM__Blank);
                atom_Path = pHost->GetLv2Urid(LV2_ATOM__Path);
                atom_float = pHost->GetLv2Urid(LV2_ATOM__Float);
                atom_Double = pHost->GetLv2Urid(LV2_ATOM__Double);
                atom_Int = pHost->GetLv2Urid(LV2_ATOM__Int);
                atom_Long = pHost->GetLv2Urid(LV2_ATOM__Long);
                atom_Bool = pHost->GetLv2Urid(LV2_ATOM__Bool);
                atom_String = pHost->GetLv2Urid(LV2_ATOM__String);
                atom_Vector = pHost->GetLv2Urid(LV2_ATOM__Vector);
                atom_Object = pHost->GetLv2Urid(LV2_ATOM__Object);


                atom_Sequence = pHost->GetLv2Urid(LV2_ATOM__Sequence);
                atom_Chunk = pHost->GetLv2Urid(LV2_ATOM__Chunk);
                atom_URID = pHost->GetLv2Urid(LV2_ATOM__URID);
                atom_eventTransfer = pHost->GetLv2Urid(LV2_ATOM__eventTransfer);
                patch_Get = pHost->GetLv2Urid(LV2_PATCH__Get);
                patch_Set = pHost->GetLv2Urid(LV2_PATCH__Set);
                patch_Put = pHost->GetLv2Urid(LV2_PATCH__Put);
                patch_body = pHost->GetLv2Urid(LV2_PATCH__body);
                patch_subject = pHost->GetLv2Urid(LV2_PATCH__subject);
                patch_property = pHost->GetLv2Urid(LV2_PATCH__property);
                //patch_accept = pHost->GetLv2Urid(LV2_PATCH__accept);
                patch_value = pHost->GetLv2Urid(LV2_PATCH__value);
                unitsFrame = pHost->GetLv2Urid(LV2_UNITS__frame);

            }
            //LV2_URID patch_accept;

            LV2_URID unitsFrame;
            LV2_URID pluginUri;
            LV2_URID atom_Blank;
            LV2_URID atom_Bool;
            LV2_URID atom_float;
            LV2_URID atom_Double;
            LV2_URID atom_Int;
            LV2_URID atom_Long;
            LV2_URID atom_String;
            LV2_URID atom_Object;
            LV2_URID atom_Vector;
            LV2_URID atom_Path;
            LV2_URID atom_Sequence;
            LV2_URID atom_Chunk;
            LV2_URID atom_URID;
            LV2_URID atom_eventTransfer;
            LV2_URID midi_Event;
            LV2_URID patch_Get;
            LV2_URID patch_Set;
            LV2_URID patch_Put;
            LV2_URID patch_body;
            LV2_URID patch_subject;
            LV2_URID patch_property;
            LV2_URID patch_value;
            LV2_URID param_uiState;
        };

        Uris uris;

        uint64_t instanceId;
        BufferPool bufferPool;

        static LV2_Worker_Status worker_schedule_fn(LV2_Worker_Schedule_Handle handle,
                                                    uint32_t size,
                                                    const void *data);

        void ResetInputAtomBuffer(char*data);
        void ResetOutputAtomBuffer(char*data);

        uint32_t bypassStartingSamples = 0;

        bool bypass = true;
        double targetBypass = 0;
        double currentBypass = 0;
        double currentBypassDx = 0;
        uint32_t bypassSamplesRemaining = 0;

        void BypassTo(float value);


        virtual void RequestParameter(LV2_URID uridUri);
        virtual void GatherParameter(RealtimeParameterRequest*pRequest);
        virtual bool IsVst3() const { return false; }
        virtual void RelayOutputMessages(uint64_t instanceId,RealtimeRingBufferWriter *realtimeRingBufferWriter);

        virtual uint8_t*GetAtomInputBuffer() {
            if (this->inputAtomBuffers.size() == 0) return nullptr;
            return (uint8_t*)this->inputAtomBuffers[0];
        }
        virtual uint8_t*GetAtomOutputBuffer()
        {
            if (this->outputAtomBuffers.size() == 0) return nullptr;
            return (uint8_t*)this->outputAtomBuffers[0];

        }


    public:
        Lv2Effect(
            IHost *pHost,
            const std::shared_ptr<Lv2PluginInfo> &info,
            const PedalBoardItem &pedalBoardItem);
        ~Lv2Effect();



        virtual void ResetAtomBuffers();
        virtual uint64_t GetInstanceId() const { return instanceId; }
        virtual int GetNumberOfInputAudioPorts() const { return inputAudioPortIndices.size(); }
        virtual int GetNumberOfOutputAudioPorts() const { return outputAudioPortIndices.size(); }
        virtual int GetNumberOfInputAtomPorts() const { return inputAtomPortIndices.size(); }
        virtual int GetNumberOfOutputAtomPorts() const { return outputAtomPortIndices.size(); }
        virtual int GetNumberOfMidiInputPorts() const { return inputMidiPortIndices.size(); }
        virtual int GetNumberOfMidiOutputPorts() const { return outputMidiPortIndices.size(); }

        virtual void SetAudioInputBuffer(int index, float *buffer);
        virtual float *GetAudioInputBuffer(int index) const { return this->inputAudioBuffers[index]; }

        virtual void SetAudioInputBuffer(float *buffer);
        virtual void SetAudioInputBuffers(float *left, float *right);

        virtual void SetAudioOutputBuffer(int index, float *buffer);
        virtual float *GetAudioOutputBuffer(int index) const { return this->outputAudioBuffers[index]; }

        virtual void SetAtomInputBuffer(int index, void *buffer) { this->inputAtomBuffers[index] = (char*)buffer;}
        virtual void *GetAtomInputBuffer(int index) const { return this->inputAtomBuffers[index]; }

        virtual void SetAtomOutputBuffer(int index, void *buffer) { this->outputAtomBuffers[index] = (char*)buffer; }
        virtual void *GetAtomOutputBuffer(int index) const { return this->outputAtomBuffers[index]; }

        virtual int GetControlIndex(const std::string &symbol) const;

        virtual void SetControl(int index, float value)
        {
            if (index == -1)
            {
                this->bypass = value != 0;
            } else {
                controlValues[index] = value;
            }
        }
        virtual float GetControlValue(int index) const {
            if (index == -1) 
            {
                return this->bypass? 1: 0;
            }
            return controlValues[index];
        }


        virtual float GetOutputControlValue(int portIndex) const
        {
            return controlValues[portIndex];
        }

        virtual void SetBypass(bool bypass)
        {
            if (bypass != this->bypass)
            {
                this->bypass = bypass;
                BypassTo(bypass? 1.0f: 0.0f);
            }

        }

        virtual void Activate();
        virtual void Run(uint32_t samples, RealtimeRingBufferWriter *realtimeRingBufferWriter);
        virtual void Deactivate();
    };

} // namespace