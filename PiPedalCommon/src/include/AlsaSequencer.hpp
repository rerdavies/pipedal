/*
 *   Copyright (c) 2025 Robin E. R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.

 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#pragma once

#include <vector>
#include <string>
#include <memory>

namespace pipedal
{

    // event messages starting with 0xFF are meta events. 
    // data not in midi format, is variable length, and private and the second byte is the MetaEventType.
    enum class MetaEventType
    {
        None,
        Tempo,
        TimeSignature,
        KeySignature,
        SetPositionTick,
        SetPositionTime
    };

    struct AlsaSequencerPort
    {
        std::string id;
        int client;
        int port;
        std::string name;
        std::string clientName;
        bool canRead = false;
        bool canWrite = false;
        bool canReadSubscribe = false;
        bool canWriteSubscribe = false;

        bool isKernelDevice = false;
        bool isSystemAnnounce = false;
        bool isUmp = false;
        bool isMidi = false;
        bool isApplication = false;
        bool isMidiSynth = false;
        bool isSynth = false;
        bool isSpecific;
        bool isHardware = false;
        bool isSoftware = false;
        bool isPort = false;
        bool isVirtual = false;
        int cardNumber = -1;
        std::string rawMidiDevice; // e.g. "hw:0,0,0" for kernel devices
    };

    struct AlsaMidiMessage
    {
        AlsaMidiMessage() 
        {
            size = 0;
            data = fixedBuffer;
        }
        AlsaMidiMessage(uint8_t cc0)
        {
            size = 1;
            data = fixedBuffer;
            fixedBuffer[0] = cc0;
        }
        AlsaMidiMessage(uint8_t cc0, uint8_t cc1)
        {
            size = 2;
            data = fixedBuffer;
            fixedBuffer[0] = cc0;
            fixedBuffer[1] = cc1;
        }
        AlsaMidiMessage(uint8_t cc0, uint8_t cc1, uint8_t cc2)
        {
            size = 3;
            data = fixedBuffer;
            fixedBuffer[0] = cc0;
            fixedBuffer[1] = cc1;
            fixedBuffer[2] = cc2;
        }
        AlsaMidiMessage(uint8_t *data, size_t size)
        {
            this->size = size;
            this->data = data;
        }
        AlsaMidiMessage(const AlsaMidiMessage &other)
        {
            this->timestamp = other.timestamp;
            this->realtime_sec = other.realtime_sec;
            this->realtime_nsec = other.realtime_nsec;

            if (other.size > 3)
            {
                this->size = other.size;
                this->data = other.data;
            }
            else
            {
                this->size = other.size;
                this->data = fixedBuffer;
                for (size_t i = 0; i < other.size; ++i)
                {
                    fixedBuffer[i] = other.fixedBuffer[i];
                }
            }
        }
        AlsaMidiMessage &operator=(const AlsaMidiMessage &other)
        {
            this->timestamp = other.timestamp;
            this->realtime_sec = other.realtime_sec;
            this->realtime_nsec = other.realtime_nsec;

            if (other.size > 3)
            {
                this->size = other.size;
                this->data = other.data;
            }
            else
            {
                this->size = other.size;
                this->data = fixedBuffer;
                for (size_t i = 0; i < other.size; ++i)
                {
                    fixedBuffer[i] = other.fixedBuffer[i];
                }
            }
            return *this;
        }
        AlsaMidiMessage(AlsaMidiMessage &&other)
        {
            this->timestamp = other.timestamp;
            this->realtime_sec = other.realtime_sec;
            this->realtime_nsec = other.realtime_nsec;

            if (other.size > 3)
            {
                this->size = other.size;
                this->data = other.data;
            }
            else
            {
                this->size = other.size;
                this->data = fixedBuffer;
                for (size_t i = 0; i < other.size; ++i)
                {
                    fixedBuffer[i] = other.fixedBuffer[i];
                }
            }
        }
        AlsaMidiMessage &operator=(AlsaMidiMessage &&other)
        {
            this->timestamp = other.timestamp;
            this->realtime_sec = other.realtime_sec;
            this->realtime_nsec = other.realtime_nsec;

            if (other.size > 3)
            {
                this->size = other.size;
                this->data = other.data;
            }
            else
            {
                this->size = other.size;
                this->data = fixedBuffer;
                for (size_t i = 0; i < other.size; ++i)
                {
                    fixedBuffer[i] = other.fixedBuffer[i];
                }
            }
            return *this;
        }

        void Set(uint8_t cc0)
        {
            size = 1;
            data = fixedBuffer;
            fixedBuffer[0] = cc0;
        }
        void Set(uint8_t cc0, uint8_t cc1)
        {
            size = 2;
            data = fixedBuffer;
            fixedBuffer[0] = cc0;
            fixedBuffer[1] = cc1;
        }
        void Set(uint8_t cc0, uint8_t cc1, uint8_t cc2)
        {
            size = 3;
            data = fixedBuffer;
            fixedBuffer[0] = cc0;
            fixedBuffer[1] = cc1;
            fixedBuffer[2] = cc2;
        }
        void Set(uint8_t *data, size_t size)
        {
            this->size = size;
            this->data = data;
        }
        uint8_t cc0() const { return size > 0 ? data[0] : 0; }
        uint8_t cc1() const { return size > 1 ? data[1] : 0; }
        uint8_t cc2() const { return size > 2 ? data[2] : 0; }


        uint32_t timestamp;     // in microseconds (tick time if using queue)
        uint64_t realtime_sec;  // real-time timestamp seconds
        uint32_t realtime_nsec; // real-time timestamp nanoseconds
        size_t size = 0;
        uint8_t *data = fixedBuffer;

        uint8_t fixedBuffer[12];
    };

    /*
     * AlsaSequencer - ALSA MIDI sequencer interface with real-time timestamp support
     *
     * Example usage with real-time timestamps:
     *
     *   AlsaSequencer sequencer;
     *   sequencer.connect(clientId, portId);
     *
     *   AlsaMidiMessage message;
     *   while (sequencer.ReadMessage(&message)) {
     *       // message.realtime_sec and message.realtime_nsec contain precise real-time timestamps
     *       // message.timestamp contains the queue tick time
     *       printf("MIDI event at %lu.%09u seconds\n", message.realtime_sec, message.realtime_nsec);
     *   }
     */

    class AlsaSequencer
    {
    protected:
        AlsaSequencer() {}

    public:
        virtual ~AlsaSequencer() = default;

        using self = AlsaSequencer;
        using ptr = std::shared_ptr<self>;

        static ptr Create();

        static std::vector<AlsaSequencerPort> EnumeratePorts();

    public:
        virtual void ConnectPort(int clientId, int portId) = 0;
        virtual void ConnectPort(const std::string &id) = 0;

        // Read a single MIDI message from the sequencer input port. A timeout of -1 blocks indefinitely.
        // A timeout of 0 returns immediately.
        virtual bool ReadMessage(AlsaMidiMessage &message, int timeoutMs = -1) = 0;

        // currently non-functional
        virtual bool GetQueueRealtime(uint64_t *sec, uint32_t *nsec) = 0;

        virtual void RemoveAllConnections() = 0;
    };

    std::string RawMidiIdToSequencerId(const std::vector<AlsaSequencerPort> &seqDevices, const std::string &rawMidiId);

}