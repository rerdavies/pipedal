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

#include "AlsaSequencer.hpp"
#include <vector>
#include <string>
#include "ss.hpp"

// enumerate alsa sequencer ports

namespace pipedal
{
    std::vector<AlsaSequencerPort> AlsaSequencer::EnumeratePorts()
    {
        std::vector<AlsaSequencerPort> ports;

        snd_seq_t *seq;
        if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_INPUT, 0) < 0)
        {
            return ports;
        }

        snd_seq_client_info_t *client_info;
        snd_seq_port_info_t *port_info;

        snd_seq_client_info_alloca(&client_info);
        snd_seq_port_info_alloca(&port_info);

        snd_seq_client_info_set_client(client_info, -1);

        while (snd_seq_query_next_client(seq, client_info) >= 0)
        {
            int client = snd_seq_client_info_get_client(client_info);

            snd_seq_port_info_set_client(port_info, client);
            snd_seq_port_info_set_port(port_info, -1);

            while (snd_seq_query_next_port(seq, port_info) >= 0)
            {
                unsigned int capability = snd_seq_port_info_get_capability(port_info);

                // Skip system ports and ports without MIDI capability
                if (client == SND_SEQ_CLIENT_SYSTEM ||
                    !(capability & (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_WRITE)))
                {
                    continue;
                }
                if ((capability & (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ)) != (SND_SEQ_PORT_CAP_READ | SND_SEQ_PORT_CAP_SUBS_READ))
                {
                    continue; // Skip ports that are not readable AND subscribable
                }

                AlsaSequencerPort port;
                port.client = client;
                port.isKernelDevice = snd_seq_client_info_get_type(client_info) == SND_SEQ_KERNEL_CLIENT;
                port.port = snd_seq_port_info_get_port(port_info);
                port.name = snd_seq_port_info_get_name(port_info);
                port.clientName = snd_seq_client_info_get_name(client_info);
                port.canRead = capability & SND_SEQ_PORT_CAP_READ;
                port.canWrite = capability & SND_SEQ_PORT_CAP_WRITE;
                port.canReadSubscribe = capability & SND_SEQ_PORT_CAP_SUBS_READ;
                port.canWriteSubscribe = capability & SND_SEQ_PORT_CAP_SUBS_WRITE;
                auto typeBits = snd_seq_port_info_get_type(port_info);

                port.isUmp = (typeBits & SND_SEQ_PORT_TYPE_MIDI_UMP) != 0;
                port.isSystemAnnounce = (typeBits & SND_SEQ_PORT_SYSTEM_ANNOUNCE) != 0;
                port.isMidiSynth = (typeBits & SND_SEQ_PORT_TYPE_MIDI_GENERIC) != 0 ||
                                   (typeBits & SND_SEQ_PORT_TYPE_MIDI_GM) != 0 ||
                                   (typeBits & SND_SEQ_PORT_TYPE_MIDI_GS) != 0 ||
                                   (typeBits & SND_SEQ_PORT_TYPE_MIDI_XG) != 0;
                port.isApplication = (typeBits & SND_SEQ_PORT_TYPE_APPLICATION) != 0;
                port.isSpecific = (typeBits & SND_SEQ_PORT_TYPE_SPECIFIC) != 0;
                port.isSynth = (typeBits & SND_SEQ_PORT_TYPE_SYNTH) != 0;
                port.isHardware = (typeBits & SND_SEQ_PORT_TYPE_HARDWARE) != 0;
                port.isSoftware = (typeBits & SND_SEQ_PORT_TYPE_SOFTWARE) != 0;

                port.cardNumber = snd_seq_client_info_get_card(client_info);
                port.port = snd_seq_port_info_get_port(port_info);

                if (port.isKernelDevice & port.cardNumber >= 0 && port.port >= 0)
                {
                    // For kernel devices, we can construct a raw MIDI device string
                    port.rawMidiDevice = SS("hw:" << port.cardNumber << ",0," << port.port);
                }
                port.id = SS("seq:" << port.clientName << "/" << port.name);

                ports.push_back(port);
            }
        }

        snd_seq_close(seq);
        return ports;
    }

    AlsaSequencer::AlsaSequencer()
    {
        seqHandle = nullptr;
        queueId = -1;
        // Open sequencer in input mode
        int rc;

        rc = snd_seq_open(&seqHandle, "default", SND_SEQ_OPEN_DUPLEX, 0);
        if (rc < 0)
        {
            // convert rc to message
            throw std::runtime_error(SS("Failed to open ALSA sequencer:" << snd_strerror(rc)));
        }
        snd_seq_set_client_name(seqHandle, "PiPedal");

        inPort = snd_seq_create_simple_port(seqHandle, "PiPedal:in",
                                        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                                        SND_SEQ_PORT_TYPE_MIDI_GENERIC |
                                            SND_SEQ_PORT_TYPE_MIDI_GM | SND_SEQ_PORT_TYPE_APPLICATION);
        if (inPort < 0)
        {
            // convert rc to message
            throw std::runtime_error(SS("Failed to open ALSA sequencer:" << snd_strerror(inPort)));
        }
    }
    AlsaSequencer::~AlsaSequencer()
    {
        for (const auto &connection : connections)
        {
            snd_seq_disconnect_from(seqHandle, inPort, connection.clientId, connection.portId);
        }
        if (queueId >= 0)
        {
            snd_seq_free_queue(seqHandle, queueId);
            queueId = -1;
        }
        if (inPort >= 0)
        {
            snd_seq_delete_port(seqHandle, inPort);
            inPort = -1;
        }
        if (seqHandle)
        {
            snd_seq_close(seqHandle);
            seqHandle = nullptr;
        }
    }

    void AlsaSequencer::ConnectPort(int clientId, int portId)
    {

        CreateRealtimeInputQueue();
        // Connect this input port to the target client:port
        int rc = snd_seq_connect_from(seqHandle, inPort, clientId, portId);
        if (rc < 0)
        {
            throw std::runtime_error(SS("Failed to connect to ALSA port: " << snd_strerror(rc)));
        }
        this->connections.push_back({clientId, portId});
    }

    void AlsaSequencer::ConnectPort(const std::string &name)
    {
        auto ports = EnumeratePorts();
        for (const auto &port : ports)
        {
            if (port.name == name)
            {
                ConnectPort(port.client, port.port);
                return;
            }
        }
        throw std::runtime_error("ALSA port not found");
    }

    bool AlsaSequencer::ReadMessage(AlsaMidiMessage &message)
    {
        // Event loop
        snd_seq_event_t *event = nullptr;
        bool success = false;
        if (snd_seq_event_input(seqHandle, &event) >= 0 && event)
        {
            success = true;
            // Extract timestamp information
            message.timestamp = event->time.tick;
            message.realtime_sec = event->time.time.tv_sec;
            message.realtime_nsec = event->time.time.tv_nsec;

            // Process MIDI event here, e.g. NOTEON, NOTEOFF, etc.
            switch (event->type)
            {
            case SND_SEQ_EVENT_NOTEON:
                message.cc0 = 0x90 | event->data.note.channel; // channel
                message.cc1 = event->data.note.note;           // note
                message.cc2 = event->data.note.velocity;       // velocity
                break;
            case SND_SEQ_EVENT_NOTEOFF:
                // handle note-off
                message.cc0 = 0x80 | event->data.note.channel; // channel
                message.cc1 = event->data.note.note;           // note
                message.cc2 = event->data.note.off_velocity;   // off velocity
                break;
            case SND_SEQ_EVENT_KEYPRESS:
                message.cc0 = 0xA0 | event->data.note.channel; // polyphonic key pressure
                message.cc1 = event->data.note.note;           // note
                message.cc2 = event->data.note.velocity;       // pressure
                break;
            case SND_SEQ_EVENT_CONTROLLER:
                message.cc0 = 0xB0 | event->data.control.channel; // control change
                message.cc1 = event->data.control.param;          // controller number
                message.cc2 = event->data.control.value;          // controller value
                break;
            case SND_SEQ_EVENT_PGMCHANGE:
                message.cc0 = 0xC0 | event->data.control.channel; // program change
                message.cc1 = event->data.control.value;          // program number
                message.cc2 = 0;                                  // unused
                break;
            case SND_SEQ_EVENT_CHANPRESS:
                message.cc0 = 0xD0 | event->data.control.channel; // channel pressure
                message.cc1 = event->data.control.value;          // pressure value
                message.cc2 = 0;                                  // unused
                break;
            case SND_SEQ_EVENT_PITCHBEND:
                message.cc0 = 0xE0 | event->data.control.channel;      // pitch bend
                message.cc1 = (event->data.control.value >> 7) & 0x7F; // MSB
                message.cc2 = event->data.control.value & 0x7F;        // LSB
                break;
            case SND_SEQ_EVENT_CONTROL14:
            case SND_SEQ_EVENT_NONREGPARAM:
            case SND_SEQ_EVENT_REGPARAM:
            case SND_SEQ_EVENT_SONGPOS:
            default:
                success = false;
                break;
            }
            snd_seq_free_event(event);
        }
        return success;
    }

    int AlsaSequencer::CreateRealtimeInputQueue()
    {
        if (!seqHandle)
        {
            throw std::runtime_error("ALSA sequencer not initialized");
        }

        // Create a new queue if we don't have one yet
        if (queueId < 0)
        {
            queueId = snd_seq_alloc_named_queue(seqHandle, "PiPedal Realtime Queue");
            if (queueId < 0)
            {
                throw std::runtime_error(SS("Failed to create ALSA queue: " << snd_strerror(queueId)));
            }

            // Set queue timing to real-time mode
            snd_seq_queue_tempo_t *tempo;
            snd_seq_queue_tempo_alloca(&tempo);
            snd_seq_queue_tempo_set_tempo(tempo, 120); // 120 BPM default
            snd_seq_queue_tempo_set_ppq(tempo, 96);    // 96 ticks per quarter note

            int rc = snd_seq_set_queue_tempo(seqHandle, queueId, tempo);
            if (rc < 0)
            {
                snd_seq_free_queue(seqHandle, queueId);
                queueId = -1;
                throw std::runtime_error(SS("Failed to set queue tempo: " << snd_strerror(rc)));
            }

            // Start the queue
            rc = snd_seq_start_queue(seqHandle, queueId, nullptr);
            if (rc < 0)
            {
                snd_seq_free_queue(seqHandle, queueId);
                queueId = -1;
                throw std::runtime_error(SS("Failed to start queue: " << snd_strerror(rc)));
            }

 

            // Set the queue for input timestamping
            snd_seq_port_info_t *port_info;
            snd_seq_port_info_alloca(&port_info);

            rc = snd_seq_get_port_info(seqHandle, inPort, port_info);
            if (rc < 0)
            {
                snd_seq_free_queue(seqHandle, queueId);
                queueId = -1;
                throw std::runtime_error(SS("Failed to get port info: " << snd_strerror(rc)));
            }

            // Enable timestamping on the input port
            snd_seq_port_info_set_timestamping(port_info, 1);
            snd_seq_port_info_set_timestamp_real(port_info, 1);
            snd_seq_port_info_set_timestamp_queue(port_info, queueId);

            rc = snd_seq_set_port_info(seqHandle, inPort, port_info);
            if (rc < 0)
            {
                snd_seq_free_queue(seqHandle, queueId);
                queueId = -1;
                throw std::runtime_error(SS("Failed to set port timestamping: " << snd_strerror(rc)));
            }
        }

        return queueId;
    }
    bool AlsaSequencer::GetQueueRealtime(uint64_t *sec, uint32_t *nsec)
    {
        if (!seqHandle || queueId < 0)
        {
            return false;
        }

        snd_seq_queue_status_t *status;
        snd_seq_queue_status_alloca(&status);

        int rc = snd_seq_get_queue_status(seqHandle, queueId, status);
        if (rc < 0)
        {
            return false;
        }

        const snd_seq_real_time_t *realtime = snd_seq_queue_status_get_real_time(status);
        if (sec)
            *sec = realtime->tv_sec;
        if (nsec)
            *nsec = realtime->tv_nsec;

        return true;
    }
}
