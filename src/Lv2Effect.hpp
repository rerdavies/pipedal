// Copyright (c) 2025 Robin Davies
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

#include "PluginHost.hpp"
#include "Pedalboard.hpp"
#include <lilv/lilv.h>
#include "BufferPool.hpp"
#include "FileBrowserFilesFeature.hpp"
#include "PatchPropertyWriter.hpp"
#include <unordered_map>
#include "MapPathFeature.hpp"
#include "OptionsFeature.hpp"

#include "IEffect.hpp"
#include "Worker.hpp"
#include "lv2/patch/patch.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/lv2plug.in/ns/extensions/units/units.h"
#include "lv2/atom/forge.h"
#include "AtomBuffer.hpp"
#include "StateInterface.hpp"
#include "LogFeature.hpp"

namespace pipedal
{

    class RealtimeRingBufferWriter;    
    class IPatchWriterCallback;
    class HostWorkerThread;

    class Lv2Effect : public IEffect, private LogFeature::LogMessageListener
    {
    private:
        virtual void OnLogError(const char*message);
        virtual void OnLogWarning(const char*message);
        virtual void OnLogInfo(const char*message);
        virtual void OnLogDebug(const char*message);

    private:
        size_t GetStagedBufferSize() const;

        std::shared_ptr<HostWorkerThread> workerThread;
        std::unique_ptr<Worker> worker;

        std::unordered_map<std::string,int> controlIndex;

        FileBrowserFilesFeature fileBrowserFilesFeature;
        std::unique_ptr<StateInterface> stateInterface;
        bool RestoreState(PedalboardItem&pedalboardItem);
        LogFeature logFeature;
        std::map<std::string,AtomBuffer> patchPropertyPrototypes;

        IHost *pHost = nullptr;
        int numberOfInputs = 0;
        int numberOfOutputs = 0;
        int numberOfMidiInputs = 0;

        LilvInstance *pInstance;
        std::shared_ptr<Lv2PluginInfo> info;
        std::vector<float> controlValues;
        std::vector<int> inputAudioPortIndices;
        std::vector<int> outputAudioPortIndices;

        std::vector<int> inputSidechainPortIndices;
        
        std::vector<int> inputAtomPortIndices;
        std::vector<int> outputAtomPortIndices;

        std::vector<int> inputMidiPortIndices;
        std::vector<int> outputMidiPortIndices;

        std::vector<int> midiInputIndices;

        std::vector<float *> inputAudioBuffers;
        std::vector<float *> inputSidechainBuffers;
        std::vector<float *> outputAudioBuffers;

        std::vector<char *> inputAtomBuffers;
        std::vector<char *> outputAtomBuffers;
        std::vector<const LV2_Feature *> features;
        LV2_Feature *work_schedule_feature = nullptr;
        MapPathFeature mapPathFeature;

        uint64_t maxInputControlPort = 0;
        std::vector<bool> isInputControlPort;
        std::vector<float> defaultInputControlValues;
        std::vector<bool> isInputTriggerControlPort;;
        int bypassControlIndex = -1;

        virtual std::string GetUri() const { return info->uri(); }

        std::vector<const Lv2PortInfo *> realtimePortInfo;

        void PreparePortIndices();
        void ConnectControlPorts();
        void AssignUnconnectedPorts();

        LV2_Atom_Forge inputForgeRt;
        LV2_Atom_Forge_Frame input_frame;

        LV2_Atom_Forge outputForgeRt;
        LV2_Atom_Forge_Frame output_frame;

        std::vector<LV2_URID> pathProperties;
        std::vector<PatchPropertyWriter> pathPropertyWriters;

        std::unordered_map<std::string,std::string> mainThreadPathProperties;

        class Urids
        {
        public:
            Urids(IHost *pHost)
            {
                atom__Chunk = pHost->GetLv2Urid(LV2_ATOM__Chunk);
                atom__Path = pHost->GetLv2Urid(LV2_ATOM__Path);
                atom__Float = pHost->GetLv2Urid(LV2_ATOM__Float);
                atom__Double = pHost->GetLv2Urid(LV2_ATOM__Double);
                atom__Int = pHost->GetLv2Urid(LV2_ATOM__Int);
                atom__Long = pHost->GetLv2Urid(LV2_ATOM__Long);
                atom__Bool = pHost->GetLv2Urid(LV2_ATOM__Bool);
                atom__String = pHost->GetLv2Urid(LV2_ATOM__String);
                atom__Vector = pHost->GetLv2Urid(LV2_ATOM__Vector);
                atom__Object = pHost->GetLv2Urid(LV2_ATOM__Object);


                atom__Sequence = pHost->GetLv2Urid(LV2_ATOM__Sequence);
                atom__URID = pHost->GetLv2Urid(LV2_ATOM__URID);
                patch__Get = pHost->GetLv2Urid(LV2_PATCH__Get);
                patch__Set = pHost->GetLv2Urid(LV2_PATCH__Set);
                patch__Put = pHost->GetLv2Urid(LV2_PATCH__Put);
                patch__body = pHost->GetLv2Urid(LV2_PATCH__body);
                patch__subject = pHost->GetLv2Urid(LV2_PATCH__subject);
                patch__property = pHost->GetLv2Urid(LV2_PATCH__property);
                patch__value = pHost->GetLv2Urid(LV2_PATCH__value);
                units__frame = pHost->GetLv2Urid(LV2_UNITS__frame);
                state__StateChanged = pHost->GetLv2Urid(LV2_STATE__StateChanged);

            }
            LV2_URID atom__Chunk;
            LV2_URID units__frame;
            LV2_URID pluginUri;
            LV2_URID atom__Bool;
            LV2_URID atom__Float;
            LV2_URID atom__Double;
            LV2_URID atom__Int;
            LV2_URID atom__Long;
            LV2_URID atom__String;
            LV2_URID atom__Object;
            LV2_URID atom__Vector;
            LV2_URID atom__Path;
            LV2_URID atom__Sequence;
            LV2_URID atom__URID;
            LV2_URID midi__Event;
            LV2_URID patch__Get;
            LV2_URID patch__Set;
            LV2_URID patch__Put;
            LV2_URID patch__body;
            LV2_URID patch__subject;
            LV2_URID patch__property;
            LV2_URID patch__value;
            LV2_URID state__StateChanged;
        };

        Urids urids;

        uint64_t instanceId;
        BufferPool bufferPool;

        static LV2_Worker_Status worker_schedule_fn(LV2_Worker_Schedule_Handle handle,
                                                    uint32_t size,
                                                    const void *data);


        int GetBypassControlPort() const { return bypassControlIndex; }

        void ResetInputAtomBuffer(char*data);
        void ResetOutputAtomBuffer(char*data);

        uint32_t bypassStartingSamples = 0;

        bool bypass = true;
        double targetBypass = 0;
        double currentBypass = 0;
        double currentBypassDx = 0;
        uint32_t bypassSamplesRemaining = 0;

        bool requestStateChangedNotification = false;

        float zeroInputMix = 0.5f;
        int actualAudioInputs = 0;
        int actualAudioOutputs = 0;
        std::vector<std::vector<float>> outputMixBuffers;
        void BypassDezipperTo(float value);
        void BypassDezipperSet(float value);

        bool borrowedEffect = false;
        bool activated = false;
        void EnableBufferStaging(size_t bufferSize);
        void CheckStagingBufferSentries();

    public:
        bool RequiresBufferStaging() const;
        bool IsBorrowedEffect() const { return borrowedEffect; }
        void SetBorrowedEffect(bool value) { borrowedEffect = value; }
        void UpdateAudioPorts();
        
        // non RT-thread use only.
        std::string GetPathPatchProperty(const std::string&propertyUri);
        // non RT-thread use only.
        void SetPathPatchProperty(const std::string &propertyUri, const std::string&jsonAtom);

        virtual bool IsLv2Effect() const { return true; }
        virtual bool GetLv2State(Lv2PluginState*state) override;
        virtual void SetLv2State(Lv2PluginState&state) override;

        virtual void RequestPatchProperty(LV2_URID uridUri) ;
        virtual void SetPatchProperty(LV2_URID uridUri,size_t size, LV2_Atom*value) override;
        virtual void RequestAllPathPatchProperties();

        virtual bool GetRequestStateChangedNotification() const override;
        virtual void SetRequestStateChangedNotification(bool value)  override;

        virtual void GatherPatchProperties(RealtimePatchPropertyRequest*pRequest);
        void GatherPathPatchProperties(IPatchWriterCallback *cbPatchWriter);        
        virtual bool IsVst3() const { return false; }
        virtual void RelayPatchSetMessages(uint64_t instanceId,RealtimeRingBufferWriter *realtimeRingBufferWriter) ;

        virtual uint8_t*GetAtomInputBuffer() {
            if (this->inputAtomBuffers.size() == 0) return nullptr;
            return (uint8_t*)this->inputAtomBuffers[0];
        }
        virtual uint8_t*GetAtomOutputBuffer()
        {
            if (this->outputAtomBuffers.size() == 0) return nullptr;
            return (uint8_t*)this->outputAtomBuffers[0];

        }
        OptionsFeature optionsFeature;

        bool hasErrorMessage = false;
        char errorMessage[1024];

        bool deleted = false;
        size_t stagingBufferSize = 0;
        size_t stagingInputIx = 0;
        size_t stagingOutputIx = 0;
        std::vector<std::vector<float>> inputStagingBuffers;
        std::vector<std::vector<float>> sidechainStagingBuffers;
        std::vector<std::vector<float>> outputStagingBuffers;
        std::vector<float*> inputStagingBufferPointers;
        std::vector<float*> sidechainStagingBufferPointers;
        std::vector<float*> outputStagingBufferPointers;

        std::vector<uint8_t> stagedInputAtomBuffer;
        void *stagedInputAtomBufferPointer = nullptr;
        std::vector<uint8_t> stagedOutputAtomBuffer;
        void *stagedOutputAtomBufferPointer = nullptr;

        size_t stageToOutput(size_t outputIndex, size_t nFrames);
        size_t stageToInput(size_t inputIndex, size_t nFrames);


        LV2_Atom_Forge stagedInputForgeRt;
        LV2_Atom_Forge_Frame staged_input_frame;

        void MixOutput(uint32_t samples, RealtimeRingBufferWriter *realtimeRingBufferWriter);
        void copyAtomBufferEventSequence(LV2_Atom_Sequence *sequence, LV2_Atom_Forge &outputForge);
        void resetStagedInputAtomBuffer();

    public:
        Lv2Effect(
            IHost *pHost,
            const std::shared_ptr<Lv2PluginInfo> &info,
            PedalboardItem &pedalboardItem);
        ~Lv2Effect();

        bool HasErrorMessage() const { return this->hasErrorMessage; }
        const char*TakeErrorMessage() { this->hasErrorMessage = false; return this->errorMessage; }

        virtual void PrepareNoInputEffect(int numberOfInputs,size_t maxBufferSize) override;


        virtual void ResetAtomBuffers();
        virtual uint64_t GetInstanceId() const { return instanceId; }
        virtual int GetNumberOfInputAudioPorts() const override { return inputAudioPortIndices.size(); }
        virtual int GetNumberOfOutputAudioPorts() const override { return outputAudioPortIndices.size(); }

        virtual int GetNumberOfInputAtomPorts() const  { return inputAtomPortIndices.size(); }

        virtual int GetNumberOfOutputAtomPorts() const  { return outputAtomPortIndices.size(); }
        virtual int GetNumberOfMidiInputPorts() const  { return inputMidiPortIndices.size(); }
        virtual int GetNumberOfMidiOutputPorts() const  { return outputMidiPortIndices.size(); }



        virtual int GetNumberOfInputAudioBuffers() const override { return this->inputAudioBuffers.size(); }
        virtual int GetNumberOfSidechainAudioBuffers() const override { return this->inputSidechainBuffers.size(); }
        virtual int GetNumberOfOutputAudioBuffers() const override {return this->outputAudioBuffers.size(); }

        virtual void SetAudioInputBuffer(int index, float *buffer) override;
        virtual float *GetAudioInputBuffer(int index) const override;

        virtual void SetAudioSidechainBuffer(int index, float *buffer) override;
        virtual float *GetAudioSidechainBuffer(int index) const override;


        virtual void SetAudioInputBuffer(float *buffer);
        virtual void SetAudioInputBuffers(float *left, float *right);

        virtual void SetAudioOutputBuffer(int index, float *buffer);
        virtual float *GetAudioOutputBuffer(int index) const;

        virtual void SetAtomInputBuffer(int index, void *buffer) { this->inputAtomBuffers[index] = (char*)buffer;}
        virtual void *GetAtomInputBuffer(int index) const { return this->inputAtomBuffers[index]; }

        virtual void SetAtomOutputBuffer(int index, void *buffer) { this->outputAtomBuffers[index] = (char*)buffer; }
        virtual void *GetAtomOutputBuffer(int index) const { return this->outputAtomBuffers[index]; }

        virtual int GetControlIndex(const std::string &symbol) const;

        virtual void SetControl(int index, float value) override
        {
            if (index == -1)
            {
                SetBypass(value != 0);
            } else {
                controlValues[index] = value;
            }
        }

        virtual uint64_t GetMaxInputControl() const override;
        virtual bool IsInputControl(uint64_t index) const override;
        virtual float GetDefaultInputControlValue(uint64_t index) const override;


        virtual float GetControlValue(int index) const {
            if (index == -1) 
            {
                return this->bypass? 1: 0;
            }
            return controlValues[index];
        }


        virtual float GetOutputControlValue(int portIndex) const
        {
            if (portIndex >= 0 && portIndex < controlValues.size()) {
                return controlValues[portIndex];
            }
            return 0;
        }

        virtual void SetBypass(bool bypass)
        {
            if (bypass != this->bypass)
            {
                this->bypass = bypass;
                if (bypassControlIndex == -1) {
                    BypassDezipperTo(bypass? 1.0f: 0.0f);
                } else {
                    controlValues[bypassControlIndex] = bypass? 1.0f: 0.0f;
                }
            }

        }

        virtual void Activate();
        virtual void Run(uint32_t samples, RealtimeRingBufferWriter *realtimeRingBufferWriter);
        virtual void RunWithBufferStaging(uint32_t samples, RealtimeRingBufferWriter *realtimeRingBufferWriter);
        virtual void Deactivate();
    };

} // namespace