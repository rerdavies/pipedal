// Copyright (c) 2022-2023 Robin Davies
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

#include "Lv2Log.hpp"

#include "JackDriver.hpp"
#include "AlsaDriver.hpp"
#include "AtomConverter.hpp"

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
    if (fMin == 0)
        fMax = 0;
    *freqMin = fMin;
    *freqMax = fMax;
}
static std::string GetGovernor()
{
    return pipedal::GetCpuGovernor();
}


class SystemMidiBinding {
private:
    MidiBinding currentBinding;
    bool controlState = false;
public:
    void SetBinding(const MidiBinding&binding) { currentBinding = binding; controlState = false; }

    bool IsMatch(const MidiEvent&event);
    bool IsTriggered(const MidiEvent&event);
};

bool SystemMidiBinding::IsTriggered(const MidiEvent &event)
{
    switch (currentBinding.bindingType())
    {
        case BINDING_TYPE_NOTE:
        {
            if (event.size != 3) return false;
            auto  command = event.buffer[0] & 0xF0;
            if (command != 0x90) return false; // MIDI note on
            if (event.buffer[1] != currentBinding.note()) return false;
            return event.buffer[2] != 0;  // note-on with velicity of zero is a note-off.
        }
        break;
        case BINDING_TYPE_CONTROL:
        {
            if (event.size != 3) return false;
            auto  command = event.buffer[0] & 0xF0;
            if (command != 0xB0) return false;
            if (event.buffer[1] != currentBinding.control()) return false;
            bool state = event.buffer[2] >= 0x64;
            if (state != this->controlState)
            {
                this->controlState = state;
                return state;
            }
            return false;
        }
        break;
        default:
            return false;
    }
}

bool SystemMidiBinding::IsMatch(const MidiEvent&event)
{
    switch (currentBinding.bindingType())
    {
        case BINDING_TYPE_NOTE:
        {
            if (event.size != 3) return false;
            auto  command = event.buffer[0] & 0xF0;
            if (command != 0x80 && command != 0x90) return false; // note on/note off.
            return event.buffer[1] == currentBinding.note();
        }
        break;
        case BINDING_TYPE_CONTROL:
        {
            if (event.size != 3) return false;
            auto  command = event.buffer[0] & 0xF0;
            if (command != 0xB0) return false;
            return event.buffer[1] == currentBinding.control();
        }
        break;
        default:
            return false;
    }
}

class AudioHostImpl : public AudioHost, private AudioDriverHost
{
private:
    IHost *pHost = nullptr;
    LV2_Atom_Forge inputWriterForge;


    std::mutex atomConverterMutex;
    AtomConverter atomConverter;

    static constexpr size_t DEFERRED_MIDI_BUFFER_SIZE = 1024;

    uint8_t deferredMidiMessages[DEFERRED_MIDI_BUFFER_SIZE];
    size_t deferredMidiMessageCount;
    bool midiProgramChangePending = false;
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

    AudioDriver *audioDriver = nullptr;

    inherit_priority_recursive_mutex mutex;
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

    SystemMidiBinding nextMidiBinding;
    SystemMidiBinding prevMidiBinding;

    JackChannelSelection channelSelection;
    bool active = false;

    std::shared_ptr<Lv2Pedalboard> currentPedalboard;
    std::vector<std::shared_ptr<Lv2Pedalboard>> activePedalboards; // pedalboards that have been sent to the audio queue.
    Lv2Pedalboard *realtimeActivePedalboard = nullptr;

    std::vector<uint8_t *> midiLv2Buffers;

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


    std::string AtomToJson(const LV2_Atom *pAtom)
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
        std::lock_guard guard(mutex);
        if (!isOpen)
            return;

        isOpen = false;

        if (realtimeMonitorPortSubscriptions != nullptr)
        {
            delete realtimeMonitorPortSubscriptions;
            realtimeMonitorPortSubscriptions = nullptr;
        }
        if (active)
        {
            audioDriver->Deactivate();

            active = false;
        }

        audioDriver->Close();

        StopReaderThread();
        pHost->GetHostWorkerThread()->Close();


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
            realtimeVuBuffers = nullptr;
        }
        this->inputRingBuffer.reset();
        this->outputRingBuffer.reset();

        for (size_t i = 0; i < midiLv2Buffers.size(); ++i)
        {
            delete[] midiLv2Buffers[i];
        }
        midiLv2Buffers.resize(0);

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
            float *out = (float *)audioDriver->GetOutputBuffer(i, nframes);
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


    virtual void SetSystemMidiBindings(const std::vector<MidiBinding>&bindings);

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

    bool reEntered = false;
    void ProcessInputCommands()
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
                }
                break;
            }
            default:
                throw PiPedalStateException("Unknown Ringbuffer command.");
            }
        }
        reEntered = false;
    }

    virtual void AckMidiProgramRequest(uint64_t requestId)
    {
        hostWriter.AckMidiProgramRequest(requestId);
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

    bool onMidiEvent(Lv2EventBufferWriter&eventBufferWriter, Lv2EventBufferWriter::LV2_EvBuf_Iterator &iterator, MidiEvent&event)
    {
        eventBufferWriter.writeMidiEvent(iterator, 0, event.size, event.buffer);

        this->realtimeActivePedalboard->OnMidiMessage(event.size, event.buffer, this, fnMidiValueChanged);
        if (listenForMidiEvent)
        {
            if (event.size >= 3)
            {
                uint8_t cmd = (uint8_t)(event.buffer[0] & 0xF0);
                bool isNote = cmd == 0x90;
                bool isControl = cmd == 0xB0;
                if (isNote || isControl)
                {
                    realtimeWriter.OnMidiListen(isNote, event.buffer[1]);
                }
            }
        }
        return true;
    }


    void ProcessMidiInput()
    {
        Lv2EventBufferWriter eventBufferWriter(this->eventBufferUrids);

        size_t midiInputBufferCount = audioDriver->MidiInputBufferCount();

        audioDriver->FillMidiBuffers();

        for (size_t midiDeviceIx = 0; midiDeviceIx < midiInputBufferCount; ++midiDeviceIx)
        {

            void *portBuffer = audioDriver->GetMidiInputBuffer(midiDeviceIx, 0);
            if (portBuffer)
            {
                uint8_t *lv2Buffer = this->midiLv2Buffers[midiDeviceIx];
                size_t n = audioDriver->GetMidiInputEventCount(portBuffer);

                eventBufferWriter.Reset(lv2Buffer, MIDI_LV2_BUFFER_SIZE);
                Lv2EventBufferWriter::LV2_EvBuf_Iterator iterator = eventBufferWriter.begin();
                MidiEvent event;


                // write all deferred midi messages.
                if (deferredMidiMessageCount != 0 && !midiProgramChangePending)
                {
                    for (size_t i = 0; i < deferredMidiMessageCount; /**/)
                    {
                        int8_t deviceIndex = deferredMidiMessages[i++];
                        int8_t messageCount = deferredMidiMessages[i++];
                        if (deviceIndex == midiDeviceIx)
                        {
                            event.size = messageCount;
                            event.buffer = deferredMidiMessages +i;
                            event.time = 0;

                            onMidiEvent(eventBufferWriter,iterator,event);
                        }

                        i += messageCount;
                    }
                }


                for (size_t frame = 0; frame < n; ++frame)
                {
                    if (audioDriver->GetMidiInputEvent(&event, portBuffer, frame))
                    {
                        uint8_t midiCommand = (uint8_t)(event.buffer[0] & 0xF0);
                        if (midiCommand == 0xC0) // midi program change.
                        {
                            this->deferredMidiMessageCount = 0; // we can discard previous control changes.
                            midiProgramChangePending = true;

                            this->realtimeWriter.OnMidiProgramChange(++(this->midiProgramChangeId), selectedBank,event.buffer[1]);
                        }
                        else if (isBankChange(event))
                        {
                            this->selectedBank = event.buffer[2];
                        } else if (this->nextMidiBinding.IsMatch(event))
                        {
                            if (nextMidiBinding.IsTriggered(event))
                            {
                                midiProgramChangePending = true;
                                this->realtimeWriter.OnNextMidiProgram(++(this->midiProgramChangeId),1);
                            }
                        } else if (this->prevMidiBinding.IsMatch(event))
                        {
                            if (prevMidiBinding.IsTriggered(event))
                            {
                                midiProgramChangePending = true;
                                this->realtimeWriter.OnNextMidiProgram(++(this->midiProgramChangeId),-1);
                            }
                        }
                        else if (midiProgramChangePending)
                        {
                            // defer the message for processing after the program change has completed.
                            if (event.size > 0 
                                && event.size < 128 
                                && event.size + 2 + this->deferredMidiMessageCount < DEFERRED_MIDI_BUFFER_SIZE)
                            {
                                this->deferredMidiMessages[deferredMidiMessageCount++] = midiDeviceIx;
                                this->deferredMidiMessages[deferredMidiMessageCount++] = (uint8_t)event.size;
                                for (size_t i = 0; i < event.size; ++i)
                                {
                                    this->deferredMidiMessages[deferredMidiMessageCount++] = event.buffer[i];
                                }
                            }
                        }
                        else
                        {
                            onMidiEvent(eventBufferWriter,iterator,event);
                        }
                    }
                }
            }
        }
    }

#define RESET_XRUN_SAMPLES 22050ul // 1/2 a second-ish.

    std::mutex audioStoppedMutex;

    bool IsAudioActive()
    {
        std::lock_guard lock{audioStoppedMutex};
        return this->active;
    }
    virtual void OnAudioStopped()
    {
        std::lock_guard lock{audioStoppedMutex};
        this->active = false;
        Lv2Log::info("Audio stopped.");
    }

    virtual void OnProcess(size_t nframes)
    {
        try
        {
            float *in, *out;

            pParameterRequests = nullptr;

            ProcessInputCommands();

            bool processed = false;

            Lv2Pedalboard *pedalboard = this->realtimeActivePedalboard;
            if (pedalboard != nullptr)
            {
                ProcessMidiInput();
                float *inputBuffers[4];
                float *outputBuffers[4];
                bool buffersValid = true;
                for (int i = 0; i < audioDriver->InputBufferCount(); ++i)
                {
                    float *input = (float *)audioDriver->GetInputBuffer(i, nframes);
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
                    float *output = audioDriver->GetOutputBuffer(i, nframes);
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
                    pedalboard->ResetAtomBuffers();
                    pedalboard->ProcessParameterRequests(pParameterRequests);

                    processed = pedalboard->Run(inputBuffers, outputBuffers, (uint32_t)nframes, &realtimeWriter);
                    if (processed)
                    {
                        if (this->realtimeVuBuffers != nullptr)
                        {
                            pedalboard->ComputeVus(this->realtimeVuBuffers, (uint32_t)nframes);

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
                }
            }

            if (!processed)
            {
                ZeroOutputBuffers(nframes);
            }

            if (pParameterRequests != nullptr)
            {
                this->realtimeWriter.ParameterRequestComplete(pParameterRequests);
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
            Lv2Log::error("Fatal error while processing jack audio. (%s)", e.what());
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
        realtimeAtomBuffer.resize(32*1024);
        lv2_atom_forge_init(&inputWriterForge,pHost->GetMapFeature().GetMap());

#if JACK_HOST
        audioDriver = CreateJackDriver(this);
#endif
#if ALSA_HOST
        audioDriver = CreateAlsaDriver(this);
#endif
    }
    virtual ~AudioHostImpl()
    {
        Close();
        CleanRestartThreads(true);

        delete audioDriver;
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



    void OnAudioComplete()
    {
        // there is actually no compelling circumstance in which this should ever happen.

        Lv2Log::error("Audio processing terminated unexpectedly.");
        realtimeWriter.AudioStopped();
    }
    std::vector<uint8_t> atomBuffer;
    std::vector<uint8_t> realtimeAtomBuffer;

    bool terminateThread;
    void ThreadProc()
    {

#if defined(__WIN32)
        // bump thread prioriy two levels to
        // ensure that the service thread doesn't
        // get bogged down by UIwork. Doesn't have to be realtime, but it
        // MUST run at higher priority than UI threads.
        xxx; // TO DO.
#elif defined(__linux__)
        int min = sched_get_priority_min(SCHED_RR);
        int max = sched_get_priority_max(SCHED_RR);

        struct sched_param param;
        memset(&param, 0, sizeof(param));
        param.sched_priority = min;

        int result = sched_setscheduler(0, SCHED_RR, &param);
        if (result == 0)
        {
            Lv2Log::debug("Service thread priority successfully boosted.");
        }
        SetThreadName("aout");
#else
        xxx; // TODO!
#endif
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
            clock_time waitTime =  std::chrono::steady_clock::now();

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
                                uint16_t msg;
                                hostReader.read(&msg);
                                if (this->pNotifyCallbacks)
                                {
                                    pNotifyCallbacks->OnNotifyMidiListen((msg & 0xFF00) != 0, (uint8_t)msg);
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
                                            if (pRequest->GetSize() != 0) {
                                                IEffect *pEffect = currentPedalboard->GetEffect(pRequest->instanceId);
                                                if (pEffect == nullptr)
                                                {
                                                    pRequest->errorMessage = "Effect no longer available.";
                                                }
                                                else
                                                {
                                                    pRequest->jsonResponse = AtomToJson((LV2_Atom*)pRequest->GetBuffer());
                                                }
                                            } else {
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
                                if (pEffect != nullptr && this->pNotifyCallbacks && listenForAtomOutput)
                                {
                                    LV2_Atom *atom = (LV2_Atom*)&atomBuffer[0];
                                    if (atom->type == uris.atom_Object)
                                    {
                                        LV2_Atom_Object *obj = (LV2_Atom_Object*)atom;
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
                                                LV2_URID propertyUrid = ((LV2_Atom_URID*)property)->body;
                                                if (pNotifyCallbacks->WantsAtomOutput(instanceId,propertyUrid))
                                                {
                                                    auto json = AtomToJson(value);
                                                    this->pNotifyCallbacks->OnNotifyAtomOutput(instanceId,propertyUrid,json);
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
                            else if (command == RingBufferCommand::AudioStopped)
                            {
                                AudioStoppedBody body;
                                hostReader.read(&body);
                                OnAudioComplete();
                                return;
                            } else if (command == RingBufferCommand::MidiProgramChange)
                            {
                                RealtimeMidiProgramRequest programRequest;
                                hostReader.read(&programRequest);
                                OnMidiProgramRequest(programRequest);
                            } else if (command == RingBufferCommand::NextMidiProgram)
                            {
                                RealtimeNextMidiProgramRequest request;
                                hostReader.read(&request);
                                pNotifyCallbacks->OnNotifyNextMidiProgram(request);
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
        if (channelSelection.GetInputAudioPorts().size() == 0 || channelSelection.GetOutputAudioPorts().size() == 0)
        {
            return;
        }

        this->currentSample = 0;
        this->underruns = 0;

        if (isOpen)
        {
            throw PiPedalStateException("Already open.");
        }
        isOpen = true;

        this->inputRingBuffer.reset();
        this->outputRingBuffer.reset();

        this->channelSelection = channelSelection;

        StartReaderThread();

        try
        {

            audioDriver->Open(jackServerSettings, this->channelSelection);

            this->sampleRate = audioDriver->GetSampleRate();

            this->overrunGracePeriodSamples = (uint64_t)(((uint64_t)this->sampleRate) * OVERRUN_GRACE_PERIOD_S);
            this->vuSamplesPerUpdate = (size_t)(sampleRate * VU_UPDATE_RATE_S);

            midiLv2Buffers.resize(audioDriver->MidiInputBufferCount());
            for (size_t i = 0; i < audioDriver->MidiInputBufferCount(); ++i)
            {
                midiLv2Buffers[i] = AllocateRealtimeBuffer(MIDI_LV2_BUFFER_SIZE);
            }

            active = true;
            audioDriver->Activate();
            Lv2Log::info(SS("Audio started. " << audioDriver->GetConfigurationDescription()));
        }
        catch (const std::exception &e)
        {
            Lv2Log::error(SS("Failed to start audio. " << e.what()));
            Close();
            active = false;
            throw;
        }
    }

    void OnMidiProgramRequest(RealtimeMidiProgramRequest &programRequest)
    {
        pNotifyCallbacks->OnNotifyMidiProgramChange(programRequest);
    }
    void OnActivePedalboardReleased(Lv2Pedalboard *pPedalboard)
    {
        if (pPedalboard)
        {
            pPedalboard->Deactivate();
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
        if (active)
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
                    if (!effect)
                    {
                        throw PiPedalStateException("Effect not found.");
                    }

                    int index = this->currentPedalboard->GetIndexOfInstanceId(instanceIds[i]);
                    vuConfig->enabledIndexes.push_back(index);
                    VuUpdate v;
                    v.instanceId_ = instanceId;
                    // Display mono VUs if a stereo device is being fed identical L/R inputs.
                    v.isStereoInput_ = effect->GetNumberOfInputAudioPorts() != 1 && effect->GetAudioInputBuffer(0) != effect->GetAudioInputBuffer(1);
                    v.isStereoOutput_ = effect->GetNumberOfOutputAudioPorts() != 1;

                    vuConfig->vuUpdateWorkingData.push_back(v);
                    vuConfig->vuUpdateResponseData.push_back(v);
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

    virtual void UpdatePluginStates(Pedalboard& pedalboard);
    void UpdatePluginState(PedalboardItem &pedalboardItem);

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
    inherit_priority_recursive_mutex restart_mutex;
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
        std::lock_guard guard(restart_mutex);
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

    static int32_t GetRaspberryPiTemperature()
    {
        try
        {
            std::ifstream f("/sys/class/thermal/thermal_zone0/temp");
            int32_t temp;
            f >> temp;
            return temp;
        }
        catch (std::exception &)
        {
            return -1000000;
        }
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

        result.temperaturemC_ = GetRaspberryPiTemperature();

        result.active_ = IsAudioActive();
        result.restarting_ = this->restarting;

        if (this->audioDriver != nullptr)
        {
            result.cpuUsage_ = audioDriver->CpuUse();
        }
        GetCpuFrequency(&result.cpuFreqMax_, &result.cpuFreqMin_);
        result.governor_ = GetGovernor();

        return result;
    }
    volatile bool listenForMidiEvent = false;
    volatile bool listenForAtomOutput = false;

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

void AudioHostImpl::SetSystemMidiBindings(const std::vector<MidiBinding>&bindings) 
{
    for (auto i = bindings.begin(); i != bindings.end(); ++i)
    {
        if (i->symbol() == "nextProgram")
        {
            this->nextMidiBinding.SetBinding(*i);
        } else if (i->symbol() == "prevProgram")
        {
            this->prevMidiBinding.SetBinding(*i);
        }
    }
}


AudioHost *AudioHost::CreateInstance(IHost *pHost)
{
    return new AudioHostImpl(pHost);
}

void AudioHostImpl::UpdatePluginStates(Pedalboard& pedalboard)
{
    auto pedalboardItems = pedalboard.GetAllPlugins();
    for (PedalboardItem* item : pedalboardItems)
    {
        if (!item->isSplit())
        {
            UpdatePluginState(*item);
        }
    }

}
void AudioHostImpl::UpdatePluginState(PedalboardItem &pedalboardItem)
{
    IEffect*effect = currentPedalboard->GetEffect(pedalboardItem.instanceId());
    if (effect != nullptr)
    {
        effect->GetLv2State(const_cast<Lv2PluginState*>(&pedalboardItem.lv2State()));
    }
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
JSON_MAP_REFERENCE(JackHostStatus, governor)
JSON_MAP_END()
