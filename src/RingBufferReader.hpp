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

#include "PiPedalException.hpp"
#include "Lv2Log.hpp"
#include "VuUpdate.hpp"
#include "JackHost.hpp"
namespace pipedal
{

    enum class RingBufferCommand : int64_t
    {
        ReplaceEffect = 0,
        EffectReplaced = 1,
        SetValue = 2,
        SetBypass = 3,
        AudioStopped = 4,
        SetVuSubscriptions = 5,
        FreeVuSubscriptions = 6,
        SendVuUpdate = 7,
        AckVuUpdate = 8,

        SetMonitorPortSubscription = 9,
        FreeMonitorPortSubscription = 10,
        SendMonitorPortUpdate = 11,
        AckMonitorPortUpdate = 12,
        ParameterRequest = 13,
        ParameterRequestComplete = 14,

        MidiValueChanged = 15,

        OnMidiListen = 16,

        AtomOutput = 17,

    };

    class RealtimeMonitorPortSubscription
    {
    public:
        RealtimeMonitorPortSubscription() { delete callbackPtr; }
        int64_t subscriptionHandle;
        int instanceIndex = 0;
        int portIndex = 0;
        PortMonitorCallback *callbackPtr = nullptr;
        int sampleRate = 0;
        int samplesToNextCallback = 0;
        bool waitingForAck = false;
        float lastValue = -1E30;
    };

    class RealtimeMonitorPortSubscriptions
    {
    public:
        std::vector<RealtimeMonitorPortSubscription> subscriptions;
    };

    struct RealtimeVuBuffers
    {
        RealtimeVuBuffers()
        {
            Reset();
        }
        bool waitingForAcknowledge = false;

        const std::vector<VuUpdate> *GetResult(size_t currentSample)
        {
            for (size_t i = 0; i < vuUpdateWorkingData.size(); ++i)
            {
                vuUpdateResponseData[i] = vuUpdateWorkingData[i];
                vuUpdateResponseData[i].sampleTime_ = currentSample;
                vuUpdateWorkingData[i].reset();
            }
            return &vuUpdateResponseData;
        }

        std::vector<int> enabledIndexes;
        std::vector<VuUpdate> vuUpdateWorkingData;
        std::vector<VuUpdate> vuUpdateResponseData;

        void Reset()
        {
            for (int i = 0; i < vuUpdateWorkingData.size(); ++i)
            {
                vuUpdateWorkingData[i].reset();
            }
        }
    };

    class AudioStoppedBody
    {
        bool dummy = true;
    };

    class SetBypassBody
    {
    public:
        int effectIndex;
        bool enabled;
    };

    class MidiValueChangedBody
    {
    public:
        int64_t instanceId;
        int controlIndex;
        float value;
    };


    class SetControlValueBody
    {
    public:
        int effectIndex;
        int controlIndex;
        float value;
    };

    class Lv2PedalBoard;

    class ReplaceEffectBody
    {
    public:
        Lv2PedalBoard *effect;
    };
    class EffectReplacedBody
    {
    public:
        Lv2PedalBoard *oldEffect;
    };

    template <bool MULTI_WRITE, bool SEMAPHORE_READ>
    class RingBufferReader
    {

    private:
        RingBuffer<MULTI_WRITE,SEMAPHORE_READ> *ringBuffer = nullptr;

    public:
        RingBufferReader()
            : ringBuffer(nullptr)
        {
        }
        RingBufferReader(RingBuffer<MULTI_WRITE,SEMAPHORE_READ> *ringBuffer)
            : ringBuffer(ringBuffer)
        {
        }

        // 0 -> ready. -1: timed out. -2: closing.
        int wait(struct timespec &timeout) {
            return ringBuffer->readWait(timeout);
        }

        bool wait() {
            return ringBuffer->readWait();
        }
        size_t readSpace() const
        {
            return ringBuffer->readSpace();
        }
        template <typename T>
        bool read(T *output)
        {
            size_t available = readSpace();
            if (available < sizeof(T))
                return false;
            if (!ringBuffer->read(sizeof(T),(uint8_t*)output))
            {
                throw PiPedalStateException("Ringbuffer read failed. Did you forget to check for space?");
            }
            return true;
        }

        bool read(size_t size, uint8_t*data)
        {
            if (!ringBuffer->read(size,data))
            {
                throw PiPedalStateException("Ringbuffer read failed. Did you forget to check for space?");
            }
            return true;

        }
        template <typename T>
        void readComplete(T *output)
        {
            if (!ringBuffer->read(sizeof(T),(uint8_t*)output))
            {
                throw PiPedalStateException("Ringbuffer read failed. Did you forget to check for space?");
            }
        }



    };

    template <typename T>
    class CommandBuffer
    {

    public:
        CommandBuffer(RingBufferCommand command)
            : command(command)
        {
        }

        CommandBuffer(RingBufferCommand command, const T &value)
            : command(command), value(value)
        {
        }
        RingBufferCommand command;

        size_t size() const { return sizeof(RingBufferCommand) + sizeof(T); }

        T value;
    };

    template<bool MULTI_WRITER, bool SEMAPHORE_READER>
    class RingBufferWriter
    {

    private:
        RingBuffer<MULTI_WRITER,SEMAPHORE_READER>* ringBuffer;

    public:
        RingBufferWriter()
            : ringBuffer(nullptr)
        {
        }
        RingBufferWriter(RingBuffer<MULTI_WRITER,SEMAPHORE_READER>*ringBuffer)
            : ringBuffer(ringBuffer)
        {
        }

        template <typename T>
        void write(RingBufferCommand command, const T &value)
        {

            // the goal: to atomically write the command and associated data.
            CommandBuffer<T> buffer(command, value);

            if (!ringBuffer->write(buffer.size(),(uint8_t *)&buffer))
            {
                Lv2Log::error("No space in audio service ringbuffer.");
                return;

            }
        }
        template <typename T>
        void write(RingBufferCommand command, const T &value, size_t dataLength, uint8_t*variableData)
        {

            // the goal: to atomically write the command and associated data.
            CommandBuffer<T> buffer(command, value);

            if (!ringBuffer->write(buffer.size(),(uint8_t *)&buffer,dataLength,variableData))
            {
                Lv2Log::error("No space in audio service ringbuffer.");
                return;

            }
        }
        void AtomOutput(uint64_t instanceId, size_t bytes, uint8_t*data)
        {
            write(RingBufferCommand::AtomOutput,instanceId,bytes,data);
        }
        void ParameterRequest(RealtimeParameterRequest *pRequest)
        {
            write(RingBufferCommand::ParameterRequest,pRequest);
        }
        void ParameterRequestComplete(RealtimeParameterRequest *pRequest)
        {
            write(RingBufferCommand::ParameterRequestComplete,pRequest);
        }
        void MidiValueChanged(int64_t instanceId, int controlIndex,float value)
        {
            MidiValueChangedBody body;
            body.instanceId = instanceId;
            body.controlIndex = controlIndex;
            body.value = value;
            write(RingBufferCommand::MidiValueChanged,body);            
        }

        void OnMidiListen(bool isNote, uint8_t noteOrControl) {
            uint16_t msg = noteOrControl;
            if (isNote) msg |= 0x100;
            write(RingBufferCommand::OnMidiListen, msg);
        }

        void SetControlValue(int effectIndex, int controlIndex, float value)
        {
            SetControlValueBody body;
            body.effectIndex = effectIndex;
            body.controlIndex = controlIndex;
            body.value = value;
            write(RingBufferCommand::SetValue,body);
        }

        void FreeVuSubscriptions(RealtimeVuBuffers *configuration)
        {
            write(RingBufferCommand::FreeVuSubscriptions, configuration);
        }
        void FreeMonitorPortSubscriptions(RealtimeMonitorPortSubscriptions *subscriptions)
        {

            write(RingBufferCommand::FreeMonitorPortSubscription, subscriptions);
        }

        void SendMonitorPortUpdate(
            PortMonitorCallback* callback,
            int64_t subscriptionHandle,
            float value)
        {
            MonitorPortUpdate body{callback, subscriptionHandle, value};
            write(RingBufferCommand::SendMonitorPortUpdate, body);
        }

        void SendVuUpdate(const std::vector<VuUpdate> *pUpdates)
        {
            write(RingBufferCommand::SendVuUpdate,pUpdates);
        }
        void AckVuUpdate()
        {
            bool value = true;
            write(RingBufferCommand::AckVuUpdate,value);
        }
        void AckMonitorPortUpdate(int64_t subscriptionHandle)
        {
            // we assume no padding between the command and the data, so we can do an atomic write.

            write(RingBufferCommand::AckMonitorPortUpdate, subscriptionHandle);
        }
        void SetVuSubscriptions(RealtimeVuBuffers *configuration)
        {
            write(RingBufferCommand::SetVuSubscriptions, configuration);
        }

        void SetMonitorPortSubscriptions(RealtimeMonitorPortSubscriptions *subscriptions)
        {

            write(RingBufferCommand::SetMonitorPortSubscription, subscriptions);
        }

        void SetBypass(int effectIndex, bool enabled)
        {

            SetBypassBody body;
            body.effectIndex = effectIndex;
            body.enabled = enabled;
            write(RingBufferCommand::SetBypass,body);
        }

        void ReplaceEffect(Lv2PedalBoard *pedalBoard)
        {
            write(RingBufferCommand::ReplaceEffect, pedalBoard);
        }

        void AudioStopped()
        {
            AudioStoppedBody body;
            write(RingBufferCommand::AudioStopped, body);
        }

        void EffectReplaced(Lv2PedalBoard *pedalBoard)
        {
            write(RingBufferCommand::EffectReplaced, pedalBoard);
        }
    };

    typedef RingBufferReader<true, false> RealtimeRingBufferReader;
    typedef RingBufferReader<false, true> HostRingBufferReader;
    typedef RingBufferWriter<true, false> HostRingBufferWriter;

    // cures a forward-declaration problem.
    class RealtimeRingBufferWriter: public RingBufferWriter<false, true> 
    {
    public:
        RealtimeRingBufferWriter()
        {
        }
        RealtimeRingBufferWriter(RingBuffer<false,true>*ringBuffer)
            : RingBufferWriter<false, true> (ringBuffer)
        {
        }



    };


} //namespace