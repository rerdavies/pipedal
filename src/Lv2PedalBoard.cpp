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
#include "Lv2PedalBoard.hpp"
#include "Lv2Effect.hpp"

#include "SplitEffect.hpp"
#include "RingBufferReader.hpp"
#include "VuUpdate.hpp"
#include "JackHost.hpp"
#include "Lv2EventBufferWriter.hpp"

using namespace pipedal;

float *Lv2PedalBoard::CreateNewAudioBuffer()
{
    return bufferPool.AllocateBuffer<float>(pHost->GetMaxAudioBufferSize());
}

std::vector<float *> Lv2PedalBoard::AllocateAudioBuffers(int nChannels)
{
    std::vector<float *> result;
    for (int i = 0; i < nChannels; ++i)
    {
        result.push_back(bufferPool.AllocateBuffer<float>(pHost->GetMaxAudioBufferSize()));
    }
    return result;
}

int Lv2PedalBoard::GetControlIndex(uint64_t instanceId, const std::string &symbol)
{
    for (int i = 0; i < realtimeEffects.size(); ++i)
    {
        auto item = realtimeEffects[i];
        if (item->GetInstanceId() == instanceId)
        {
            return item->GetControlIndex(symbol);
        }
    }
    return -1;
}
std::vector<float *> Lv2PedalBoard::PrepareItems(
    const std::vector<PedalBoardItem> &items,
    std::vector<float *> inputBuffers)
{
    for (int i = 0; i < items.size(); ++i)
    {
        auto &item = items[i];
        if (!item.isEmpty())
        {
            IEffect *pEffect = nullptr;
            if (item.isSplit())
            {
                auto pSplit = new SplitEffect(item.instanceId(), pHost->GetSampleRate(), inputBuffers);
                pEffect = pSplit;

                int topInputChannels = inputBuffers.size();
                int bottomInputChannels = inputBuffers.size();

                std::vector<float *> topInputs = AllocateAudioBuffers(topInputChannels);
                std::vector<float *> bottomInputs = AllocateAudioBuffers(bottomInputChannels);

                auto preMixAction = [pSplit](uint32_t frames)
                { pSplit->PreMix(frames); };

                this->processActions.push_back(preMixAction);

                std::vector<float *> topResult = PrepareItems(item.topChain(), topInputs);
                std::vector<float *> bottomResult = PrepareItems(item.bottomChain(), bottomInputs);

                this->processActions.push_back(
                    [pSplit](uint32_t frames)
                    { pSplit->PostMix(frames); });

                pSplit->SetChainBuffers(topInputs, bottomInputs, topResult, bottomResult);

                for (int i = 0; i < item.controlValues().size(); ++i)
                {
                    auto &controlValue = item.controlValues()[i];
                    int index = pSplit->GetControlIndex(controlValue.key());
                    if (index != -1)
                    {
                        pSplit->SetControl(index, controlValue.value());
                    }
                }
            }
            else
            {

                auto pLv2Effect = this->pHost->CreateEffect(item);
                if (pLv2Effect)
                {
                    pEffect = pLv2Effect;

                    uint64_t instanceId = pEffect->GetInstanceId();

                    if (inputBuffers.size() == 1)
                    {
                        if (pLv2Effect->GetNumberOfInputAudioPorts() == 1)
                        {
                            pLv2Effect->SetAudioInputBuffer(0, inputBuffers[0]);
                        }
                        else
                        {
                            pLv2Effect->SetAudioInputBuffer(0, inputBuffers[0]);
                            pLv2Effect->SetAudioInputBuffer(1, inputBuffers[0]);
                        }
                    }
                    else
                    {
                        if (pLv2Effect->GetNumberOfInputAudioPorts() == 1)
                        {
                            pLv2Effect->SetAudioInputBuffer(0, inputBuffers[0]);

                            auto inputBuffer = inputBuffers[0];
                        }
                        else
                        {
                            pLv2Effect->SetAudioInputBuffer(0, inputBuffers[0]);
                            pLv2Effect->SetAudioInputBuffer(1, inputBuffers[1]);

                            auto bufferL = inputBuffers[0];
                            auto bufferR = inputBuffers[1];
                        }
                    }

                    this->processActions.push_back(
                        [pLv2Effect,this,instanceId](uint32_t frames)
                        {
                            pLv2Effect->Run(frames,instanceId,this->ringBufferWriter);
                        });
                }
            }
            if (pEffect)
            {
                this->effects.push_back(std::shared_ptr<IEffect>(pEffect)); // for ownership.
                this->realtimeEffects.push_back(pEffect);                   // because std::shared_ptr is not threadsafe.

                std::vector<float *> effectOutput;

#ifdef RECYCLE_AUDIO_BUFFERS
                // can't do this anymore if we're going to do pop-less bypbass.
                if (pEffect->GetNumberOfOutputAudioPorts() == 1)
                {
                    float *pLeft = inputBuffers[0];
                    effectOutput.push_back(pLeft);
                }
                else
                {
                    if (inputBuffers.size() == 1)
                    {
                        effectOutput.push_back(inputBuffers[0]);
                        effectOutput.push_back(CreateNewAudioBuffer());
                    }
                    else
                    {
                        effectOutput.push_back(inputBuffers[0]);
                        effectOutput.push_back(inputBuffers[1]);
                    }
                }
#else
                if (pEffect->GetNumberOfOutputAudioPorts() == 1)
                {
                    effectOutput.push_back(CreateNewAudioBuffer());
                }
                else
                {
                    effectOutput.push_back(CreateNewAudioBuffer());
                    effectOutput.push_back(CreateNewAudioBuffer());
                }
#endif
                for (size_t i = 0; i < effectOutput.size(); ++i)
                {
                    pEffect->SetAudioOutputBuffer(i, effectOutput[i]);
                }
                inputBuffers = effectOutput;
            }
        }
    }
    return inputBuffers;
}

void Lv2PedalBoard::Prepare(IHost *pHost, const PedalBoard &pedalBoard)
{
    this->pHost = pHost;

    for (int i = 0; i < pHost->GetNumberOfInputAudioChannels(); ++i)
    {
        this->pedalBoardInputBuffers.push_back(bufferPool.AllocateBuffer<float>(pHost->GetMaxAudioBufferSize()));
    }

    auto outputs = PrepareItems(pedalBoard.items(), this->pedalBoardInputBuffers);
    int nOutputs = pHost->GetNumberOfOutputAudioChannels();
    if (nOutputs == 1)
    {
        this->pedalBoardOutputBuffers.push_back(outputs[0]);
    }
    else
    {
        if (outputs.size() == 1)
        {
            this->pedalBoardOutputBuffers.push_back(outputs[0]);
            this->pedalBoardOutputBuffers.push_back(outputs[0]);
        }
        else
        {
            this->pedalBoardOutputBuffers.push_back(outputs[0]);
            this->pedalBoardOutputBuffers.push_back(outputs[1]);
        }
    }
    PrepareMidiMap(pedalBoard);
}

void Lv2PedalBoard::PrepareMidiMap(const PedalBoardItem &pedalBoardItem)
{
    if (pedalBoardItem.midiBindings().size() != 0)
    {
        auto pluginInfo = pHost->GetPluginInfo(pedalBoardItem.uri());
        const Lv2PluginInfo *pPluginInfo;
        if (pluginInfo == nullptr && pedalBoardItem.uri() == SPLIT_PEDALBOARD_ITEM_URI)
        {
            pPluginInfo = GetSplitterPluginInfo();
        } else {
            pPluginInfo = pluginInfo.get();
        }

        int effectIndex = this->GetIndexOfInstanceId(pedalBoardItem.instanceId());

        if (pluginInfo && effectIndex != -1)
        {
            for (size_t bindingIndex = 0; bindingIndex < pedalBoardItem.midiBindings().size(); ++bindingIndex)
            {
                auto &binding = pedalBoardItem.midiBindings()[bindingIndex];
                {
                    const Lv2PortInfo*pPortInfo;
                    int controlIndex;
                    if (binding.symbol() == "__bypass")
                    {
                        pPortInfo = GetBypassPortInfo();
                        controlIndex = -1;
                    } else {
                        try {
                            pPortInfo = &pluginInfo->getPort(binding.symbol());
                            controlIndex = this->GetControlIndex(pedalBoardItem.instanceId(), binding.symbol());
                        } catch (const std::exception&ignored)
                        {
                            continue;
                        }
                        

                    }
                    
                    MidiMapping mapping;
                    mapping.pluginInfo = pluginInfo; // for lifetime management. <shrugs> We're holding internal pointers to this. May save us in a disorderly shutdown.
                    mapping.pPortInfo = pPortInfo;
                    mapping.effectIndex = effectIndex;
                    mapping.controlIndex = controlIndex;
                    mapping.midiBinding = binding;
                    mapping.instanceId = pedalBoardItem.instanceId();
                    if (pPortInfo->IsSwitch())
                    {
                        mapping.mappingType = binding.switchControlType() == LATCH_CONTROL_TYPE ? MappingType::Latched : MappingType::Momentary;
                    }
                    else
                    {
                        mapping.mappingType = binding.linearControlType() == LATCH_CONTROL_TYPE ? MappingType::Linear : MappingType::Circular;
                    }
                    if (binding.bindingType() == BINDING_TYPE_NOTE)
                    {
                        mapping.key = 0x9000 | binding.note(); // i.e. midi note on.
                    }
                    else if (binding.bindingType() == BINDING_TYPE_CONTROL)
                    {
                        mapping.key = 0xB000 | binding.control(); // i.e. midi control
                    }
                    else
                    {
                        mapping.key = -1;
                    }
                    if (mapping.key != -1)
                    {
                        midiMappings.push_back(std::move(mapping));
                    }
                }
            }
        }
    }
    for (size_t i = 0; i < pedalBoardItem.topChain().size(); ++i)
    {
        PrepareMidiMap(pedalBoardItem.topChain()[i]);
    }
    for (size_t i = 0; i < pedalBoardItem.bottomChain().size(); ++i)
    {
        PrepareMidiMap(pedalBoardItem.bottomChain()[i]);
    }
}
void Lv2PedalBoard::PrepareMidiMap(const PedalBoard &pedalBoard)
{
    for (size_t i = 0; i < pedalBoard.items().size(); ++i)
    {
        auto &item = pedalBoard.items()[i];
        PrepareMidiMap(item);

        auto pluginInfo = pHost->GetPluginInfo(item.uri());

        if (pluginInfo)
        {
            for (size_t bindingIndex = 0; bindingIndex < item.midiBindings().size(); ++bindingIndex)
            {
                auto &binding = item.midiBindings()[i];
                {
                }
            }
        }
        std::sort(this->midiMappings.begin(), this->midiMappings.end(),
                  [](const MidiMapping &left, const MidiMapping &right)
                  { return left.key < right.key; });
    }
}
void Lv2PedalBoard::Activate()
{
    for (int i = 0; i < this->effects.size(); ++i)
    {
        this->realtimeEffects[i]->Activate();
    }
}
void Lv2PedalBoard::Deactivate()
{
    for (int i = 0; i < this->effects.size(); ++i)
    {
        this->realtimeEffects[i]->Deactivate();
    }
}

static void Copy(float *input, float *output, uint32_t samples)
{
    for (uint32_t i = 0; i < samples; ++i)
    {
        output[i] = input[i];
    }
}
bool Lv2PedalBoard::Run(float **inputBuffers, float **outputBuffers, uint32_t samples, RealtimeRingBufferWriter*ringBufferWriter)
{
    this->ringBufferWriter = ringBufferWriter;
    for (int i = 0; i < this->pedalBoardInputBuffers.size(); ++i)
    {
        if (inputBuffers[i] == nullptr)
            return false;
        Copy(inputBuffers[i], this->pedalBoardInputBuffers[i], samples);
    }
    for (int i = 0; i < this->processActions.size(); ++i)
    {
        processActions[i](samples);
    }
    for (int i = 0; i < this->pedalBoardOutputBuffers.size(); ++i)
    {
        if (outputBuffers[i] == nullptr)
            return false;
        Copy(this->pedalBoardOutputBuffers[i], outputBuffers[i], samples);
    }
    return true;
}

float Lv2PedalBoard::GetControlOutputValue(int effectIndex, int portIndex)
{
    auto effect = realtimeEffects[effectIndex];
    return effect->GetOutputControlValue(portIndex);
}

void Lv2PedalBoard::SetControlValue(int effectIndex, int index, float value)
{
    auto effect = realtimeEffects[effectIndex];
    effect->SetControl(index, value);
}
void Lv2PedalBoard::SetBypass(int effectIndex, bool enabled)
{
    auto effect = realtimeEffects[effectIndex];
    effect->SetBypass(enabled);
}

void Lv2PedalBoard::ComputeVus(RealtimeVuBuffers *vuConfiguration, uint32_t samples)
{
    for (size_t i = 0; i < vuConfiguration->enabledIndexes.size(); ++i)
    {
        int index = vuConfiguration->enabledIndexes[i];
        VuUpdate *pUpdate = &vuConfiguration->vuUpdateWorkingData[i];

        auto effect = this->realtimeEffects[index];

        if (effect->GetNumberOfInputAudioPorts() == 1)
        {
            pUpdate->AccumulateInputs(effect->GetAudioInputBuffer(0), samples);
        }
        else
        {
            pUpdate->AccumulateInputs(
                effect->GetAudioInputBuffer(0),
                effect->GetAudioInputBuffer(1), samples);
        }
        if (effect->GetNumberOfOutputAudioPorts() == 1)
        {
            pUpdate->AccumulateOutputs(effect->GetAudioOutputBuffer(0), samples);
        }
        else
        {
            pUpdate->AccumulateOutputs(
                effect->GetAudioOutputBuffer(0),
                effect->GetAudioOutputBuffer(1),
                samples);
        }
    }
}

void Lv2PedalBoard::ResetAtomBuffers()
{
    for (size_t i = 0; i < this->effects.size(); ++i)
    {
        auto effect = this->effects[i];
        effect->ResetAtomBuffers();
    }
}

void Lv2PedalBoard::ProcessParameterRequests(RealtimeParameterRequest *pParameterRequests)
{
    while (pParameterRequests != nullptr)
    {
        IEffect *pEffect = this->GetEffect(pParameterRequests->instanceId);
        if (pEffect == nullptr)
        {
            pParameterRequests->errorMessage = "No such effect.";
        }
        else
        {
            pEffect->RequestParameter(pParameterRequests->uridUri);
        }
        pParameterRequests = pParameterRequests->pNext;
    }
}

void Lv2PedalBoard::GatherParameterRequests(RealtimeParameterRequest *pParameterRequests)
{
    while (pParameterRequests != nullptr)
    {
        IEffect *effect = this->GetEffect(pParameterRequests->instanceId);
        if (effect == nullptr)
        {
            pParameterRequests->errorMessage = "No such effect.";
        }
        else
        {
            effect->GatherParameter(pParameterRequests);
        }

        pParameterRequests = pParameterRequests->pNext;
    }
}

void Lv2PedalBoard::OnMidiMessage(size_t size, uint8_t *message,
                                  void *callbackHandle,
                                  MidiCallbackFn *pfnCallback)

{
    if (midiMappings.size() == 0)
        return;

    if (size < 2)
        return;
    uint8_t cmd = message[0];
    uint8_t channel = cmd & 0x0F;
    cmd &= 0xF0;

    uint8_t value;
    uint8_t index;
    if (cmd == 0x80) // note off.
    {
        index = message[1];
        cmd = 0x90;
        index = message[1];
        value = 0;
    }
    else if (cmd == 0x90) // note on.
    {
        if (size < 3)
            return;
        index = message[1];
        value = 127;
    }
    else if (cmd == 0xB0) // midi control.
    {
        if (size < 3)
            return;
        index = message[1];
        value = message[2] & 0x7F;
    }
    int searchKey = (cmd << 8) | index;
    int min = 0;
    int max = midiMappings.size() - 1;
    while (max > min)
    {
        int mid = (min + max) / 2;
        if (midiMappings[mid].key < searchKey)
        {
            min = mid + 1;
        }
        else if (midiMappings[mid].key > searchKey)
        {
            max = mid - 1;
        }
        else
        {
            if (mid == 0)
            {
                min = max = mid;
            }
            else
            {
                if (midiMappings[mid - 1].key == searchKey)
                {
                    max = mid - 1;
                }
                else
                {
                    min = max = mid;
                }
            }
        }
    }
    if (midiMappings[min].key == searchKey)
    {
        float range = value / 127.0;

        for (int i = min; i < midiMappings.size(); ++i)
        {
            auto &mapping = midiMappings[i];
            if (mapping.key != searchKey)
                break;

            if (mapping.midiBinding.channel() == -1 || mapping.midiBinding.channel() == channel)
            {
                switch (mapping.mappingType)
                {
                case MappingType::Circular:
                case MappingType::Linear:
                {
                    float thisRange = (mapping.midiBinding.maxValue() - mapping.midiBinding.minValue()) * range + mapping.midiBinding.minValue();
                    float value = mapping.pPortInfo->rangeToValue(thisRange);
                    this->SetControlValue(mapping.effectIndex, mapping.controlIndex, value);
                    pfnCallback(callbackHandle, mapping.instanceId, mapping.pPortInfo->index(), value);
                    break;
                }
                case MappingType::Latched:
                {
                    range = std::round(range);

                    if (!mapping.hasLastValue)
                    {
                        mapping.lastValue = 0;
                        mapping.hasLastValue = true;
                    }
                    if (range != mapping.lastValue && range == 1)
                    {
                        IEffect *pEffect = this->realtimeEffects[mapping.effectIndex];
                        float currentValue = pEffect->GetControlValue(mapping.controlIndex);
                        currentValue = currentValue == 0 ? 1 : 0;
                        pEffect->SetControl(mapping.controlIndex, currentValue);
                        pfnCallback(callbackHandle, mapping.instanceId, mapping.pPortInfo->index(), currentValue);
                    }
                    mapping.lastValue = range;
                    break;
                }
                case MappingType::Momentary:
                {
                    IEffect *pEffect = this->realtimeEffects[mapping.effectIndex];
                    float value = mapping.pPortInfo->rangeToValue(range);
                    if (pEffect->GetControlValue(mapping.controlIndex) != value)
                    {
                        this->SetControlValue(mapping.effectIndex, mapping.controlIndex, value);
                        pfnCallback(callbackHandle, mapping.instanceId, mapping.pPortInfo->index(), value);
                    }
                    break;
                }
                }
            }
        }
    }
}