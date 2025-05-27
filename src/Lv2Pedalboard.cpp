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

#include "pch.h"
#include "Lv2Pedalboard.hpp"
#include "Lv2Effect.hpp"

#include "SplitEffect.hpp"
#include "RingBufferReader.hpp"
#include "VuUpdate.hpp"
#include "AudioHost.hpp"
#include "Lv2EventBufferWriter.hpp"
#include "Lv2Log.hpp"

using namespace pipedal;

float *Lv2Pedalboard::CreateNewAudioBuffer()
{
    return bufferPool.AllocateBuffer<float>(pHost->GetMaxAudioBufferSize());
}

std::vector<float *> Lv2Pedalboard::AllocateAudioBuffers(int nChannels)
{
    std::vector<float *> result;
    for (int i = 0; i < nChannels; ++i)
    {
        result.push_back(bufferPool.AllocateBuffer<float>(pHost->GetMaxAudioBufferSize()));
    }
    return result;
}

int Lv2Pedalboard::GetControlIndex(uint64_t instanceId, const std::string &symbol)
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
std::vector<float *> Lv2Pedalboard::PrepareItems(
    std::vector<PedalboardItem> &items,
    std::vector<float *> inputBuffers,
    Lv2PedalboardErrorList &errorList,
    ExistingEffectMap *existingEffects)
{
    for (int i = 0; i < items.size(); ++i)
    {
        auto &item = items[i];
        if (!item.isEmpty())
        {
            std::shared_ptr<IEffect> pEffect = nullptr;

            if (item.isSplit())
            {
                auto pSplit = new SplitEffect(item.instanceId(), pHost->GetSampleRate(), inputBuffers);
                pEffect = std::shared_ptr<IEffect>(pSplit);

                int topInputChannels = inputBuffers.size();
                int bottomInputChannels = inputBuffers.size();

                std::vector<float *> topInputs = AllocateAudioBuffers(topInputChannels);
                std::vector<float *> bottomInputs = AllocateAudioBuffers(bottomInputChannels);

                auto preMixAction = [pSplit](uint32_t frames)
                { pSplit->PreMix(frames); };

                this->processActions.push_back(preMixAction);

                std::vector<float *> topResult = PrepareItems(item.topChain(), topInputs, errorList,existingEffects);
                std::vector<float *> bottomResult = PrepareItems(item.bottomChain(), bottomInputs, errorList,existingEffects);

                this->processActions.push_back(
                    [pSplit](uint32_t frames)
                    { pSplit->PostMix(frames); });
                auto controlValue = item.GetControlValue("splitType");
                // if split is L/R, always output stereo.

                bool forceStereo = (controlValue != nullptr && controlValue->value() == 2);
                pSplit->SetChainBuffers(topInputs, bottomInputs, topResult, bottomResult, forceStereo);

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
                std::shared_ptr<IEffect> pLv2Effect;

                if (existingEffects && existingEffects->contains(item.instanceId())
                )
                {
                    pLv2Effect = existingEffects->at(item.instanceId());
                    ((Lv2Effect*)pLv2Effect.get())->SetBorrowedEffect(true);
                }
                else
                {
                    try
                    {
                        pLv2Effect = std::shared_ptr<IEffect>(this->pHost->CreateEffect(item));
                    }
                    catch (const std::exception &e)
                    {
                        Lv2Log::warning(SS(e.what()));
                    }

                    if (pLv2Effect && pLv2Effect->HasErrorMessage())
                    {
                        std::string error = pLv2Effect->TakeErrorMessage();
                        Lv2Log::error(error);
                        errorList.push_back({item.instanceId(), error});
                    }
                }

                if (pLv2Effect)
                {

                    pEffect = pLv2Effect;

                    uint64_t instanceId = pEffect->GetInstanceId();
                    pLv2Effect->PrepareNoInputEffect(inputBuffers.size(), pHost->GetMaxAudioBufferSize());

                    if (inputBuffers.size() == 1)
                    {
                        if (pLv2Effect->GetNumberOfInputAudioBuffers() == 1)
                        {
                            pLv2Effect->SetAudioInputBuffer(0, inputBuffers[0]);
                        }
                        else if (pLv2Effect->GetNumberOfInputAudioBuffers() >= 2)
                        {
                            pLv2Effect->SetAudioInputBuffer(0, inputBuffers[0]);
                            pLv2Effect->SetAudioInputBuffer(1, inputBuffers[0]);
                        }
                    }
                    else
                    {
                        if (pLv2Effect->GetNumberOfInputAudioBuffers() == 1)
                        {
                            pLv2Effect->SetAudioInputBuffer(0, inputBuffers[0]);

                            auto inputBuffer = inputBuffers[0];
                        }
                        else if (pLv2Effect->GetNumberOfInputAudioBuffers() >= 2)
                        {
                            pLv2Effect->SetAudioInputBuffer(0, inputBuffers[0]);
                            pLv2Effect->SetAudioInputBuffer(1, inputBuffers[1]);

                            auto bufferL = inputBuffers[0];
                            auto bufferR = inputBuffers[1];
                        }
                    }

                    this->processActions.push_back(
                        [pLv2Effect, this](uint32_t frames)
                        {
                            pLv2Effect->Run(frames, this->ringBufferWriter);
                        });

                    // Reset any trigger controls to default state after processing
                    if (pLv2Effect->IsLv2Effect())
                    {
                        Lv2Effect *lv2Effect = (Lv2Effect *)pLv2Effect.get();
                        auto pluginInfo = pHost->GetPluginInfo(item.uri());
                        if (pluginInfo)
                        {
                            for (auto control : pluginInfo->ports())
                            {
                                if (control->trigger_property() && control->is_input() && control->is_control_port())
                                {
                                    int controlIndex = lv2Effect->GetControlIndex(control->symbol());
                                    if (controlIndex >= 0)
                                    {
                                        float defaultValue = control->default_value();
                                        this->processActions.push_back(
                                            [pLv2Effect, controlIndex, defaultValue](int32_t frames)
                                            {
                                                pLv2Effect->SetControl(controlIndex, defaultValue);
                                            });
                                    }
                                }
                            }
                        }
                    }
                }
            }
            if (pEffect)
            {
                this->effects.push_back(pEffect);               // for ownership.
                this->realtimeEffects.push_back(pEffect.get()); // because std::shared_ptr is not threadsafe.

                std::vector<float *> effectOutput;

                if (pEffect->GetNumberOfOutputAudioBuffers() == 1)
                {
                    effectOutput.push_back(CreateNewAudioBuffer());
                }
                else if (pEffect->GetNumberOfOutputAudioBuffers() >= 2)
                {
                    effectOutput.push_back(CreateNewAudioBuffer());
                    effectOutput.push_back(CreateNewAudioBuffer());
                }
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

void Lv2Pedalboard::Prepare(IHost *pHost, Pedalboard &pedalboard, Lv2PedalboardErrorList &errorList, ExistingEffectMap *existingEffects)
{
    this->pHost = pHost;

    inputVolume.SetSampleRate((float)(this->pHost->GetSampleRate()));
    outputVolume.SetSampleRate((float)(this->pHost->GetSampleRate()));
    inputVolume.SetMinDb(-60);
    outputVolume.SetMinDb(-60);

    inputVolume.SetTarget(pedalboard.input_volume_db());
    outputVolume.SetTarget(pedalboard.output_volume_db());

    for (int i = 0; i < pHost->GetNumberOfInputAudioChannels(); ++i)
    {
        this->pedalboardInputBuffers.push_back(bufferPool.AllocateBuffer<float>(pHost->GetMaxAudioBufferSize()));
    }

    auto outputs = PrepareItems(pedalboard.items(), this->pedalboardInputBuffers, errorList, existingEffects);
    int nOutputs = pHost->GetNumberOfOutputAudioChannels();
    if (nOutputs == 1)
    {
        this->pedalboardOutputBuffers.push_back(outputs[0]);
    }
    else
    {
        if (outputs.size() == 1)
        {
            this->pedalboardOutputBuffers.push_back(outputs[0]);
            this->pedalboardOutputBuffers.push_back(outputs[0]);
        }
        else
        {
            this->pedalboardOutputBuffers.push_back(outputs[0]);
            this->pedalboardOutputBuffers.push_back(outputs[1]);
        }
    }
    PrepareMidiMap(pedalboard);
}

void Lv2Pedalboard::PrepareMidiMap(const PedalboardItem &pedalboardItem)
{
    if (pedalboardItem.midiBindings().size() != 0)
    {
        Lv2PluginInfo::ptr pluginInfo;
        if (pedalboardItem.uri() == SPLIT_PEDALBOARD_ITEM_URI)
        {
            pluginInfo = GetSplitterPluginInfo();
        }
        else
        {
            pluginInfo = pHost->GetPluginInfo(pedalboardItem.uri());
        }

        int effectIndex = this->GetIndexOfInstanceId(pedalboardItem.instanceId());

        if (pluginInfo && effectIndex != -1)
        {
            for (size_t bindingIndex = 0; bindingIndex < pedalboardItem.midiBindings().size(); ++bindingIndex)
            {
                auto &binding = pedalboardItem.midiBindings()[bindingIndex];
                {
                    const Lv2PortInfo *pPortInfo;
                    int controlIndex;
                    if (binding.symbol() == "__bypass")
                    {
                        pPortInfo = GetBypassPortInfo();
                        controlIndex = -1;
                    }
                    else
                    {
                        try
                        {
                            pPortInfo = &pluginInfo->getPort(binding.symbol());
                            controlIndex = this->GetControlIndex(pedalboardItem.instanceId(), binding.symbol());
                        }
                        catch (const std::exception &ignored)
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
                    mapping.instanceId = pedalboardItem.instanceId();


                    if (pPortInfo->mod_momentaryOffByDefault() || pPortInfo->mod_momentaryOnByDefault())
                    {
                        mapping.mappingType = MidiControlType::MomentarySwitch;
                    }
                    else if (pPortInfo->trigger_property())
                    {
                        mapping.mappingType = MidiControlType::Trigger;
                    }
                    else if (pPortInfo->IsSwitch())
                    {
                        mapping.mappingType = MidiControlType::Toggle;
                    }
                    else if (pPortInfo->enumeration_property())
                    {
                        mapping.mappingType = MidiControlType::Select;
                    }
                    else
                    {
                        mapping.mappingType = MidiControlType::Dial;
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
    for (size_t i = 0; i < pedalboardItem.topChain().size(); ++i)
    {
        PrepareMidiMap(pedalboardItem.topChain()[i]);
    }
    for (size_t i = 0; i < pedalboardItem.bottomChain().size(); ++i)
    {
        PrepareMidiMap(pedalboardItem.bottomChain()[i]);
    }
}
void Lv2Pedalboard::PrepareMidiMap(const Pedalboard &pedalboard)
{
    for (size_t i = 0; i < pedalboard.items().size(); ++i)
    {
        auto &item = pedalboard.items()[i];
        PrepareMidiMap(item);
    }
    std::sort(this->midiMappings.begin(), this->midiMappings.end(),
              [](const MidiMapping &left, const MidiMapping &right)
              { return left.key < right.key; });
}

void Lv2Pedalboard::UpdateAudioPorts()
{
    for (int i = 0; i < this->effects.size(); ++i)
    {
        IEffect *effect = this->realtimeEffects[i];
        if (effect->IsLv2Effect())
        {
            Lv2Effect *lv2Effect = (Lv2Effect *)effect;
            lv2Effect->UpdateAudioPorts();
        }   
    }
}

void Lv2Pedalboard::Activate()
{
    for (int i = 0; i < this->effects.size(); ++i)
    {
        this->realtimeEffects[i]->Activate();
    }
}
void Lv2Pedalboard::Deactivate()
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
bool Lv2Pedalboard::Run(float **inputBuffers, float **outputBuffers, uint32_t samples, RealtimeRingBufferWriter *ringBufferWriter)
{
    this->ringBufferWriter = ringBufferWriter;
    for (size_t i = 0; i < this->pedalboardInputBuffers.size(); ++i)
    {
        if (inputBuffers[i] == nullptr)
        {
            return false;
        }
    }
    for (size_t i = 0; i < samples; ++i)
    {
        float volume = this->inputVolume.Tick();
        for (int c = 0; c < this->pedalboardInputBuffers.size(); ++c)
        {
            this->pedalboardInputBuffers[c][i] = inputBuffers[c][i] * volume;
        }
    }
    for (int i = 0; i < this->processActions.size(); ++i)
    {
        processActions[i](samples);
    }
    for (size_t i = 0; i < this->effects.size(); ++i)
    {
        IEffect *effect = effects[i].get();
        if (effect->HasErrorMessage())
        {
            ringBufferWriter->WriteLv2ErrorMessage(effect->GetInstanceId(), effect->TakeErrorMessage());
        }
    }
    for (size_t i = 0; i < samples; ++i)
    {
        float volume = outputVolume.Tick();
        for (size_t c = 0; c < this->pedalboardOutputBuffers.size(); ++c)
        {
            outputBuffers[c][i] = this->pedalboardOutputBuffers[c][i] * volume;
        }
    }
    return true;
}

float Lv2Pedalboard::GetControlOutputValue(int effectIndex, int portIndex)
{
    auto effect = realtimeEffects[effectIndex];
    return effect->GetOutputControlValue(portIndex);
}

void Lv2Pedalboard::SetControlValue(int effectIndex, int index, float value)
{
    auto effect = realtimeEffects[effectIndex];
    effect->SetControl(index, value);
}
void Lv2Pedalboard::SetBypass(int effectIndex, bool enabled)
{
    auto effect = realtimeEffects[effectIndex];
    effect->SetBypass(enabled);
}

void Lv2Pedalboard::ComputeVus(RealtimeVuBuffers *vuConfiguration, uint32_t samples, float **inputBuffers, float **outputBuffers)
{
    for (size_t i = 0; i < vuConfiguration->enabledIndexes.size(); ++i)
    {
        int index = vuConfiguration->enabledIndexes[i];
        VuUpdate *pUpdate = &vuConfiguration->vuUpdateWorkingData[i];
        if (index == Pedalboard::INPUT_VOLUME_ID)
        {
            if (this->pedalboardInputBuffers.size() > 1)
            {
                GetInputBuffers();
                pUpdate->AccumulateInputs(inputBuffers[0], inputBuffers[1], samples);
                pUpdate->AccumulateOutputs(&(this->pedalboardInputBuffers[0][0]), &(this->pedalboardInputBuffers[1][0]), samples); // after outputVolume applied.
            }
            else
            {
                pUpdate->AccumulateInputs(inputBuffers[0], samples);
                pUpdate->AccumulateOutputs(&(this->pedalboardInputBuffers[0][0]), samples); // after input volume applied.
            }
        }
        else if (index == Pedalboard::OUTPUT_VOLUME_ID)
        {
            if (this->pedalboardOutputBuffers.size() > 1)
            {
                pUpdate->AccumulateInputs(&(this->pedalboardOutputBuffers[0][0]), &(this->pedalboardOutputBuffers[1][0]), samples);
                pUpdate->AccumulateOutputs(outputBuffers[0], outputBuffers[1], samples);
            }
            else
            {
                pUpdate->AccumulateInputs(&(this->pedalboardOutputBuffers[0][0]), samples);
                pUpdate->AccumulateOutputs(outputBuffers[0], samples);
            }
        }
        else
        {
            auto effect = this->realtimeEffects[index];

            if (effect->GetNumberOfInputAudioBuffers() == 1)
            {
                pUpdate->AccumulateInputs(effect->GetAudioInputBuffer(0), samples);
            }
            else if (effect->GetNumberOfInputAudioBuffers() >= 2)
            {
                pUpdate->AccumulateInputs(
                    effect->GetAudioInputBuffer(0),
                    effect->GetAudioInputBuffer(1), samples);
            }
            if (effect->GetNumberOfOutputAudioBuffers() == 1)
            {
                pUpdate->AccumulateOutputs(effect->GetAudioOutputBuffer(0), samples);
            }
            else if (effect->GetNumberOfOutputAudioBuffers() >= 2)
            {
                pUpdate->AccumulateOutputs(
                    effect->GetAudioOutputBuffer(0),
                    effect->GetAudioOutputBuffer(1),
                    samples);
            }
        }
    }
}

void Lv2Pedalboard::ResetAtomBuffers()
{
    for (size_t i = 0; i < this->effects.size(); ++i)
    {
        auto effect = this->effects[i];
        effect->ResetAtomBuffers();
    }
}

void Lv2Pedalboard::GatherPathPatchProperties(IPatchWriterCallback *cbPatchWriter)
{
    for (auto &pEffect : this->effects)
    {
        if (pEffect->IsLv2Effect())
        {
            Lv2Effect *pLv2Effect = (Lv2Effect *)pEffect.get();
            pLv2Effect->GatherPathPatchProperties(cbPatchWriter);
        }
    }
}

void Lv2Pedalboard::ProcessParameterRequests(RealtimePatchPropertyRequest *pParameterRequests, size_t samplesThisTime)
{
    while (pParameterRequests != nullptr)
    {
        pParameterRequests->sampleTimeout -= samplesThisTime;
        IEffect *pEffect = this->GetEffect(pParameterRequests->instanceId);

        if (pEffect == nullptr)
        {
            pParameterRequests->errorMessage = "No such effect.";
        }
        else if (pEffect->IsVst3())
        {
            pParameterRequests->errorMessage = "Not supported for VST3 plugins";
        } else if (pParameterRequests->sampleTimeout < 0)
        {
            pParameterRequests->sampleTimeout = 0;
            pParameterRequests->errorMessage = "Timed out.";
        }
        else 
        {
            if (pEffect->IsLv2Effect())
            {
                Lv2Effect *pLv2Effect = dynamic_cast<Lv2Effect *>(pEffect);

                if (pParameterRequests->requestType == RealtimePatchPropertyRequest::RequestType::PatchGet)
                {
                    pLv2Effect->RequestPatchProperty(pParameterRequests->uridUri);
                }
                else if (pParameterRequests->requestType == RealtimePatchPropertyRequest::RequestType::PatchSet)
                {
                    pLv2Effect->SetPatchProperty(
                        pParameterRequests->uridUri,
                        pParameterRequests->GetSize(),
                        (LV2_Atom *)pParameterRequests->GetBuffer());
                    pLv2Effect->SetRequestStateChangedNotification(true);
                }
            }
        }
        pParameterRequests = pParameterRequests->pNext;
    }
}

void Lv2Pedalboard::GatherPatchProperties(RealtimePatchPropertyRequest *pParameterRequests)
{
    while (pParameterRequests != nullptr)
    {
        if (pParameterRequests->requestType == RealtimePatchPropertyRequest::RequestType::PatchGet)
        {
            IEffect *effect = this->GetEffect(pParameterRequests->instanceId);
            if (effect == nullptr)
            {
                pParameterRequests->errorMessage = "No such effect.";
            }
            else if (effect->IsVst3())
            {
                pParameterRequests->errorMessage = "Not supported for VST3";
            }
            else
            {
                if (effect->IsLv2Effect())
                {
                    Lv2Effect *pLv2Effect = dynamic_cast<Lv2Effect *>(effect);
                    pLv2Effect->GatherPatchProperties(pParameterRequests);
                }
            }
        }

        pParameterRequests = pParameterRequests->pNext;
    }
}

void Lv2Pedalboard::OnMidiMessage(size_t size, uint8_t *message,
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
        value = message[2] == 0 ? 0 : 127; // zero velocity = note off.
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
                case MidiControlType::Trigger:
                {
                    bool triggered = false;
                    if (mapping.midiBinding.switchControlType() == SwitchControlTypeT::TRIGGER_ON_RISING_EDGE || mapping.midiBinding.bindingType() == BINDING_TYPE_NOTE)
                    {
                        if (mapping.lastValue < range)
                        {
                            if (!mapping.lastValueIncreasing)
                            {
                                triggered = true;
                            }
                            mapping.lastValueIncreasing = true;
                            mapping.lastValue = range;
                        }
                        else
                        {
                            mapping.lastValueIncreasing = false;
                            mapping.lastValue = range;
                        }
                    }
                    else
                    {
                        triggered = true;
                    }
                    if (triggered)
                    {
                        IEffect *pEffect = this->realtimeEffects[mapping.effectIndex];
                        float value = mapping.pPortInfo->max_value();
                        if (value == mapping.pPortInfo->default_value())
                        {
                            value = mapping.pPortInfo->min_value();
                        }
                        this->SetControlValue(mapping.effectIndex, mapping.controlIndex, value);
                        // do NOT notify anyone!
                    }
                    break;
                }
                case MidiControlType::Toggle:
                {
                    bool triggered = false;

                    range = std::round(range);

                    if (mapping.midiBinding.switchControlType() == SwitchControlTypeT::TOGGLE_ON_RISING_EDGE)
                    {
                        if (range > mapping.lastValue)
                        {
                            if (!mapping.lastValueIncreasing)
                            {
                                triggered = true;
                            }
                            mapping.lastValueIncreasing = true;
                            mapping.lastValue = range;
                        }
                        else
                        {
                            mapping.lastValueIncreasing = false;
                            mapping.lastValue = range;
                        }
                        if (triggered)
                        {
                            IEffect *pEffect = this->realtimeEffects[mapping.effectIndex];
                            float currentValue = pEffect->GetControlValue(mapping.controlIndex);

                            currentValue = currentValue == 0 ? 1 : 0;
                            pEffect->SetControl(mapping.controlIndex, currentValue);
                            pfnCallback(callbackHandle, mapping.instanceId, mapping.pPortInfo->index(), currentValue);
                        }
                    }
                    else if (mapping.midiBinding.switchControlType() == SwitchControlTypeT::TOGGLE_ON_VALUE)
                    {
                        triggered = true;
                        mapping.lastValue = range;
                        IEffect *pEffect = this->realtimeEffects[mapping.effectIndex];
                        float currentValue = pEffect->GetControlValue(mapping.controlIndex);
                        if (currentValue != range)
                        {
                            pEffect->SetControl(mapping.controlIndex, range);
                            pfnCallback(callbackHandle, mapping.instanceId, mapping.pPortInfo->index(), range);
                        }
                    }
                    else
                    {
                        // any control value toggles.
                        triggered = true;
                        mapping.lastValue = range;
                        IEffect *pEffect = this->realtimeEffects[mapping.effectIndex];
                        float currentValue = pEffect->GetControlValue(mapping.controlIndex);
                        currentValue = currentValue == 0 ? 1 : 0;
                        pEffect->SetControl(mapping.controlIndex, currentValue);
                        pfnCallback(callbackHandle, mapping.instanceId, mapping.pPortInfo->index(), currentValue);
                    }
                    break;
                }
                case MidiControlType::MomentarySwitch:
                {
                    IEffect *pEffect = this->realtimeEffects[mapping.effectIndex];
                    pEffect->SetControl(mapping.controlIndex, range != 0 ? mapping.pPortInfo->max_value() : mapping.pPortInfo->min_value());
                    // do NOT notify anyone!
                }
                break;

                case MidiControlType::Select:
                case MidiControlType::Dial:
                {
                    IEffect *pEffect = this->realtimeEffects[mapping.effectIndex];
                    range = mapping.midiBinding.adjustRange(range);
                    float currentValue = mapping.pPortInfo->rangeToValue(range);
                    if (pEffect->GetControlValue(mapping.controlIndex) != currentValue)
                    {
                        this->SetControlValue(mapping.effectIndex, mapping.controlIndex, currentValue);
                        pfnCallback(callbackHandle, mapping.instanceId, mapping.pPortInfo->index(), currentValue);
                    }
                    break;
                }
                case MidiControlType::None:
                default:
                    break;
                }
            }
        }
    }
}