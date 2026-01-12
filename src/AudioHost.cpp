// Copyright (c) Robin E. R. Davies
// Copyright (c) Gabriel Hernandez
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

#include "AudioHost.hpp"
#include "util.hpp"
#include <lv2/atom/atom.h>
#include "SchedulerPriority.hpp"
#include "AlsaSequencer.hpp"

#include "Lv2Log.hpp"

#include "SchedulerPriority.hpp"
#include "JackDriver.hpp"
#include "AlsaDriver.hpp"
#include "DummyAudioDriver.hpp"
#include "AtomConverter.hpp"
#include <unordered_map>
#include "PluginHost.hpp"
#include "PatchPropertyWriter.hpp"
#include "CpuTemperatureMonitor.hpp"
#include "restrict.hpp"

using namespace pipedal;

#include "AudioConfig.hpp"

#include <string.h>
#include <stdio.h>
#include <mutex>
#include <thread>
#include <semaphore.h>
#include "VuUpdate.hpp"
#include "CpuGovernor.hpp"

#include "RingBuffer.hpp"
#include "RingBufferReader.hpp"

#include "PiPedalException.hpp"
#include "pthread.h"
#include "sched.h"
#include <cmath>
#include <chrono>
#include <fstream>
#include "Lv2EventBufferWriter.hpp"
#include "InheritPriorityMutex.hpp"
#include <atomic>

#ifdef __linux__
#include <sched.h>
#include <sys/syscall.h>
#include <unistd.h>
#endif

#define JACK_SESSION_CALLBACKS 0

#include "AdminClient.hpp"

const double VU_UPDATE_RATE_S = 1.0 / 30;
const double OVERRUN_GRACE_PERIOD_S = 15;
using namespace pipedal;

const int MIDI_LV2_BUFFER_SIZE = 16 * 1024;

static void GetCpuFrequency(uint64_t *freqMin, uint64_t *freqMax)
{
    uint64_t fMax = 0;
    uint64_t fMin = UINT64_MAX;
    char deviceName[128];
    try
    {
        for (int i = 0; true; ++i)
        {

            snprintf(deviceName, sizeof(deviceName), "/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq", i);
            std::ifstream f(deviceName);
            if (!f)
            {
                break;
            }
            uint64_t freq;
            f >> freq;
            if (!f)
                break;
            if (freq < fMin)
                fMin = freq;
            if (freq > fMax)
                fMax = freq;
        }
    }
    catch (const std::exception &)
    {
    }
    if (fMax == 0)
        fMin = 0;
    *freqMin = fMin;
    *freqMax = fMax;
}
static std::string GetGovernor()
{
    return pipedal::GetCpuGovernor();
}

namespace pipedal
{

    struct PathPatchProperty
    {
        LV2_URID propertyUrid = 0;
        std::vector<uint8_t> atomBuffer;
    };

    class IndexedSnapshotValue
    {
    private:
        struct InputControlEntry
        {
            bool isInputControl = false;
            float value = 0;
        };

    public:
        IndexedSnapshotValue(IEffect *effect, SnapshotValue *snapshotValue, PluginHost &pluginHost)
            : pEffect(effect)
        {
            auto maxInputControl = effect->GetMaxInputControl();
            inputControlValues.resize(maxInputControl);
            this->enabled = true;
            for (uint64_t i = 0; i < maxInputControl; ++i)
            {
                bool isInputControl = effect->IsInputControl(i);
                if (isInputControl)
                {
                    inputControlValues[i] = InputControlEntry{isInputControl : true, value : effect->GetDefaultInputControlValue(i)};
                }
                else
                {
                    inputControlValues[i] = InputControlEntry{isInputControl : false, value : 0};
                }
            }
            if (snapshotValue)
            {
                this->enabled = snapshotValue->isEnabled_;
                for (auto &controlValue : snapshotValue->controlValues_)
                {
                    auto index = effect->GetControlIndex(controlValue.key());
                    if (index >= 0 && index < inputControlValues.size())
                    {
                        inputControlValues[index].value = controlValue.value();
                    }
                }
                this->lv2State = snapshotValue->lv2State_;

                if (effect->IsLv2Effect())
                {
                    Lv2Effect *lv2Effect = (Lv2Effect *)effect;
                    for (auto &pathProperty : snapshotValue->pathProperties_)
                    {
                        // only transmit changed path patch properties.
                        if (lv2Effect->GetPathPatchProperty(pathProperty.first) != pathProperty.second)
                        {
                            lv2Effect->SetPathPatchProperty(pathProperty.first, pathProperty.second);

                            PathPatchProperty pathPatchProperty;
                            pathPatchProperty.propertyUrid = pluginHost.GetLv2Urid(pathProperty.first.c_str());
                            // convert to json variant so we do a mappath operation.
                            json_variant vProperty;
                            std::istringstream ss(pathProperty.second);
                            json_reader reader(ss);
                            reader.read(&vProperty);
                            if (!vProperty.is_null())
                            {
                                try
                                {
                                    vProperty = pluginHost.MapPath(vProperty);

                                    // now to atom format (what we want on the rt thread0)
                                    AtomConverter atomConverter(pluginHost.GetMapFeature());
                                    LV2_Atom *atomValue = atomConverter.ToAtom(vProperty);

                                    size_t atomBufferSize = (atomValue->size + sizeof(LV2_Atom) + 3) / 4 * 4;
                                    pathPatchProperty.atomBuffer.resize(atomValue->size + sizeof(LV2_Atom));
                                    memcpy(pathPatchProperty.atomBuffer.data(), atomValue, pathPatchProperty.atomBuffer.size());
                                    this->pathPatchProperties.push_back(std::move(pathPatchProperty));
                                }
                                catch (const std::exception &e)
                                {
                                    Lv2Log::info(SS("IndexedSnapshotValue: Failed to map path property " << pathProperty.first << ". " << e.what()));
                                }
                            }
                        }
                    }
                }
            }
        }
        void ApplyValues(IEffect *effect)
        {

            effect->SetBypass(this->enabled);
            if (effect != pEffect)
            {
                throw std::runtime_error("Wrong effect");
            }
            for (size_t i = 0; i < inputControlValues.size(); ++i)
            {
                InputControlEntry &e = inputControlValues[i];
                if (e.isInputControl)
                {
                    effect->SetControl((int)i, e.value);
                }
            }

            for (const auto &patchProperty : pathPatchProperties)
            {
                // yyy: only if the property changed!.
                effect->SetPatchProperty(
                    patchProperty.propertyUrid, patchProperty.atomBuffer.size(), (LV2_Atom *)patchProperty.atomBuffer.data());
            }
            // effect->SetLv2State(lv2State);
        }

    private:
        IEffect *pEffect;
        Lv2PluginState lv2State;
        bool enabled;
        std::vector<InputControlEntry> inputControlValues;
        std::vector<PathPatchProperty> pathPatchProperties;
    };
    class IndexedSnapshot
    {
    public:
        IndexedSnapshot(Snapshot *snapshot, std::shared_ptr<Lv2Pedalboard> currentPedalboard, PluginHost &pluginHost)
        {
            std::unordered_map<uint64_t, SnapshotValue *> index;
            for (auto &value : snapshot->values_)
            {
                index[value.instanceId_] = &value;
            }
            std::vector<IEffect *> &effects = currentPedalboard->GetEffects();
            snapshotValues.reserve(effects.size());

            for (size_t i = 0; i < effects.size(); ++i)
            {
                auto &effect = effects[i];

                SnapshotValue *snapshotValue = getSnapshotValue(index, effect->GetInstanceId());
                snapshotValues.push_back(IndexedSnapshotValue(effect, snapshotValue, pluginHost));
            }
        }
        static SnapshotValue *getSnapshotValue(std::unordered_map<uint64_t, SnapshotValue *> &index, uint64_t instanceId)
        {
            auto iter = index.find(instanceId);
            if (iter == index.end())
            {
                return nullptr;
            }
            return iter->second;
        }

        void Apply(std::vector<IEffect *> &effects)
        {
            if (effects.size() != snapshotValues.size())
            {
                throw std::runtime_error("Effects and values don't match");
            }
            for (size_t i = 0; i < snapshotValues.size(); ++i)
            {
                snapshotValues[i].ApplyValues(effects[i]);
            }
        }
        ~IndexedSnapshot()
        {
        }

    private:
        std::vector<IndexedSnapshotValue> snapshotValues;
    };
}

class SystemMidiBinding
{
private:
    MidiBinding currentBinding;
    bool controlState = false;
    uint8_t lastControlValue = 0;

public:
    void SetBinding(const MidiBinding &binding)
    {
        currentBinding = binding;
        controlState = false;
        lastControlValue = 0;
    }

    bool IsMatch(const MidiEvent &event);
    bool IsTriggered(const MidiEvent &event);
};

bool SystemMidiBinding::IsTriggered(const MidiEvent &event)
{
    if (!IsMatch(event))
    {
        return false;
    }
    switch (currentBinding.bindingType())
    {
    case BINDING_TYPE_NOTE:
    {
        if (event.size != 3)
            return false;
        auto command = event.buffer[0] & 0xF0;
        if (command != 0x90)
            return false; // MIDI note on
        if (event.buffer[1] != currentBinding.note())
            return false;
        return event.buffer[2] != 0; // note-on with velicity of zero is a note-off.
    }
    break;
    case BINDING_TYPE_CONTROL:
    {
        if (event.size != 3)
            return false;
        auto command = event.buffer[0] & 0xF0;
        if (command != 0xB0)
            return false;
        if (event.buffer[1] != currentBinding.control())
            return false;

        uint8_t value = event.buffer[2];
        bool result = false;
        if (currentBinding.switchControlType() == SwitchControlTypeT::TRIGGER_ON_RISING_EDGE)
        {
            result = value >= 0x64 && lastControlValue < 0x64;
        } else if (currentBinding.switchControlType() == SwitchControlTypeT::TRIGGER_ON_ANY)
        {
            result = true;
        }
        lastControlValue = value;
        return result;
    }
    break;
    default:
        return false;
    }
}

bool SystemMidiBinding::IsMatch(const MidiEvent &event)
{
    switch (currentBinding.bindingType())
    {
    case BINDING_TYPE_NOTE:
    case BINDING_TYPE_TAP_TEMPO:
    {
        if (event.size != 3)
            return false;
        auto command = event.buffer[0] & 0xF0;
        if (command != 0x80 && command != 0x90)
            return false; // note on/note off.
        return event.buffer[1] == currentBinding.note();
    }
    break;
    case BINDING_TYPE_CONTROL:
    {
        if (event.size != 3)
            return false;
        auto command = event.buffer[0] & 0xF0;
        if (command != 0xB0)
            return false;
        return event.buffer[1] == currentBinding.control();
    }
    break;
    default:
        return false;
    }
}

class AudioHostImpl : public AudioHost, private AudioDriverHost, private IPatchWriterCallback
{
private:
    AlsaSequencer::ptr alsaSequencer;
    AlsaSequencerDeviceMonitor::ptr alsaDeviceMonitor;

    void OnWritePatchPropertyBuffer(
        PatchPropertyWriter::Buffer *);

    IHost *pHost = nullptr;
    LV2_Atom_Forge inputWriterForge;

    std::mutex atomConverterMutex;
    AtomConverter atomConverter;

    CpuTemperatureMonitor::ptr cpuTemperatureMonitor;
    static constexpr size_t DEFERRED_MIDI_BUFFER_SIZE = 1024;

    uint8_t deferredMidiMessages[DEFERRED_MIDI_BUFFER_SIZE];
    size_t deferredMidiMessageCount = 0;
    bool midiProgramChangePending = false;
    bool midiSnapshotRequestPending = false;
    int64_t snapshotRequestId = 0;

    int selectedBank = -1;
    int64_t midiProgramChangeId = 0;

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
            // patch_accept = pHost->GetLv2Urid(LV2_PATCH__accept);
            patch_value = pHost->GetLv2Urid(LV2_PATCH__value);
            unitsFrame = pHost->GetLv2Urid(LV2_UNITS__frame);
        }
        // LV2_URID patch_accept;

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

    std::unique_ptr<AudioDriver> audioDriver;

    std::recursive_mutex mutex;
    int64_t overrunGracePeriodSamples = 0;

    IAudioHostCallbacks *pNotifyCallbacks = nullptr;

    virtual void SetNotificationCallbacks(IAudioHostCallbacks *pNotifyCallbacks)
    {
        this->pNotifyCallbacks = pNotifyCallbacks;
    }

    const size_t RING_BUFFER_SIZE = 64 * 1024;

    RingBuffer<true, false> inputRingBuffer;
    RingBuffer<false, true> outputRingBuffer;

    RingBufferWriter<true, false> x;

    RealtimeRingBufferReader realtimeReader;
    RealtimeRingBufferWriter realtimeWriter;
    HostRingBufferReader hostReader;
    HostRingBufferWriter hostWriter;

    SystemMidiBinding prevBankMidiBinding;
    SystemMidiBinding nextBankMidiBinding;
    SystemMidiBinding nextPresetMidiBinding;
    SystemMidiBinding prevPresetMidiBinding;

    SystemMidiBinding snapshot1MidiBinding;
    SystemMidiBinding snapshot2MidiBinding;
    SystemMidiBinding snapshot3MidiBinding;
    SystemMidiBinding snapshot4MidiBinding;
    SystemMidiBinding snapshot5MidiBinding;
    SystemMidiBinding snapshot6MidiBinding;

    SystemMidiBinding startHotspotMidiBinding;
    SystemMidiBinding stopHotspotMidiBinding;
    SystemMidiBinding rebootMidiBinding;
    SystemMidiBinding shutdownMidiBinding;

    JackChannelSelection channelSelection;
    std::atomic<bool> active = false;
    std::atomic<bool> audioStopped = false;
    std::atomic<bool> isDummyAudioDriver = false;

    std::shared_ptr<Lv2Pedalboard> currentPedalboard;
    std::vector<std::shared_ptr<Lv2Pedalboard>> activePedalboards; // pedalboards that have been sent to the audio queue.
    Lv2Pedalboard *realtimeActivePedalboard = nullptr;

    uint32_t sampleRate = 0;
    uint64_t currentSample = 0;

    std::atomic<uint64_t> underruns = 0;
    std::atomic<std::chrono::system_clock::time_point> lastUnderrunTime =
        std::chrono::system_clock::from_time_t(0);

    std::string GetAtomObjectType(uint8_t *pData)
    {
    LV2_Atom_Object *pAtom = (LV2_Atom_Object *)pData;
        if (pAtom->atom.type != uris.atom_Object)
        {
            throw std::invalid_argument("Not an Lv2 Object");
        }
        return pHost->Lv2UridToString(pAtom->body.otype);
    }

    virtual std::string AtomToJson(const LV2_Atom *pAtom) override
    {
        std::lock_guard lock(atomConverterMutex);
        json_variant vAtom = atomConverter.ToJson(pAtom);

        std::stringstream s;
        json_writer writer(s);

        writer.write(vAtom);
        return s.str();
    }

    virtual void OnUnderrun()
    {
        ++this->underruns;
        this->lastUnderrunTime = std::chrono::system_clock ::now();
    }

    virtual void Close()
    {
        {
            std::lock_guard guard{mutex};
            if (!isOpen)
                return;

            isOpen = false;
        }

        if (active)
        {
            audioDriver->Deactivate();

            active = false;
        }

        audioDriver->Close();

        StopReaderThread();

        // delete any leaked snapshots.
        CleanUpSnapshots();

        // release any pdealboards owned by the process thread.
        this->activePedalboards.resize(0);
        this->realtimeActivePedalboard = nullptr;

        // clean up any realtime buffers that may have been lost in transit.
        // TODO: These should be lists, really. There may be multiple items in flight..
        if (realtimeVuBuffers != nullptr)
        {
            delete realtimeVuBuffers;
            realtimeVuBuffers = nullptr;
        }
        if (realtimeMonitorPortSubscriptions != nullptr)
        {
            delete realtimeMonitorPortSubscriptions;
            realtimeMonitorPortSubscriptions = nullptr;
        }
        this->inputRingBuffer.reset();
        this->outputRingBuffer.reset();

        audioDriver = nullptr;
    }

    void ZeroBuffer(float *buffer, size_t nframes)
    {
        for (size_t i = 0; i < nframes; ++i)
        {
            buffer[i] = 0;
        }
    }
    Lv2EventBufferUrids eventBufferUrids;

    void ZeroOutputBuffers(size_t nframes)
    {
        for (size_t i = 0; i < audioDriver->OutputBufferCount(); ++i)
        {
            float * out = (float *)audioDriver->GetOutputBuffer(i);
            if (out)
            {
                ZeroBuffer(out, nframes);
            }
        }
    }
    RealtimeVuBuffers *realtimeVuBuffers = nullptr;
    size_t vuSamplesPerUpdate = 0;
    int64_t vuSamplesRemaining = 0;

    void freeRealtimeVuConfiguration()
    {
        if (this->realtimeVuBuffers != nullptr)
        {
            realtimeWriter.FreeVuSubscriptions(this->realtimeVuBuffers);

            this->realtimeVuBuffers = nullptr;
        }
    }

    RealtimeMonitorPortSubscriptions *realtimeMonitorPortSubscriptions = nullptr;

    void freeRealtimeMonitorPortSubscriptions()
    {
        if (this->realtimeMonitorPortSubscriptions != nullptr)
        {
            realtimeWriter.FreeMonitorPortSubscriptions(this->realtimeMonitorPortSubscriptions);

            this->realtimeMonitorPortSubscriptions = nullptr;
        }
    }

    virtual void SetSystemMidiBindings(const std::vector<MidiBinding> &bindings);

    void writeVu()
    {
        // throttling: we send one; but won't send another until the host thread
        // acknowledges receipt.

        if (!realtimeVuBuffers->waitingForAcknowledge)
        {
            auto pResult = realtimeVuBuffers->GetResult(currentSample);

            this->realtimeWriter.SendVuUpdate(pResult);
            realtimeVuBuffers->waitingForAcknowledge = true;
        }
    }

    void processMonitorPortSubscriptions(uint32_t nframes)
    {
        for (size_t i = 0; i < this->realtimeMonitorPortSubscriptions->subscriptions.size(); ++i)
        {
            auto &portSubscription = realtimeMonitorPortSubscriptions->subscriptions[i];

            portSubscription.samplesToNextCallback -= portSubscription.sampleRate;
            if (portSubscription.samplesToNextCallback < 0)
            {
                portSubscription.samplesToNextCallback += portSubscription.sampleRate;
                if (!portSubscription.waitingForAck)
                {
                    float value = realtimeActivePedalboard->GetControlOutputValue(
                        portSubscription.instanceIndex,
                        portSubscription.portIndex);
                    if (value != portSubscription.lastValue)
                    {
                        portSubscription.waitingForAck = true;
                        portSubscription.lastValue = value;
                        this->realtimeWriter.SendMonitorPortUpdate(
                            portSubscription.callbackPtr,
                            portSubscription.subscriptionHandle,
                            value);
                    }
                }
            }
        }
    }

    RealtimePatchPropertyRequest *pParameterRequests = nullptr;

    void cancelParameterRequests()
    {
        return;

        // auto p = this->pParameterRequests;
        // while (pParameterRequests != nullptr)
        // {
        //     auto nextParameterRequest = p->pNext;
        //     if (p->requestType ==  RealtimePatchPropertyRequest::RequestType::PatchGet) {
        //         p->errorMessage = "Effect unloaded.";
        //     }
        //     p = nextParameterRequest;
        // }
        // this->realtimeWriter.ParameterRequestComplete(pParameterRequests);
        // this->pParameterRequests = nullptr;
    }

    bool reEntered = false;
    // returns false if the current effect has been replaced.
    bool ProcessInputCommands()
    {
        if (reEntered)
        {
            throw PiPedalStateException("Rentry of process command.");
        }
        reEntered = true;

        while (true)
        {
            RingBufferCommand command;
            size_t space = realtimeReader.readSpace();
            if (space <= sizeof(RingBufferCommand)) // RingBufferCommand + at least 1 more byte for the body.
                break;
            if (!realtimeReader.read(&command))
            {
                break;
            }
            switch (command)
            {
            case RingBufferCommand::SetValue:
            {
                SetControlValueBody body;
                realtimeReader.readComplete(&body);
                this->realtimeActivePedalboard->SetControlValue(body.effectIndex, body.controlIndex, body.value);
                break;
            }
            case RingBufferCommand::SetInputVolume:
            {
                SetVolumeBody body;
                realtimeReader.readComplete(&body);
                this->realtimeActivePedalboard->SetInputVolume(body.value);
                break;
            }
            case RingBufferCommand::SetOutputVolume:
            {
                SetVolumeBody body;
                realtimeReader.readComplete(&body);
                this->realtimeActivePedalboard->SetOutputVolume(body.value);
                break;
            }
            case RingBufferCommand::ParameterRequest:
            {
                RealtimePatchPropertyRequest *pRequest = nullptr;
                realtimeReader.readComplete(&pRequest);

                // link to the list of parameter requests.
                pRequest->pNext = pParameterRequests;
                pParameterRequests = pRequest;
                break;
            }
            case RingBufferCommand::AckVuUpdate:
            {
                bool dummy;
                realtimeReader.readComplete(&dummy);
                if (this->realtimeVuBuffers)
                {
                    this->realtimeVuBuffers->waitingForAcknowledge = false;
                }

                break;
            }
            case RingBufferCommand::AckMonitorPortUpdate:
            {
                int64_t subscriptionHandle = 0;
                realtimeReader.readComplete(&subscriptionHandle);
                if (this->realtimeMonitorPortSubscriptions != nullptr)
                {
                    for (size_t i = 0; i < this->realtimeMonitorPortSubscriptions->subscriptions.size(); ++i)
                    {
                        if (this->realtimeMonitorPortSubscriptions->subscriptions[i].subscriptionHandle == subscriptionHandle)
                        {
                            this->realtimeMonitorPortSubscriptions->subscriptions[i].waitingForAck = false;
                        }
                    }
                }
                break;
            }
            case RingBufferCommand::SetMonitorPortSubscription:
            {
                RealtimeMonitorPortSubscriptions *pSubscriptions;
                realtimeReader.readComplete(&pSubscriptions);
                this->freeRealtimeMonitorPortSubscriptions();
                this->realtimeMonitorPortSubscriptions = pSubscriptions;
                break;
            }
            case RingBufferCommand::AckMidiProgramChange:
            {
                int64_t requestId;
                realtimeReader.readComplete(&requestId);
                if (requestId == this->midiProgramChangeId)
                {
                    this->midiProgramChangePending = false;
                }
                break;
            }
            case RingBufferCommand::AckMidiSnapshotRequest:
            {
                int64_t requestId;
                realtimeReader.readComplete(&requestId);
                if (requestId == this->snapshotRequestId)
                {
                    this->midiSnapshotRequestPending = false;
                }
                break;
            }
            case RingBufferCommand::LoadSnapshot:
            {
                IndexedSnapshot *snapshot;
                realtimeReader.readComplete(&snapshot);
                ApplySnapshot(snapshot);
                realtimeWriter.FreeSnapshot(snapshot);
                break;
            }
            case RingBufferCommand::SetVuSubscriptions:
            {
                RealtimeVuBuffers *configuration;
                realtimeReader.readComplete(&configuration);
                this->freeRealtimeVuConfiguration();
                this->realtimeVuBuffers = configuration;
                if (this->realtimeVuBuffers != nullptr)
                {
                    this->realtimeVuBuffers->waitingForAcknowledge = false;
                }
                vuSamplesRemaining = vuSamplesPerUpdate;

                break;
            }
            case RingBufferCommand::SetBypass:
            {
                SetBypassBody body;
                realtimeReader.readComplete(&body);
                this->realtimeActivePedalboard->SetBypass(body.effectIndex, body.enabled);
                break;
            }
            case RingBufferCommand::ReplaceEffect:
            {
                ReplaceEffectBody body;
                realtimeReader.readComplete(&body);

                if (body.effect != nullptr)
                {
                    auto oldValue = this->realtimeActivePedalboard;
                    this->realtimeActivePedalboard = body.effect;

                    realtimeWriter.EffectReplaced(oldValue);

                    // invalidate the possibly no-good subscriptions. Model will update them shortly.
                    freeRealtimeVuConfiguration();
                    freeRealtimeMonitorPortSubscriptions();
                    cancelParameterRequests();

                    if (realtimeActivePedalboard)
                    {
                        realtimeActivePedalboard->ResetAtomBuffers();
                        // issue patch gets for all writable path properties.
                        for (auto pEffect : realtimeActivePedalboard->GetEffects())
                        {
                            pEffect->RequestAllPathPatchProperties();
                        }
                    }
                    realtimeActivePedalboard->UpdateAudioPorts();
                }
                reEntered = false;

                return false; // signal to caller that the effect has been replaced, and processing needs to start again.
            }
            default:
                throw PiPedalStateException("Unknown Ringbuffer command.");
            }
        }
        reEntered = false;
        return true;
    }

    void ApplySnapshot(IndexedSnapshot *snapshot)
    {
        auto &effects = this->realtimeActivePedalboard->GetEffects();
        snapshot->Apply(effects);
    }
    virtual void AckMidiProgramRequest(uint64_t requestId)
    {
        hostWriter.AckMidiProgramRequest(requestId);
    }
    virtual void AckSnapshotRequest(uint64_t snapshotRequestId)
    {
        hostWriter.AckMidiSnapshotRequest(snapshotRequestId);
    }

    void OnMidiValueChanged(uint64_t instanceId, int controlIndex, float value)
    {
        realtimeWriter.MidiValueChanged(instanceId, controlIndex, value);
    }
    static void fnMidiValueChanged(void *data, uint64_t instanceId, int controlIndex, float value)
    {
        ((AudioHostImpl *)data)->OnMidiValueChanged(instanceId, controlIndex, value);
    }
    static bool isBankChange(MidiEvent &event)
    {
        return (event.size == 3 && event.buffer[0] == 0xB0 && event.buffer[1] == 0x00);
    }

    bool onMidiEvent(Lv2EventBufferWriter &eventBufferWriter, Lv2EventBufferWriter::LV2_EvBuf_Iterator &iterator, MidiEvent &event)
    {
        // eventBufferWriter.writeMidiEvent(iterator, 0, event.size, event.buffer);

        this->realtimeActivePedalboard->OnMidiMessage(
            event, this, fnMidiValueChanged);
        if (listenForMidiEvent)
        {
            if (event.size >= 3)
            {
                uint8_t cmd = (uint8_t)(event.buffer[0] & 0xF0);
                bool isNote = cmd == 0x90 && event.buffer[2] != 0; // note on with velocity > 0.
                bool isControl = cmd == 0xB0;
                if (isControl)
                {
                    // ignore bank select, and CC LSB values.
                    uint8_t cc1 = (uint8_t)(event.buffer[1]);
                    if (cc1 == 0 || (cc1 >= 32 && cc1 < 64))
                    {
                        isControl = false;
                    }
                }
                if (isNote || isControl)
                {
                    MidiNotifyBody notifyBody (event.buffer[0],event.buffer[1], event.buffer[2]);
                    realtimeWriter.OnMidiListen(notifyBody);
                }
            }
        }
        return true;
    }

    void OnSnapshotTriggered(int snapshotIndex)
    {
        // midiProgramChangePending = true;
        this->midiSnapshotRequestPending = true;
        this->realtimeWriter.OnRealtimeMidiSnapshotRequest(snapshotIndex, ++snapshotRequestId);
    }

    void ProcessMidiEvent(Lv2EventBufferWriter &eventBufferWriter, Lv2EventBufferWriter::LV2_EvBuf_Iterator &iterator, MidiEvent &event)
    {
        uint8_t midiCommand = (uint8_t)(event.buffer[0] & 0xF0);
        if (midiCommand == 0xC0) // midi program change.
        {
            this->deferredMidiMessageCount = 0; // we can discard previous control changes.
            midiProgramChangePending = true;

            this->realtimeWriter.OnMidiProgramChange(++(this->midiProgramChangeId), selectedBank, event.buffer[1]);
        }
        else if (isBankChange(event))
        {
            this->selectedBank = event.buffer[2];
        }
        else if (this->nextBankMidiBinding.IsTriggered(event))
        {
            this->deferredMidiMessageCount = 0; // we can discard previous control changes.
            midiProgramChangePending = true;

            this->realtimeWriter.OnNextMidiBank(++(this->midiProgramChangeId), 1);
        }
        else if (this->prevBankMidiBinding.IsTriggered(event))
        {
            this->deferredMidiMessageCount = 0; // we can discard previous control changes.
            midiProgramChangePending = true;

            this->realtimeWriter.OnNextMidiBank(++(this->midiProgramChangeId), -1);
        }
        else if (nextPresetMidiBinding.IsTriggered(event))
        {
            this->deferredMidiMessageCount = 0; // we can discard previous control changes.
            midiProgramChangePending = true;

            midiProgramChangePending = true;
            this->realtimeWriter.OnNextMidiProgram(++(this->midiProgramChangeId), 1);
        }
        else if (prevPresetMidiBinding.IsTriggered(event))
        {
            this->deferredMidiMessageCount = 0; // we can discard previous control changes.
            midiProgramChangePending = true;
            this->realtimeWriter.OnNextMidiProgram(++(this->midiProgramChangeId), -1);
        }

        else if (shutdownMidiBinding.IsTriggered(event))
        {
            this->realtimeWriter.OnRealtimeMidiEvent(RealtimeMidiEventType::Shutdown);
        }
        if (rebootMidiBinding.IsTriggered(event))
        {
            this->realtimeWriter.OnRealtimeMidiEvent(RealtimeMidiEventType::Reboot);
        }
        if (startHotspotMidiBinding.IsTriggered(event))
        {
            this->realtimeWriter.OnRealtimeMidiEvent(RealtimeMidiEventType::StartHotspot);
        }
        if (stopHotspotMidiBinding.IsTriggered(event))
        {
            this->realtimeWriter.OnRealtimeMidiEvent(RealtimeMidiEventType::StopHotspot);
        }
        else if (midiProgramChangePending)
        {
            // defer the message for processing after the program change has completed discarding messages that don't fit in our buffer.
            if (event.size > 0 && event.size < 128 && event.size + 2 + this->deferredMidiMessageCount < DEFERRED_MIDI_BUFFER_SIZE)
            {
                this->deferredMidiMessages[deferredMidiMessageCount++] = (uint8_t)event.size;
                for (size_t i = 0; i < event.size; ++i)
                {
                    this->deferredMidiMessages[deferredMidiMessageCount++] = event.buffer[i];
                }
            }
            return;
        }
        else if (this->snapshot1MidiBinding.IsTriggered(event))
        {
            OnSnapshotTriggered(0);
        }
        else if (this->snapshot2MidiBinding.IsTriggered(event))
        {
            OnSnapshotTriggered(1);
        }
        else if (this->snapshot3MidiBinding.IsTriggered(event))
        {
            OnSnapshotTriggered(2);
        }
        else if (this->snapshot4MidiBinding.IsTriggered(event))
        {
            OnSnapshotTriggered(3);
        }
        else if (this->snapshot5MidiBinding.IsTriggered(event))
        {
            OnSnapshotTriggered(4);
        }
        else if (this->snapshot6MidiBinding.IsTriggered(event))
        {
            OnSnapshotTriggered(5);
        }
        else
        {
            onMidiEvent(eventBufferWriter, iterator, event);
        }
    }

    void ProcessDeferredMidiMessages(
        Lv2EventBufferWriter eventBufferWriter,
        Lv2EventBufferWriter::LV2_EvBuf_Iterator &iterator)
    {
        MidiEvent event;

        // write all deferred midi messages.
        if (!midiProgramChangePending && !midiSnapshotRequestPending)
        {
            for (size_t i = 0; i < deferredMidiMessageCount; /**/)
            {
                int8_t deviceIndex = deferredMidiMessages[i++];
                int8_t messageCount = deferredMidiMessages[i++];
                event.size = messageCount;
                event.buffer = deferredMidiMessages + i;
                event.frame = 0;

                ProcessMidiEvent(eventBufferWriter, iterator, event);

                if (midiProgramChangePending)
                {
                    break;
                }
                i += messageCount;

                if (midiSnapshotRequestPending)
                {
                    // consume what has been written so far, leaving what follows for future processing.
                    size_t remaining = deferredMidiMessageCount - i;
                    for (size_t j = 0; j < remaining; ++i)
                    {
                        deferredMidiMessages[j] = deferredMidiMessages[i + j];
                    }
                    deferredMidiMessageCount = remaining;
                    break;
                }
            }
        }
    }
    void ProcessMidiInput()
    {
        Lv2EventBufferWriter eventBufferWriter(this->eventBufferUrids);
        Lv2EventBufferWriter::LV2_EvBuf_Iterator iterator = eventBufferWriter.begin();

        ProcessDeferredMidiMessages(eventBufferWriter, iterator);

        {
            size_t n = audioDriver->GetMidiInputEventCount();
            MidiEvent *events = audioDriver->GetMidiEvents();

            for (size_t i = 0; i < n; ++i)
            {
                ProcessMidiEvent(eventBufferWriter, iterator, events[i]);
            }
        }
    }

#define RESET_XRUN_SAMPLES 22050ul // 1/2 a second-ish.

    std::mutex audioStoppedMutex;

    bool IsAudioRunning()
    {
        return this->active && (!this->audioStopped) && !this->isDummyAudioDriver;
    }
    virtual void OnAlsaDriverStopped() override
    {

        this->audioStopped = true;
        Lv2Log::info("Audio stopped.");
        this->realtimeWriter.AudioTerminatedAbnormally();
    }
    virtual void OnAudioTerminated() override
    {
        this->active = false;
        Lv2Log::info("Audio thread terminated.");
    }

    virtual void OnProcess(size_t nframes)
    {
        try
        {
            float * restrict in , * restrict out;

            Lv2Pedalboard *pedalboard = nullptr;
            pedalboard = this->realtimeActivePedalboard;
            if (pedalboard)
            {
                pedalboard->ResetAtomBuffers();
            }
            while (true)
            {

                if (ProcessInputCommands())
                {
                    break;
                }
                // else a new pedalboard was installed. start again.
                pedalboard = this->realtimeActivePedalboard;
            }

            bool processed = false;

            if (pedalboard != nullptr)
            {
                ProcessMidiInput();
                float *inputBuffers[4];
                float *outputBuffers[4];
                bool buffersValid = true;
                for (int i = 0; i < audioDriver->InputBufferCount(); ++i)
                {
                    float *input = (float *)audioDriver->GetInputBuffer(i);
                    if (input == nullptr)
                    {
                        buffersValid = false;
                        break;
                    }
                    inputBuffers[i] = input;
                }
                inputBuffers[audioDriver->InputBufferCount()] = nullptr;

                for (int i = 0; i < audioDriver->OutputBufferCount(); ++i)
                {
                    float *output = audioDriver->GetOutputBuffer(i);
                    if (output == nullptr)
                    {
                        buffersValid = false;
                        break;
                    }
                    outputBuffers[i] = output;
                }
                outputBuffers[audioDriver->OutputBufferCount()] = nullptr;

                if (buffersValid)
                {
                    pedalboard->ProcessParameterRequests(pParameterRequests,nframes);

                    processed = pedalboard->Run(inputBuffers, outputBuffers, (uint32_t)nframes, &realtimeWriter);
                    if (processed)
                    {
                        if (this->realtimeVuBuffers != nullptr)
                        {
                            pedalboard->ComputeVus(this->realtimeVuBuffers, (uint32_t)nframes, inputBuffers, outputBuffers);

                            vuSamplesRemaining -= nframes;
                            if (vuSamplesRemaining <= 0)
                            {
                                writeVu();
                                vuSamplesRemaining += vuSamplesPerUpdate;
                            }
                        }
                        if (this->realtimeMonitorPortSubscriptions != nullptr)
                        {
                            processMonitorPortSubscriptions(nframes);
                        }
                    }
                    pedalboard->GatherPatchProperties(pParameterRequests);
                    pedalboard->GatherPathPatchProperties(this);
                }
            }

            if (!processed)
            {
                ZeroOutputBuffers(nframes);
            }

            if (pParameterRequests != nullptr)
            {
                this->realtimeWriter.ParameterRequestComplete(pParameterRequests);
                pParameterRequests = nullptr;
            }
            // provide a grace period for undderruns, while spinning up. (15 second-ish)
            if (currentSample <= this->overrunGracePeriodSamples && currentSample + nframes > this->overrunGracePeriodSamples)
            {
                this->underruns = 0;
            }
            this->currentSample += nframes;
        }
        catch (const std::exception &e)
        {
            Lv2Log::error("Fatal error while processing realtime audio. (%s)", e.what());
            throw;
        }
    }

public:
    AudioHostImpl(IHost *pHost)
        : inputRingBuffer(RING_BUFFER_SIZE),
          outputRingBuffer(RING_BUFFER_SIZE),
          realtimeReader(&this->inputRingBuffer),
          realtimeWriter(&this->outputRingBuffer),
          hostReader(&this->outputRingBuffer),
          hostWriter(&this->inputRingBuffer),
          eventBufferUrids(pHost),
          pHost(pHost),
          uris(pHost),
          atomConverter(pHost->GetMapFeature())
    {
        realtimeAtomBuffer.resize(32 * 1024);
        lv2_atom_forge_init(&inputWriterForge, pHost->GetMapFeature().GetMap());

        cpuTemperatureMonitor = CpuTemperatureMonitor::Get();
        this->alsaSequencer = AlsaSequencer::Create();
        this->alsaDeviceMonitor = AlsaSequencerDeviceMonitor::Create();

        this->alsaDeviceMonitor->StartMonitoring(
            [this](
                AlsaSequencerDeviceMonitor::MonitorAction action, 
                int client,
                const std::string &clientName)            
                {
                    HandleAlsaSequencerDevicesChanged(action, client, clientName);
                }
            );
    }
    virtual ~AudioHostImpl()
    {
        alsaDeviceMonitor->StopMonitoring();
        Close();
        CleanRestartThreads(true);
        audioDriver = nullptr;
        this->alsaSequencer = nullptr;
    }

    virtual JackConfiguration GetServerConfiguration()
    {
        JackConfiguration result;

        result.JackInitialize();
        return result;
    }

    virtual uint32_t GetSampleRate()
    {
        return this->sampleRate;
    }
    void HandleAlsaSequencerDevicesChanged(
        AlsaSequencerDeviceMonitor::MonitorAction action, int client, const std::string &clientName)
    {
        if (pNotifyCallbacks)
        {
            if (action == AlsaSequencerDeviceMonitor::MonitorAction::DeviceRemoved)
            {
                pNotifyCallbacks->OnAlsaSequencerDeviceRemoved(client);
            }
            else if (action == AlsaSequencerDeviceMonitor::MonitorAction::DeviceAdded)
            {
                pNotifyCallbacks->OnAlsaSequencerDeviceAdded(client,clientName);
            }
        }
    }   
    void HandleAudioTerminatedAbnormally()
    {
        Lv2Log::error("Audio processing terminated unexpectedly.");

        if (pNotifyCallbacks)
        {
            pNotifyCallbacks->OnAlsaDriverTerminatedAbnormally();
        }
    }
    std::vector<uint8_t> atomBuffer;
    std::vector<uint8_t> realtimeAtomBuffer;

    bool terminateThread;
    void ThreadProc()
    {
        SetThreadName("rtsvc");
        SetThreadPriority(SchedulerPriority::AudioService);

        int underrunMessagesGiven = 0;
        try
        {

            uint64_t lastUnderrunCount = this->underruns;

            using clock = std::chrono::steady_clock;
            using clock_time = std::chrono::steady_clock::time_point;
            using clock_duration = clock_time::duration;

            // check for overruns every 30 seconds.
            clock_duration waitPeriod =
                std::chrono::duration_cast<clock_duration>(std::chrono::seconds(30));
            clock_time waitTime = std::chrono::steady_clock::now();

            while (true)
            {

                // wait for an event.
                // 0 -> ready. -1: timed out. -2: closing.

                auto result = hostReader.wait_until(sizeof(RingBufferCommand), waitTime);
                if (result == RingBufferStatus::Closed)
                {
                    return;
                }
                else if (result == RingBufferStatus::TimedOut)
                {
                    // timeout.
                    if (underruns != lastUnderrunCount)
                    {
                        if (underrunMessagesGiven < 60) // limit how much log file clutter we generate.
                        {
                            Lv2Log::info("Audio underrun count: %lu", (unsigned long)underruns);
                            lastUnderrunCount = underruns;
                            ++underrunMessagesGiven;
                        }
                    }
                    waitTime += waitPeriod;
                }
                else
                {
                    while (true)
                    {
                        size_t space = hostReader.readSpace();
                        if (space <= sizeof(RingBufferCommand))
                        {
                            break;
                        }
                        RingBufferCommand command;
                        if (hostReader.read(&command))
                        {
                            if (command == RingBufferCommand::OnMidiListen)
                            {
                                MidiNotifyBody body;
                                hostReader.read(&body);
                                if (this->pNotifyCallbacks)
                                {
                                    pNotifyCallbacks->OnNotifyMidiListen(body.cc0_, body.cc1_, body.cc2_);
                                }
                            }
                            else if (command == RingBufferCommand::MidiValueChanged)
                            {
                                MidiValueChangedBody body;
                                hostReader.read(&body);

                                if (this->pNotifyCallbacks)
                                {
                                    this->pNotifyCallbacks->OnNotifyMidiValueChanged(body.instanceId, body.controlIndex, body.value);
                                }
                            }
                            else if (command == RingBufferCommand::ParameterRequestComplete)
                            {
                                RealtimePatchPropertyRequest *pRequest = nullptr;
                                hostReader.read(&pRequest);

                                std::shared_ptr<Lv2Pedalboard> currentpedalboard;

                                {
                                    std::lock_guard guard(mutex);
                                    currentPedalboard = this->currentPedalboard;
                                }

                                while (pRequest != nullptr)
                                {
                                    auto pNext = pRequest->pNext;
                                    if (pRequest->requestType == RealtimePatchPropertyRequest::RequestType::PatchGet)
                                    {
                                        if (pRequest->errorMessage == nullptr)
                                        {
                                            if (pRequest->GetSize() != 0)
                                            {
                                                IEffect *pEffect = currentPedalboard->GetEffect(pRequest->instanceId);
                                                if (pEffect == nullptr)
                                                {
                                                    pRequest->errorMessage = "Effect no longer available.";
                                                }
                                                else
                                                {
                                                    pRequest->jsonResponse = AtomToJson((LV2_Atom *)pRequest->GetBuffer());
                                                }
                                            }
                                            else
                                            {
                                                pRequest->errorMessage = "Plugin did not respond.";
                                            }
                                        }
                                    }
                                    if (pRequest->onPatchRequestComplete)
                                    {
                                        pRequest->onPatchRequestComplete(pRequest);
                                    }
                                    pRequest = pNext;
                                }
                            }
                            else if (command == RingBufferCommand::SendMonitorPortUpdate)
                            {
                                MonitorPortUpdate body;
                                hostReader.read(&body);

                                if (this->pNotifyCallbacks != nullptr)
                                {
                                    this->pNotifyCallbacks->OnNotifyMonitorPort(body);
                                }
                                this->hostWriter.AckMonitorPortUpdate(body.subscriptionHandle); // please sir, can I have some more?
                            }
                            else if (command == RingBufferCommand::SendVuUpdate)
                            {
                                const std::vector<VuUpdate> *updates = nullptr;
                                hostReader.read(&updates);

                                if (this->pNotifyCallbacks)
                                {
                                    this->pNotifyCallbacks->OnNotifyVusSubscription(*updates);
                                }
                                this->hostWriter.AckVuUpdate(); // please sir, can I have some more?
                            }
                            else if (command == RingBufferCommand::Lv2StateChanged)
                            {
                                uint64_t instanceId;
                                hostReader.read(&instanceId);
                                this->pNotifyCallbacks->OnNotifyLv2StateChanged(instanceId);
                            }
                            else if (command == RingBufferCommand::MaybeLv2StateChanged)
                            {
                                uint64_t instanceId;
                                hostReader.read(&instanceId);
                                this->pNotifyCallbacks->OnNotifyMaybeLv2StateChanged(instanceId);
                            }
                            else if (command == RingBufferCommand::AtomOutput)
                            {
                                uint64_t instanceId;
                                hostReader.read(&instanceId);
                                size_t extraBytes;
                                hostReader.read(&extraBytes);
                                if (atomBuffer.size() < extraBytes)
                                {
                                    atomBuffer.resize(extraBytes);
                                }
                                hostReader.read(extraBytes, &(atomBuffer[0]));

                                IEffect *pEffect = currentPedalboard->GetEffect(instanceId);
                                if (pEffect != nullptr && this->pNotifyCallbacks)
                                {
                                    LV2_Atom *atom = (LV2_Atom *)&atomBuffer[0];
                                    if (atom->type == uris.atom_Object)
                                    {
                                        LV2_Atom_Object *obj = (LV2_Atom_Object *)atom;
                                        if (obj->body.otype == uris.patch_Set)
                                        {

                                            // Get the property and value of the set message
                                            const LV2_Atom *property = nullptr;
                                            const LV2_Atom *value = nullptr;

                                            lv2_atom_object_get(
                                                obj,
                                                uris.patch_property, &property,
                                                uris.patch_value, &value,
                                                0);
                                            if (property != nullptr && value != nullptr && property->type == uris.atom_URID)
                                            {
                                                LV2_URID propertyUrid = ((LV2_Atom_URID *)property)->body;
                                                if (this->pNotifyCallbacks) 
                                                {
                                                    this->pNotifyCallbacks->OnPatchSetReply(instanceId, propertyUrid, value);
                                                }
                                            }
                                        }
                                    }
                                }
                            }
                            else if (command == RingBufferCommand::FreeVuSubscriptions)
                            {
                                RealtimeVuBuffers *config;
                                hostReader.read(&config);
                                delete config;
                            }
                            else if (command == RingBufferCommand::FreeMonitorPortSubscription)
                            {
                                RealtimeMonitorPortSubscriptions *pSubscriptions;
                                hostReader.read(&pSubscriptions);
                                delete pSubscriptions;
                            }
                            else if (command == RingBufferCommand::EffectReplaced)
                            {
                                EffectReplacedBody body;
                                hostReader.read(&body);
                                OnActivePedalboardReleased(body.oldEffect);
                            }
                            else if (command == RingBufferCommand::FreeSnapshot)
                            {
                                IndexedSnapshot *snapshot;
                                hostReader.read(&snapshot);
                                OnFreeSnapshot(snapshot);
                            }
                            else if (command == RingBufferCommand::SendPathPropertyBuffer)
                            {
                                PatchPropertyWriter::Buffer *buffer = nullptr;
                                hostReader.read(&buffer);
                                OnPathPropertyReceived(buffer);
                            }
                            else if (command == RingBufferCommand::AudioTerminatedAbnormally)
                            {
                                AudioStoppedBody body;
                                hostReader.read(&body);
                                HandleAudioTerminatedAbnormally();
                                return;
                            }
                            else if (command == RingBufferCommand::
                                                    MidiProgramChange)
                            {
                                RealtimeMidiProgramRequest programRequest;
                                hostReader.read(&programRequest);
                                OnMidiProgramRequest(programRequest);
                            }
                            else if (command == RingBufferCommand::NextMidiProgram)
                            {
                                RealtimeNextMidiProgramRequest request;
                                hostReader.read(&request);
                                pNotifyCallbacks->OnNotifyNextMidiProgram(request);
                            }
                            else if (command == RingBufferCommand::NextMidiBank)
                            {
                                RealtimeNextMidiProgramRequest request;
                                hostReader.read(&request);
                                pNotifyCallbacks->OnNotifyNextMidiBank(request);
                            }

                            else if (command == RingBufferCommand::RealtimeMidiEvent)
                            {
                                RealtimeMidiEventRequest request;
                                hostReader.read(&request);
                                pNotifyCallbacks->OnNotifyMidiRealtimeEvent(request.eventType);
                            }
                            else if (command == RingBufferCommand::RealtimeMidiSnapshotRequest)
                            {
                                RealtimeMidiSnapshotRequest request;
                                hostReader.read(&request);
                                pNotifyCallbacks->OnNotifyMidiRealtimeSnapshotRequest(
                                    request.snapshotIndex,
                                    request.snapshotRequestId);
                            }
                            else if (command == RingBufferCommand::Lv2ErrorMessage)
                            {
                                size_t size;
                                int64_t instanceId;
                                hostReader.read(&instanceId);
                                hostReader.read(&size);
                                if (this->atomBuffer.size() < size + 1)
                                {
                                    this->atomBuffer.resize(size + 1);
                                }
                                hostReader.read(size, &(atomBuffer[0]));
                                char *p = (char *)&(atomBuffer[0]);
                                p[size] = 0;
                                std::string message(p);
                                pNotifyCallbacks->OnNotifyLv2RealtimeError(instanceId, message);
                            }
                            else
                            {
                                throw PiPedalStateException("Unrecognized command received from audio thread.");
                            }
                        }
                    }
                }
            }
        }
        catch (const std::exception &e)
        {
            Lv2Log::error("Realtime response thread terminated abnormally. (%s)", e.what());
        }
    }

    std::thread *readerThread = nullptr;

    void StopReaderThread()
    {
        if (readerThread != nullptr)
        {
            this->terminateThread = true;
            this->outputRingBuffer.close();

            readerThread->join();
            delete readerThread;
            readerThread = nullptr;
        }
    }
    void StartReaderThread()
    {
        terminateThread = false;
        auto f = [this]()
        {
            this->ThreadProc();
        };

        this->readerThread = new std::thread(f);
    }

    bool isOpen = false;

    virtual bool IsOpen() const
    {
        return isOpen;
    }

    uint8_t *AllocateRealtimeBuffer(size_t size)
    {
        uint8_t *result = new uint8_t[size];
        // populate pages if required. We should have mlocked-ed alread, so they will stay.
        for (size_t i = 0; i < size; i += 1024)
        {
            result[i] = 0;
        }
        return result;
    }

    virtual void Open(const JackServerSettings &jackServerSettings, const JackChannelSelection &channelSelection)
    {

        std::lock_guard guard(mutex);

        if (isOpen)
        {
            throw PiPedalStateException("Already open.");
        }
        isOpen = true;

        if (jackServerSettings.IsDummyAudioDevice())
        {
            this->isDummyAudioDriver = true;
            this->audioDriver = std::unique_ptr<AudioDriver>(CreateDummyAudioDriver(this, jackServerSettings.GetAlsaInputDevice()));
        }
        else
        {
            this->isDummyAudioDriver = false;
            this->audioDriver = std::unique_ptr<AudioDriver>(CreateAlsaDriver(this));
        }

        if (channelSelection.GetInputAudioPorts().size() == 0 || channelSelection.GetOutputAudioPorts().size() == 0)
        {
            return;
        }

        this->currentSample = 0;
        this->underruns = 0;

        this->inputRingBuffer.reset();
        this->outputRingBuffer.reset();
        this->hostReader.Reset();
        this->hostWriter.Reset();
        this->realtimeReader.Reset();
        this->realtimeWriter.Reset();

        this->channelSelection = channelSelection;

        StartReaderThread();

        try
        {
            audioDriver->Open(jackServerSettings, this->channelSelection);
            audioDriver->SetAlsaSequencer(this->alsaSequencer);
            this->sampleRate = audioDriver->GetSampleRate();

            this->overrunGracePeriodSamples = (uint64_t)(((uint64_t)this->sampleRate) * OVERRUN_GRACE_PERIOD_S);
            this->vuSamplesPerUpdate = (size_t)(sampleRate * VU_UPDATE_RATE_S);

            active = true;
            audioStopped = false;
            audioDriver->Activate();
            Lv2Log::info(SS("Audio started. " << audioDriver->GetConfigurationDescription()));
        }
        catch (const std::exception &e)
        {
            Close();
            active = false;
            isOpen = false;
            throw;
        }
    }

    void OnMidiProgramRequest(RealtimeMidiProgramRequest &programRequest)
    {
        pNotifyCallbacks->OnNotifyMidiProgramChange(programRequest);
    }

    void OnPathPropertyReceived(PatchPropertyWriter::Buffer *buffer)
    {
        if (pNotifyCallbacks)
        {
            pNotifyCallbacks->OnNotifyPathPatchPropertyReceived(
                buffer->instanceId,
                buffer->patchPropertyUrid,
                (LV2_Atom *)(buffer->memory.data()));
        }
        buffer->OnBufferReadComplete();
    }
    void OnActivePedalboardReleased(Lv2Pedalboard *pPedalboard)
    {
        if (pPedalboard)
        {
            std::lock_guard guard(mutex);

            for (auto it = activePedalboards.begin(); it != activePedalboards.end(); ++it)
            {
                if ((*it).get() == pPedalboard)
                {
                    // erase it, relinquishing shared_ptr ownership, usually deleting the object.
                    activePedalboards.erase(it);
                    return;
                }
            }
        }
    }

    virtual void SetPedalboard(const std::shared_ptr<Lv2Pedalboard> &pedalboard)
    {
        std::lock_guard guard(mutex);

        this->currentPedalboard = pedalboard;
        if (active && pedalboard)
        {
            pedalboard->Activate();
            this->activePedalboards.push_back(pedalboard);
            hostWriter.ReplaceEffect(pedalboard.get());
        }
    }

    virtual void SetBypass(uint64_t instanceId, bool enabled)
    {
        std::lock_guard guard(mutex);
        if (active && this->currentPedalboard)
        {
            // use indices not instance ids, so we can just do a straight array index on the audio thread.
            auto index = currentPedalboard->GetIndexOfInstanceId(instanceId);
            if (index >= 0)
            {
                hostWriter.SetBypass((uint32_t)index, enabled);
            }
        }
    }

    virtual void SetPluginPreset(uint64_t instanceId, const std::vector<ControlValue> &values)
    {
        std::lock_guard guard(mutex);
        if (active && this->currentPedalboard)
        {
            auto effectIndex = currentPedalboard->GetIndexOfInstanceId(instanceId);
            if (effectIndex != -1)
            {
                for (size_t i = 0; i < values.size(); ++i)
                {
                    const ControlValue &value = values[i];
                    int controlIndex = this->currentPedalboard->GetControlIndex(instanceId, value.key());
                    if (controlIndex != -1 && effectIndex != -1)
                    {
                        hostWriter.SetControlValue(effectIndex, controlIndex, value.value());
                    }
                }
            }
        }
    }

    void SetControlValue(uint64_t instanceId, const std::string &symbol, float value)
    {
        std::lock_guard guard(mutex);
        if (active && this->currentPedalboard)
        {
            // use indices not instance ids, so we can just do a straight array index on the audio thread.
            int controlIndex = this->currentPedalboard->GetControlIndex(instanceId, symbol);
            auto effectIndex = currentPedalboard->GetIndexOfInstanceId(instanceId);

            if (controlIndex != -1 && effectIndex != -1)
            {
                hostWriter.SetControlValue(effectIndex, controlIndex, value);
            }
        }
    }

    virtual void SetInputVolume(float value)
    {
        std::lock_guard guard(mutex);
        if (active && this->currentPedalboard)
        {
            hostWriter.SetInputVolume(value);
        }
    }

    virtual void SetOutputVolume(float value)
    {
        std::lock_guard guard(mutex);
        if (active && this->currentPedalboard)
        {
            hostWriter.SetOutputVolume(value);
        }
    }

    std::vector<IndexedSnapshot *> pendingSnapshots;

    virtual void LoadSnapshot(Snapshot &snapshot, PluginHost &pluginHost) override
    {
        std::lock_guard guard(mutex);
        if (active && this->currentPedalboard)
        {
            IndexedSnapshot *indexedSnapshot = new IndexedSnapshot(&snapshot, this->currentPedalboard, pluginHost);
            pendingSnapshots.push_back(indexedSnapshot);
            this->hostWriter.LoadSnapshot(indexedSnapshot);
        }
    }

    virtual void SetAlsaSequencerConfiguration(const AlsaSequencerConfiguration &alsaSequencerConfiguration) override;


    void OnNotifyPathPatchPropertyReceived(
        int64_t instanceId,
        const std::string &pathPatchPropertyUri,
        const std::string &jsonAtom) override;

    void OnFreeSnapshot(IndexedSnapshot *snapshot)
    {
        {
            std::lock_guard guard(mutex);
            for (auto i = pendingSnapshots.begin(); i != pendingSnapshots.end(); ++i)
            {
                if (*i == snapshot)
                {
                    pendingSnapshots.erase(i);
                    break;
                }
            }
        }
        delete snapshot;
    }
    void CleanUpSnapshots()
    {
        for (auto i = pendingSnapshots.begin(); i != pendingSnapshots.end(); ++i)
        {
            IndexedSnapshot *snapshot = *i;
            delete snapshot;
        }
        pendingSnapshots.clear();
    }

    virtual void SetVuSubscriptions(const std::vector<int64_t> &instanceIds)
    {
        std::lock_guard guard(mutex);

        if (active && this->currentPedalboard)
        {

            if (instanceIds.size() == 0)
            {
                this->hostWriter.SetVuSubscriptions(nullptr);
            }
            else
            {
                RealtimeVuBuffers *vuConfig = new RealtimeVuBuffers();

                for (size_t i = 0; i < instanceIds.size(); ++i)
                {
                    int64_t instanceId = instanceIds[i];
                    auto effect = this->currentPedalboard->GetEffect(instanceId);
                    if (effect)
                    {
                        int index = this->currentPedalboard->GetIndexOfInstanceId(instanceIds[i]);
                        vuConfig->enabledIndexes.push_back(index);
                        VuUpdate v;
                        v.instanceId_ = instanceId;
                        // Display mono VUs if a stereo device is being fed identical L/R inputs.
                        v.isStereoInput_ = effect->GetNumberOfInputAudioBuffers() >= 2 
                            && effect->GetAudioInputBuffer(0) != effect->GetAudioInputBuffer(1);
                        v.isStereoOutput_ = effect->GetNumberOfOutputAudioBuffers() >= 2;

                        vuConfig->vuUpdateWorkingData.push_back(v);
                        vuConfig->vuUpdateResponseData.push_back(v);
                    }
                    else if (instanceId == Pedalboard::INPUT_VOLUME_ID || instanceId == Pedalboard::OUTPUT_VOLUME_ID)
                    {
                        int index = (int)instanceId;
                        VuUpdate v;
                        vuConfig->enabledIndexes.push_back(index);
                        v.instanceId_ = instanceId;
                        if (instanceId == Pedalboard::INPUT_VOLUME_ID)
                        {
                            v.isStereoInput_ = v.isStereoOutput_ = this->pHost->GetNumberOfInputAudioChannels() > 1;
                        }
                        else
                        {
                            v.isStereoInput_ = v.isStereoOutput_ = this->pHost->GetNumberOfOutputAudioChannels() > 1;
                        }
                        vuConfig->vuUpdateWorkingData.push_back(v);
                        vuConfig->vuUpdateResponseData.push_back(v);
                    }
                }

                this->hostWriter.SetVuSubscriptions(vuConfig);
            }
        }
    }

    RealtimeMonitorPortSubscription MakeRealtimeSubscription(const MonitorPortSubscription &subscription)
    {
        RealtimeMonitorPortSubscription result;
        result.subscriptionHandle = subscription.subscriptionHandle;
        result.instanceIndex = this->currentPedalboard->GetIndexOfInstanceId(subscription.instanceid);
        IEffect *pEffect = this->currentPedalboard->GetEffect(subscription.instanceid);

        result.portIndex = pEffect->GetControlIndex(subscription.key);
        result.sampleRate = (int)(this->GetSampleRate() * subscription.updateInterval);
        result.samplesToNextCallback = result.sampleRate;
        PortMonitorCallback *ptr = new PortMonitorCallback(subscription.onUpdate);
        result.callbackPtr = ptr;
        return result;
    }
    virtual void SetMonitorPortSubscriptions(const std::vector<MonitorPortSubscription> &subscriptions)
    {
        if (!active)
            return;
        if (this->currentPedalboard == nullptr)
            return;
        if (subscriptions.size() == 0)
        {
            this->hostWriter.SetMonitorPortSubscriptions(nullptr);
        }
        else
        {
            RealtimeMonitorPortSubscriptions *pSubscriptions = new RealtimeMonitorPortSubscriptions();

            for (size_t i = 0; i < subscriptions.size(); ++i)
            {
                if (this->currentPedalboard->GetEffect(subscriptions[i].instanceid) != nullptr)
                {
                    pSubscriptions->subscriptions.push_back(
                        MakeRealtimeSubscription(subscriptions[i]));
                }
            }
            this->hostWriter.SetMonitorPortSubscriptions(pSubscriptions);
        }
    }

private:
    // virtual bool UpdatePluginStates(Pedalboard &pedalboard) override;
    virtual bool UpdatePluginState(PedalboardItem &pedalboardItem) override;

    class RestartThread
    {
        AudioHostImpl *this_;
        JackServerSettings jackServerSettings;
        std::function<void(bool success, const std::string &errorMessage)> onComplete;
        std::atomic<bool> isComplete = false;
        std::thread *pThread = nullptr;

    public:
        RestartThread(
            AudioHostImpl *host,
            const JackServerSettings &jackServerSettings_,
            std::function<void(bool success, const std::string &errorMessage)> onComplete_)
            : this_(host),
              jackServerSettings(jackServerSettings_),
              onComplete(onComplete_)
        {
        }
        ~RestartThread()
        {
            pThread->join();
            delete pThread;
        }
        bool IsComplete() const { return isComplete; }

        void ThreadProc()
        {
            this_->restarting = true;
            // this_->Close(); (JackServerConfiguration now does a service restart.)
            try
            {
                AdminClient client;

                client.SetJackServerConfiguration(jackServerSettings);

                // this_->Open(this_->channelSelection);
                this_->restarting = false;
                onComplete(true, "");
                isComplete = true;
            }
            catch (const std::exception &e)
            {
                onComplete(false, e.what());
                this_->restarting = false;
                isComplete = true;
            }
        }
        static void ThreadProc_(RestartThread *this_)
        {
            this_->ThreadProc();
        }
        void Run()
        {
            pThread = new std::thread(ThreadProc_, this);
        }
    };

    bool restarting = false;
    std::vector<RestartThread *> restartThreads;

public:
    std::recursive_mutex restart_mutex;
    virtual void UpdateServerConfiguration(const JackServerSettings &jackServerSettings,
                                           std::function<void(bool success, const std::string &errorMessage)> onComplete)
    {
        std::lock_guard guard(restart_mutex);
        RestartThread *pShutdown = new RestartThread(this, jackServerSettings, onComplete);
        restartThreads.push_back(pShutdown);
        pShutdown->Run();
    }
    void CleanRestartThreads(bool final)
    {
        std::lock_guard guard{restart_mutex};
        for (size_t i = 0; i < restartThreads.size(); ++i)
        {
            if (final)
            {
                delete restartThreads[i];
            }
            else
            {
                if (restartThreads[i]->IsComplete())
                {
                    delete restartThreads[i];
                    restartThreads.erase(restartThreads.begin() + i);
                    --i;
                }
            }
        }
        if (final)
        {
            restartThreads.clear();
        }
    }

    virtual void sendRealtimeParameterRequest(RealtimePatchPropertyRequest *pParameterRequest)
    {
        if (!active)
        {
            pParameterRequest->errorMessage = "Not active.";
            pParameterRequest->onPatchRequestComplete(pParameterRequest);
            return;
        }
        this->hostWriter.ParameterRequest(pParameterRequest);
    }

    virtual JackHostStatus getJackStatus()
    {
        CleanRestartThreads(false);
        using namespace std::chrono;
        using std::chrono::duration_cast;
        using std::chrono::milliseconds;

        std::lock_guard guard(mutex);

        JackHostStatus result;
        result.underruns_ = this->underruns;
        auto dt = duration_cast<milliseconds>(std::chrono::system_clock ::now().time_since_epoch()).count() - duration_cast<milliseconds>(this->lastUnderrunTime.load().time_since_epoch()).count();

        result.msSinceLastUnderrun_ = (uint64_t)dt;

        result.temperaturemC_ = (int32_t)(std::round(cpuTemperatureMonitor->GetTemperatureC() * 1000));

        result.active_ = IsAudioRunning();
        result.restarting_ = this->restarting;

        if (this->audioDriver != nullptr)
        {
            result.cpuUsage_ = audioDriver->CpuUse();
        }
        GetCpuFrequency(&result.cpuFreqMin_, &result.cpuFreqMax_);
        result.hasCpuGovernor_ = HasCpuGovernor();
        if (result.hasCpuGovernor_)
        {
            result.governor_ = GetGovernor();
        }
        else
        {
            result.governor_ = "";
        }

        return result;
    }
    std::atomic<bool> listenForMidiEvent = false;
    std::atomic<bool> listenForAtomOutput = false;

    virtual void SetListenForMidiEvent(bool listen)
    {
        this->listenForMidiEvent = listen;
    }
    virtual void SetListenForAtomOutput(bool listen)
    {
        this->listenForAtomOutput = listen;
    }
};

static std::string UriToFieldName(const std::string &uri)
{
    int pos;
    for (pos = uri.length(); pos >= 0; --pos)
    {
        char c = uri[pos];
        if (c == '#' || c == '/' || c == ':')
        {
            break;
        }
    }
    return uri.substr(pos + 1);
}

void AudioHostImpl::SetSystemMidiBindings(const std::vector<MidiBinding> &bindings)
{
    for (auto i = bindings.begin(); i != bindings.end(); ++i)
    {

        if (i->symbol() == "nextBank")
        {
            this->nextBankMidiBinding.SetBinding(*i);
        }
        else if (i->symbol() == "prevBank")
        {
            this->prevBankMidiBinding.SetBinding(*i);
        }
        else if (i->symbol() == "nextProgram")
        {
            this->nextPresetMidiBinding.SetBinding(*i);
        }
        else if (i->symbol() == "prevProgram")
        {
            this->prevPresetMidiBinding.SetBinding(*i);
        }
        else if (i->symbol() == "startHotspot")
        {
            this->startHotspotMidiBinding.SetBinding(*i);
        }
        else if (i->symbol() == "stopHotspot")
        {
            this->stopHotspotMidiBinding.SetBinding(*i);
        }
        else if (i->symbol() == "reboot")
        {
            this->rebootMidiBinding.SetBinding(*i);
        }
        else if (i->symbol() == "shutdown")
        {
            this->shutdownMidiBinding.SetBinding(*i);
        }
        else if (i->symbol() == "snapshot1")
        {
            this->snapshot1MidiBinding.SetBinding(*i);
        }
        else if (i->symbol() == "snapshot2")
        {
            this->snapshot2MidiBinding.SetBinding(*i);
        }
        else if (i->symbol() == "snapshot3")
        {
            this->snapshot3MidiBinding.SetBinding(*i);
        }
        else if (i->symbol() == "snapshot4")
        {
            this->snapshot4MidiBinding.SetBinding(*i);
        }
        else if (i->symbol() == "snapshot5")
        {
            this->snapshot5MidiBinding.SetBinding(*i);
        }
        else if (i->symbol() == "snapshot6")
        {
            this->snapshot6MidiBinding.SetBinding(*i);
        }
        else
        {
            Lv2Log::error(SS("Invalid system midi binding: " << i->symbol()));
        }
    }
}

AudioHost *AudioHost::CreateInstance(IHost *pHost)
{
    return new AudioHostImpl(pHost);
}

// Removed because any updates to state have to be sent to clients as well,
// so this functionality was moved to PiPedalModel::SyncLv2States();
// bool AudioHostImpl::UpdatePluginStates(Pedalboard &pedalboard)
// {
//     bool changed = false;
//     auto pedalboardItems = pedalboard.GetAllPlugins();
//     for (PedalboardItem *item : pedalboardItems)
//     {
//         if (!item->isSplit())
//         {
//             if (UpdatePluginState(*item))
//             {
//                 changed = true;
//             }
//         }
//     }
//     return changed;
// }

void AudioHostImpl::OnNotifyPathPatchPropertyReceived(
    int64_t instanceId,
    const std::string &pathPatchPropertyUri,
    const std::string &jsonAtom)
{
    if (this->currentPedalboard)
    {
        IEffect *effect = this->currentPedalboard->GetEffect(instanceId);
        if (!effect)
        {
            return;
        }
        if (effect->IsLv2Effect())
        {
            Lv2Effect *lv2Effect = (Lv2Effect *)effect;
            lv2Effect->SetPathPatchProperty(pathPatchPropertyUri, jsonAtom);
        }
    }
}

bool AudioHostImpl::UpdatePluginState(PedalboardItem &pedalboardItem)
{
    IEffect *effect = currentPedalboard->GetEffect(pedalboardItem.instanceId());
    if (effect != nullptr)
    {
        try
        {
            Lv2PluginState state;
            if (!effect->GetLv2State(&state))
            {
                return false;
            }
            if (state != pedalboardItem.lv2State())
            {
                pedalboardItem.lv2State(state);
                return true;
            }
            return false;
        }
        catch (const std::exception &e)
        {
            Lv2Log::warning(SS(pedalboardItem.pluginName() << ": " << e.what()));
            return false;
        }
    }
    return false;
}

void AudioHostImpl::OnWritePatchPropertyBuffer(
    PatchPropertyWriter::Buffer *buffer)
{
    this->realtimeWriter.SendPathPropertyBuffer(buffer);
}

void AudioHostImpl::SetAlsaSequencerConfiguration(const AlsaSequencerConfiguration &alsaSequencerConfiguration)
{
    this->alsaSequencer->SetConfiguration(alsaSequencerConfiguration);
}



JSON_MAP_BEGIN(JackHostStatus)
JSON_MAP_REFERENCE(JackHostStatus, active)
JSON_MAP_REFERENCE(JackHostStatus, errorMessage)
JSON_MAP_REFERENCE(JackHostStatus, restarting)
JSON_MAP_REFERENCE(JackHostStatus, underruns)
JSON_MAP_REFERENCE(JackHostStatus, cpuUsage)
JSON_MAP_REFERENCE(JackHostStatus, msSinceLastUnderrun)
JSON_MAP_REFERENCE(JackHostStatus, temperaturemC)
JSON_MAP_REFERENCE(JackHostStatus, cpuFreqMin)
JSON_MAP_REFERENCE(JackHostStatus, cpuFreqMax)
JSON_MAP_REFERENCE(JackHostStatus, hasCpuGovernor)
JSON_MAP_REFERENCE(JackHostStatus, governor)
JSON_MAP_END()
