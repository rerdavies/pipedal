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
#include <alsa/asoundlib.h>
#include <alsa/seq.h>
#include <stdexcept>

namespace pipedal
{
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
        int cardNumber = -1;
        std::string rawMidiDevice; // e.g. "hw:0,0,0" for kernel devices
    };

    struct AlsaMidiMessage
    {
        uint32_t timestamp; // in microseconds (tick time if using queue)
        uint64_t realtime_sec;  // real-time timestamp seconds
        uint32_t realtime_nsec; // real-time timestamp nanoseconds
        uint8_t cc0;
        uint8_t cc1;
        uint8_t cc2;
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
    public:
        static std::vector<AlsaSequencerPort> EnumeratePorts();

        AlsaSequencer();
        ~AlsaSequencer();

        void ConnectPort(int clientId, int portId);
        void ConnectPort(const std::string&name);

        // Read a single MIDI message from the sequencer input port
        bool ReadMessage(AlsaMidiMessage &message);


        // Get current real-time from the queue (useful for calculating precise timing)
        bool GetQueueRealtime(uint64_t* sec, uint32_t* nsec);

        // Get the current queue ID (returns -1 if no queue is active)
        int GetQueueId() const { return queueId; }

    private:
        // Create an ALSA input queue with real-time timestamps for the given client/port
        int CreateRealtimeInputQueue();

        struct Connection
        {
            int clientId;
            int portId;
        };

        std::vector<Connection> connections;

        snd_seq_t *seqHandle = nullptr;
        int inPort = -1;
        int queueId = -1;  // Queue for real-time timestamps

        // Example: open an ALSA sequencer input port and read MIDI events continuously
        void ReadMidiFromPort(int clientId, int portId);
    };

}