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

#include "JackHost.hpp"

#include "Lv2Log.hpp"

using namespace pipedal;

#include <jack/jack.h>
#include <jack/types.h>
#include <jack/session.h>
#include <jack/midiport.h>
#include <string.h>
#include <stdio.h>
#include <mutex>
#include <thread>
#include <semaphore.h>
#include "VuUpdate.hpp"

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

#include "ShutdownClient.hpp"

const double VU_UPDATE_RATE_S = 1.0 / 30;
const double OVERRUN_GRACE_PERIOD_S = 15; 
using namespace pipedal;

const int MIDI_LV2_BUFFER_SIZE = 16 * 1024;

namespace pipedal
{
    // private import from JackConfiguration.cpp
    std::string GetJackErrorMessage(jack_status_t status);
}

static void GetCpuFrequency(uint64_t*freqMin,uint64_t*freqMax)
{
    uint64_t fMax = 0;
    uint64_t fMin = UINT64_MAX;
    char deviceName[128];
    try {
    for (int i = 0; true; ++i)
    {
        
        snprintf(deviceName,sizeof(deviceName),"/sys/devices/system/cpu/cpu%d/cpufreq/scaling_cur_freq",i);
        std::ifstream f(deviceName);
        if (!f)
        {
            break;
        }
        uint64_t freq;
        f >> freq;
        if (!f) break;
        if (freq < fMin) fMin = freq;
        if (freq > fMax) fMax = freq;
    }
    } catch (const std::exception &)
    {

    }
    if (fMin == 0) fMax = 0;
    *freqMin = fMin;
    *freqMax = fMax;
}
static std::string GetGovernor()
{
    std::string result;
    try {
        std::ifstream f("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        f >> result;
    } catch (const std::exception &)
    {

    }
    return result;
}



class JackHostImpl : public JackHost
{
private:

    inherit_priority_recursive_mutex mutex;
    int64_t overrunGracePeriodSamples = 0;

    IJackHostCallbacks *pNotifyCallbacks = nullptr;

    virtual void SetNotificationCallbacks(IJackHostCallbacks *pNotifyCallbacks)
    {
        this->pNotifyCallbacks = pNotifyCallbacks;
    }

    const size_t RING_BUFFER_SIZE = 64 * 1024;

    RingBuffer<true, false> inputRingBuffer;
    RingBuffer<false, true> outputRingBuffer;

    RingBufferWriter<true,false> x;

    RealtimeRingBufferReader realtimeReader;
    RealtimeRingBufferWriter realtimeWriter;
    HostRingBufferReader hostReader;
    HostRingBufferWriter hostWriter;

    JackChannelSelection channelSelection;
    bool active = false;
    jack_client_t *client = nullptr;

    std::shared_ptr<Lv2PedalBoard> currentPedalBoard;
    std::vector<std::shared_ptr<Lv2PedalBoard>> activePedalBoards; // pedalboards that have been sent to the audio queue.
    Lv2PedalBoard *realtimeActivePedalBoard = nullptr;

    std::vector<jack_port_t *> inputPorts;
    std::vector<jack_port_t *> outputPorts;
    std::vector<jack_port_t *> midiInputPorts;

    std::vector<uint8_t *> midiLv2Buffers;

    uint32_t sampleRate;
    uint64_t currentSample = 0;

    std::atomic<uint64_t> underruns = 0;
    std::atomic<std::chrono::system_clock::time_point> lastUnderrunTime =
        std::chrono::system_clock::from_time_t(0);

    int XrunCallback()
    {
        ++this->underruns;
        this->lastUnderrunTime = std::chrono::system_clock ::now();
        return 0;
    }

    static int xrun_callback_fn(void *arg)
    {
        return ((JackHostImpl *)arg)->XrunCallback();
    }

    virtual void Close()
    {
        std::lock_guard guard(mutex);
        if (!isOpen)
            return;

        isOpen = false;
        StopReaderThread();
        if (active)
        {
            // disconnect ports.
            for (size_t i = 0; i < this->inputPorts.size(); ++i)
            {
                auto port = inputPorts[i];
                if (port != nullptr)
                {
                    jack_disconnect(client, channelSelection.GetInputAudioPorts()[i].c_str(), jack_port_name(port));
                }
            }
            for (size_t i = 0; i < outputPorts.size(); ++i)
            {
                auto port = outputPorts[i];
                if (port != nullptr)
                {
                    jack_disconnect(client, jack_port_name(port),channelSelection.getOutputAudioPorts()[i].c_str());
                }
            }
            for (size_t i = 0; i < midiInputPorts.size(); ++i)
            {
                auto port = midiInputPorts[i];
                if (port != nullptr)
                {
                    jack_disconnect(client, channelSelection.GetInputMidiPorts()[i].c_str(), jack_port_name(port));
                }
            }

            jack_deactivate(client);

            jack_set_process_callback(client, nullptr, nullptr);
            jack_on_shutdown(client, nullptr, nullptr);
            #if JACK_SESSION_CALLBACK // (deprecated, and not actually useful)
                jack_set_session_callback(client, nullptr, nullptr);
            #endif
            jack_set_xrun_callback(client, nullptr, nullptr);

            active = false;
        }
        // unregister ports.
        for (size_t i = 0; i < this->inputPorts.size(); ++i)
        {
            auto port = inputPorts[i];
            if (port)
            {
                jack_port_unregister(client, port);
            }
        }
        inputPorts.clear();

        for (size_t i = 0; i < this->outputPorts.size(); ++i)
        {
            auto port = outputPorts[i];
            if (port)
            {
                jack_port_unregister(client, port);
            }
        }
        outputPorts.clear();
        for (size_t i = 0; i < this->midiInputPorts.size(); ++i)
        {
            auto port = midiInputPorts[i];
            if (port)
            {
                jack_port_unregister(client, port);
            }
        }
        midiInputPorts.resize(0);

        for (size_t i = 0; i < midiLv2Buffers.size(); ++i)
        {
            delete[] midiLv2Buffers[i];
        }
        midiLv2Buffers.resize(0);

        if (client)
        {
            jack_client_close(client);
            client = nullptr;
        }
        // release any pdealboards owned by the process thread.
        this->activePedalBoards.resize(0);
        this->realtimeActivePedalBoard = nullptr;

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
    }

    void ZeroBuffer(float *buffer, jack_nframes_t nframes)
    {
        for (jack_nframes_t i = 0; i < nframes; ++i)
        {
            buffer[i] = 0;
        }
    }
    void ZeroOutputBuffers(jack_nframes_t nframes)
    {
        for (int i = 0; i < this->outputPorts.size(); ++i)
        {
            float *out = (float *)jack_port_get_buffer(this->outputPorts[i], nframes);
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
                    portSubscription.waitingForAck = true;
                    float value = realtimeActivePedalBoard->GetControlOutputValue(
                        portSubscription.instanceIndex,
                        portSubscription.portIndex);

                    this->realtimeWriter.SendMonitorPortUpdate(
                        portSubscription.callbackPtr,
                        portSubscription.subscriptionHandle,
                        value);
                }
            }
        }
    }

    RealtimeParameterRequest *pParameterRequests = nullptr;

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
                this->realtimeActivePedalBoard->SetControlValue(body.effectIndex, body.controlIndex, body.value);
                break;
            }
            case RingBufferCommand::ParameterRequest:
            {
                RealtimeParameterRequest *pRequest = nullptr;
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
                this->realtimeActivePedalBoard->SetBypass(body.effectIndex, body.enabled);
                break;
            }
            case RingBufferCommand::ReplaceEffect:
            {
                ReplaceEffectBody body;
                realtimeReader.readComplete(&body);

                auto oldValue = this->realtimeActivePedalBoard;
                this->realtimeActivePedalBoard = body.effect;

                realtimeWriter.EffectReplaced(oldValue);

                // invalidate the possibly no-good subscriptions. Model will update them shortly.
                freeRealtimeVuConfiguration();
                freeRealtimeMonitorPortSubscriptions();
                break;
            }
            default:
                throw PiPedalStateException("Unknown Ringbuffer command.");
            }
        }
        reEntered = false;
    }

    void OnMidiValueChanged(uint64_t instanceId, int controlIndex, float value)
    {
        realtimeWriter.MidiValueChanged(instanceId, controlIndex, value);
    }
    static void fnMidiValueChanged(void *data, uint64_t instanceId, int controlIndex, float value)
    {
        ((JackHostImpl *)data)->OnMidiValueChanged(instanceId, controlIndex, value);
    }
    void ProcessJackMidi()
    {
        Lv2EventBufferWriter eventBufferWriter(this->eventBufferUrids);

        for (int i = 0; i < this->midiInputPorts.size(); ++i)
        {
            jack_port_t *port = midiInputPorts[i];
            void *portBuffer = jack_port_get_buffer(midiInputPorts[i], 0);
            if (portBuffer)
            {
                uint8_t *lv2Buffer = this->midiLv2Buffers[i];
                jack_nframes_t n = jack_midi_get_event_count(portBuffer);

                eventBufferWriter.Reset(lv2Buffer, MIDI_LV2_BUFFER_SIZE);
                auto iterator = eventBufferWriter.begin();

                for (jack_nframes_t frame = 0; frame < n; ++frame)
                {
                    jack_midi_event_t event;
                    if (jack_midi_event_get(&event, portBuffer, frame) == 0)
                    {
                        eventBufferWriter.writeMidiEvent(iterator, 0, event.size, event.buffer);

                        this->realtimeActivePedalBoard->OnMidiMessage(event.size, event.buffer, this, fnMidiValueChanged);
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
                    }
                }
            }
        }
    }

#define RESET_XRUN_SAMPLES 22050ul // 1/2 a second-ish.

    int OnProcess(jack_nframes_t nframes)
    {
        try
        {
            jack_default_audio_sample_t *in, *out;

            pParameterRequests = nullptr;

            ProcessInputCommands();

            bool processed = false;

            Lv2PedalBoard *pedalBoard = this->realtimeActivePedalBoard;
            if (pedalBoard != nullptr)
            {
                ProcessJackMidi();
                float *inputBuffers[4];
                float *outputBuffers[4];
                bool buffersValid = true;
                for (int i = 0; i < inputPorts.size(); ++i)
                {
                    float *input = (float *)jack_port_get_buffer(inputPorts[i], nframes);
                    if (input == nullptr)
                    {
                        buffersValid = false;
                        break;
                    }
                    inputBuffers[i] = input;
                }
                inputBuffers[inputPorts.size()] = nullptr;

                for (int i = 0; i < outputPorts.size(); ++i)
                {
                    float *output = (float *)jack_port_get_buffer(outputPorts[i], nframes);
                    if (output == nullptr)
                    {
                        buffersValid = false;
                        break;
                    }
                    outputBuffers[i] = output;
                }
                outputBuffers[outputPorts.size()] = nullptr;

                if (buffersValid)
                {
                    pedalBoard->ResetAtomBuffers();
                    pedalBoard->ProcessParameterRequests(pParameterRequests);

                    processed = pedalBoard->Run(inputBuffers, outputBuffers, (uint32_t)nframes,&realtimeWriter);
                    if (processed)
                    {
                        if (this->realtimeVuBuffers != nullptr)
                        {
                            pedalBoard->ComputeVus(this->realtimeVuBuffers, (uint32_t)nframes);

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
                    pedalBoard->GatherParameterRequests(pParameterRequests);
                }
            }

            // in = jack_port_get_buffer(input_port, nframes);
            // out = jack_port_get_buffer(output_port, nframes);
            // memcpy(out, in,
            //        sizeof(jack_default_audio_sample_t) * nframes);
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

        return 0;
    }

    static int process_fn(jack_nframes_t nframes, void *arg)
    {
        return ((JackHostImpl *)arg)->OnProcess(nframes);
    }
    void OnShutdown()
    {
        Lv2Log::info("Jack Audio Server has shut down.");
    }

    static void
    jack_shutdown_fn(void *arg)
    {
        ((JackHostImpl *)arg)->OnShutdown();
    }

    void OnSessionCallback(jack_session_event_t *event)
    {
        #if JACK_SESSION_CALLBACK // deprecated and not actually useful.
        char retval[100];
        Lv2Log::info("path %s, uuid %s, type: %s\n", event->session_dir, event->client_uuid, event->type == JackSessionSave ? "save" : "quit");

        snprintf(retval, 100, "jack_simple_session_client %s", event->client_uuid);
        event->command_line = strdup(retval);

        jack_session_reply(client, event);

        if (event->type == JackSessionSaveAndQuit)
        {
            // simple_quit = 1;
        }

        jack_session_event_free(event);
        #endif
    }

    static void jack_error_fn(const char *msg)
    {
        Lv2Log::error("Jack - %s", msg);
    }
    static void jack_info_fn(const char *msg)
    {
        Lv2Log::info("Jack - %s", msg);
    }

    static void
    session_callback_fn(jack_session_event_t *event, void *arg)
    {
        ((JackHostImpl *)arg)->OnSessionCallback(event);
    };

    Lv2EventBufferUrids eventBufferUrids;

public:
    JackHostImpl(IHost *pHost)
        : inputRingBuffer(RING_BUFFER_SIZE),
          outputRingBuffer(RING_BUFFER_SIZE),
          realtimeReader(&this->inputRingBuffer),
          realtimeWriter(&this->outputRingBuffer),
          hostReader(&this->outputRingBuffer),
          hostWriter(&this->inputRingBuffer),
          eventBufferUrids(pHost)
    {
        jack_set_error_function(jack_error_fn);
        jack_set_info_function(jack_info_fn);
    }
    virtual ~JackHostImpl()
    {
        Close();
        CleanRestartThreads(true);
        jack_set_error_function(nullptr);
        jack_set_info_function(nullptr);
    }

    virtual JackConfiguration GetServerConfiguration()
    {
        JackConfiguration result;

        result.Initialize();
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
#else
        xxx; // TODO!
#endif

        try
        {

            struct timespec ts;
            // ever 30  seconds, timeout check for and log any overruns.
            int pollRateS = 30;
            if (clock_gettime(CLOCK_REALTIME, &ts) == -1)
            {
                Lv2Log::error("clock_gettime failed!");
                return;
            }
            ts.tv_sec += pollRateS;
            uint64_t lastUnderrunCount = this->underruns;

            while (true)
            {

                // wait for an event.
                // 0 -> ready. -1: timed out. -2: closing.

                int result = hostReader.wait(ts);
                if (result == -2)
                {
                    return;
                }
                else if (result == -1)
                {
                    // timeout.
                    ts.tv_sec += pollRateS;
                    uint64_t underruns = this->underruns;
                    if (underruns != lastUnderrunCount)
                    {
                        Lv2Log::info("Jack - Underrun count: %lu", (unsigned long)underruns);
                        lastUnderrunCount = underruns;
                    }
                    clock_gettime(CLOCK_REALTIME, &ts);
                    ts.tv_sec += pollRateS;
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
                                RealtimeParameterRequest *pRequest = nullptr;
                                hostReader.read(&pRequest);

                                std::shared_ptr<Lv2PedalBoard> currentpedalBoard;

                                {
                                    std::lock_guard guard(mutex);
                                    currentPedalBoard = this->currentPedalBoard;
                                }

                                while (pRequest != nullptr)
                                {
                                    auto pNext = pRequest->pNext;
                                    if (pRequest->errorMessage == nullptr && pRequest->responseLength != 0)
                                    {
                                        IEffect *pEffect = currentPedalBoard->GetEffect(pRequest->instanceId);
                                        if (pEffect == nullptr)
                                        {
                                            pRequest->errorMessage = "Effect no longer available.";
                                        }
                                        else
                                        {
                                            pRequest->jsonResponse = pEffect->AtomToJson(pRequest->response);
                                        }
                                    }
                                    pRequest->onJackRequestComplete(pRequest);
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
                            } else if (command == RingBufferCommand::AtomOutput)
                            {
                                uint64_t instanceId;
                                hostReader.read(&instanceId);
                                size_t extraBytes;
                                hostReader.read(&extraBytes);
                                if (atomBuffer.size() < extraBytes)
                                {
                                    atomBuffer.resize(extraBytes);
                                }
                                hostReader.read(extraBytes,&(atomBuffer[0]));

                        
                                IEffect *pEffect = currentPedalBoard->GetEffect(instanceId);
                                if (pEffect != nullptr &&this->pNotifyCallbacks && listenForAtomOutput)
                                {
                                    std::string atomType = pEffect->GetAtomObjectType(&atomBuffer[0]);
                                    auto json  = pEffect->AtomToJson(&(atomBuffer[0]));
                                    this->pNotifyCallbacks->OnNotifyAtomOutput(instanceId,atomType,json);
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
                                OnActivePedalBoardReleased(body.oldEffect);
                            }
                            else if (command == RingBufferCommand::AudioStopped)
                            {
                                AudioStoppedBody body;
                                hostReader.read(&body);
                                OnAudioComplete();
                                return;
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

    static int jackInstanceId; 

    virtual void Open(const JackChannelSelection &channelSelection)
    {

        std::lock_guard guard(mutex);
        if (channelSelection.GetInputAudioPorts().size() == 0
        || channelSelection.getOutputAudioPorts().size() == 0)
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

        jack_status_t status;
        try
        {
            // need a unique instance name every timme.
            std::stringstream s;
            s << "PiPedal";
            if (jackInstanceId != 0) {
                s << jackInstanceId;
            }
            ++jackInstanceId;

            std::string instanceName = s.str();

            client = jack_client_open(instanceName.c_str(), JackNullOption, &status);

            if (client == nullptr || status & JackFailure)
            {
                if (client)
                {
                    jack_client_close(client);
                    client = nullptr;
                }
                std::string error = GetJackErrorMessage(status);
                Lv2Log::error(error);
                throw PiPedalStateException(error.c_str());
            }

            if (status & JackServerStarted)
            {
                Lv2Log::info("Jack server started.");
            }

            jack_set_process_callback(client, process_fn, this);

            jack_on_shutdown(client, jack_shutdown_fn, this);

            #if JACK_SESSION_CALLBACK
                jack_set_session_callback(client, session_callback_fn, NULL);
            #endif

            jack_set_xrun_callback(client, xrun_callback_fn, this);

            this->sampleRate = jack_get_sample_rate(client);

            this->overrunGracePeriodSamples = (uint64_t)(((uint64_t)this->sampleRate)*OVERRUN_GRACE_PERIOD_S);


            this->vuSamplesPerUpdate = (size_t)(sampleRate * VU_UPDATE_RATE_S);

            auto &selectedInputPorts = channelSelection.GetInputAudioPorts();
            this->inputPorts.clear();
            this->outputPorts.clear();
            this->midiInputPorts.clear();

            this->inputPorts.resize(selectedInputPorts.size());
            for (int i = 0; i < selectedInputPorts.size(); ++i)
            {
                std::stringstream name;
                name << "input" << (i + 1);
                this->inputPorts[i] =
                    jack_port_register(
                        client, name.str().c_str(),
                        JACK_DEFAULT_AUDIO_TYPE,
                        JackPortIsInput, 0);
                if (this->inputPorts[i] == nullptr)
                {
                    Lv2Log::error("Can't allocate Jack Audio input ports.");
                    throw PiPedalStateException("Failed to allocate Jack Audio port.");
                }
            }
            auto &selectedOutputPorts = channelSelection.getOutputAudioPorts();
            this->outputPorts.resize(selectedOutputPorts.size());
            for (int i = 0; i < selectedOutputPorts.size(); ++i)
            {
                std::stringstream name;
                name << "output" << (i + 1);
                this->outputPorts[i] =
                    jack_port_register(client, name.str().c_str(),
                                       JACK_DEFAULT_AUDIO_TYPE,
                                       JackPortIsOutput, 0);
                if (this->outputPorts[i] == nullptr)
                {
                    Lv2Log::error("Can't allocate Jack Audio output port.");
                    throw PiPedalStateException("Failed to allocate Jack Audio port.");
                }
            }
            auto &selectedMidiPorts = channelSelection.GetInputMidiPorts();

            midiLv2Buffers.resize(selectedMidiPorts.size());
            for (size_t i = 0; i < selectedMidiPorts.size(); ++i)
            {
                midiLv2Buffers[i] = AllocateRealtimeBuffer(MIDI_LV2_BUFFER_SIZE);
            }

            this->midiInputPorts.resize(selectedMidiPorts.size());
            for (int i = 0; i < selectedMidiPorts.size(); ++i)
            {
                std::stringstream name;
                name << "midiIn" << (i + 1);
                this->midiInputPorts[i] =
                    jack_port_register(client, name.str().c_str(),
                                       JACK_DEFAULT_MIDI_TYPE,
                                       JackPortIsInput, 0);
                if (this->midiInputPorts[i] == nullptr)
                {
                    std::stringstream s;
                    s << "can't register Jack port " << name.str().c_str();
                    Lv2Log::error(s.str());
                    throw PiPedalStateException(s.str().c_str());
                }
            }

            int activateResult = jack_activate(client);
            if (activateResult == 0)
            {
                active = true;

                for (int i = 0; i < selectedInputPorts.size(); ++i)
                {
                    auto result = jack_connect(client, selectedInputPorts[i].c_str(), jack_port_name(this->inputPorts[i]));
                    if (result)
                    {
                        Lv2Log::error("Can't connect input port %s", selectedInputPorts[i].c_str());
                        throw PiPedalStateException("Jack Audio port connection failed.");
                    }
                }
                for (int i = 0; i < selectedOutputPorts.size(); ++i)
                {
                    auto result = jack_connect(client, jack_port_name(this->outputPorts[i]), selectedOutputPorts[i].c_str());
                    if (result)
                    {
                        Lv2Log::error("Can't connect output port %s", selectedOutputPorts[i].c_str());
                        throw PiPedalStateException("Jack Audio port connection failed.");
                    }
                }
                for (int i = 0; i < selectedMidiPorts.size(); ++i)
                {
                    auto result = jack_connect(client, selectedMidiPorts[i].c_str(), jack_port_name(this->midiInputPorts[i]));
                    if (result)
                    {
                        Lv2Log::error("Can't connect midi input port %s", selectedMidiPorts[i].c_str());
                        throw PiPedalStateException("Jack Audio midi port connection failed.");
                    }
                }

                Lv2Log::info("Jack configuration complete.");
            }
            else
            {
                Lv2Log::error("Failed to activate Jack Audio client. (%d)", (int)activateResult);
                throw PiPedalStateException("Failed to activate Jack Audio client.");
            }
        }
        catch (PiPedalException &e)
        {
            Close();
            throw;
        }
    }

    void OnActivePedalBoardReleased(Lv2PedalBoard *pPedalBoard)
    {
        if (pPedalBoard)
        {
            pPedalBoard->Deactivate();
            std::lock_guard guard(mutex);

            for (auto it = activePedalBoards.begin(); it != activePedalBoards.end(); ++it)
            {
                if ((*it).get() == pPedalBoard)
                {
                    // erase it, relinquishing shared_ptr ownership, usually deleting the object.
                    activePedalBoards.erase(it);
                    return;
                }
            }
        }
    }

    virtual void SetPedalBoard(const std::shared_ptr<Lv2PedalBoard> &pedalBoard)
    {
        std::lock_guard guard(mutex);

        this->currentPedalBoard = pedalBoard;
        if (active)
        {
            pedalBoard->Activate();
            this->activePedalBoards.push_back(pedalBoard);
            hostWriter.ReplaceEffect(pedalBoard.get());
        }
    }

    virtual void SetBypass(uint64_t instanceId, bool enabled)
    {
        std::lock_guard guard(mutex);
        if (active && this->currentPedalBoard)
        {
            // use indices not instance ids, so we can just do a straight array index on the audio thread.
            auto index = currentPedalBoard->GetIndexOfInstanceId(instanceId);
            if (index >= 0)
            {
                hostWriter.SetBypass((uint32_t)index, enabled);
            }
        }
    }

    virtual void SetPluginPreset(uint64_t instanceId, const std::vector<ControlValue> &values)
    {
        std::lock_guard guard(mutex);
        if (active && this->currentPedalBoard)
        {
            auto effectIndex = currentPedalBoard->GetIndexOfInstanceId(instanceId);
            if (effectIndex != -1)
            {
                for (size_t i = 0; i < values.size(); ++i)
                {
                    const ControlValue &value = values[i];
                    int controlIndex = this->currentPedalBoard->GetControlIndex(instanceId, value.key());
                    if (controlIndex != -1 && effectIndex != -1)
                    {
                        hostWriter.SetControlValue(effectIndex, controlIndex, value.value());
                    }
                }
            }
        }
    }

    void SetControlValue(uint64_t  instanceId, const std::string &symbol, float value)
    {
        std::lock_guard guard(mutex);
        if (active && this->currentPedalBoard)
        {
            // use indices not instance ids, so we can just do a straight array index on the audio thread.
            int controlIndex = this->currentPedalBoard->GetControlIndex(instanceId, symbol);
            auto effectIndex = currentPedalBoard->GetIndexOfInstanceId(instanceId);

            if (controlIndex != -1 && effectIndex != -1)
            {
                hostWriter.SetControlValue(effectIndex, controlIndex, value);
            }
        }
    }

    virtual void SetVuSubscriptions(const std::vector<int64_t> &instanceIds)
    {
        std::lock_guard guard(mutex);

        if (active && this->currentPedalBoard)
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
                    auto effect = this->currentPedalBoard->GetEffect(instanceId);
                    if (!effect)
                    {
                        throw PiPedalStateException("Effect not found.");
                    }

                    int index = this->currentPedalBoard->GetIndexOfInstanceId(instanceIds[i]);
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
        result.instanceIndex = this->currentPedalBoard->GetIndexOfInstanceId(subscription.instanceid);
        IEffect *pEffect = this->currentPedalBoard->GetEffect(subscription.instanceid);

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
        if (this->currentPedalBoard == nullptr)
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
                if (this->currentPedalBoard->GetEffect(subscriptions[i].instanceid) != nullptr)
                {
                    pSubscriptions->subscriptions.push_back(
                        MakeRealtimeSubscription(subscriptions[i]));
                }
            }
            this->hostWriter.SetMonitorPortSubscriptions(pSubscriptions);
        }
    }

private:
    class RestartThread
    {
        JackHostImpl *this_;
        JackServerSettings jackServerSettings;
        std::function<void(bool success, const std::string &errorMessage)> onComplete;
        std::atomic<bool> isComplete = false;
        std::thread *pThread = nullptr;

    public:
        RestartThread(
            JackHostImpl *host,
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
            try {
                ShutdownClient::SetJackServerConfiguration(jackServerSettings);
                //this_->Open(this_->channelSelection);
                this_->restarting = false;
                onComplete(true, "");
                isComplete = true;
            } catch (const std::exception &e)
            {
                onComplete(false,e.what());
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

    virtual void getRealtimeParameter(RealtimeParameterRequest *pParameterRequest)
    {
        if (!active)
        {
            pParameterRequest->errorMessage = "Not active.";
            pParameterRequest->onJackRequestComplete(pParameterRequest);
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

        result.active_ = this->active;
        result.restarting_ = this->restarting;

        if (client != nullptr)
        {
            result.cpuUsage_ = jack_cpu_load(this->client);
        }
        else
        {
            result.cpuUsage_ = 0;
        }
        GetCpuFrequency(&result.cpuFreqMax_,&result.cpuFreqMin_);
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

int JackHostImpl::jackInstanceId = 0;

JackHost *JackHost::CreateInstance(IHost *pHost)
{
    return new JackHostImpl(pHost);
}

JSON_MAP_BEGIN(JackHostStatus)
JSON_MAP_REFERENCE(JackHostStatus, active)
JSON_MAP_REFERENCE(JackHostStatus, restarting)
JSON_MAP_REFERENCE(JackHostStatus, underruns)
JSON_MAP_REFERENCE(JackHostStatus, cpuUsage)
JSON_MAP_REFERENCE(JackHostStatus, msSinceLastUnderrun)
JSON_MAP_REFERENCE(JackHostStatus, temperaturemC)
JSON_MAP_REFERENCE(JackHostStatus, cpuFreqMin)
JSON_MAP_REFERENCE(JackHostStatus, cpuFreqMax)
JSON_MAP_REFERENCE(JackHostStatus, governor)
JSON_MAP_END()
