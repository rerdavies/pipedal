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
#include <regex>
#include "ss.hpp"
#include <alsa/asoundlib.h>
#include <alsa/seq.h>
#include <stdexcept>
#include <poll.h>
#include <cstring>
#include "Finally.hpp"
#include "Lv2Log.hpp"
#include <mutex>
#include <thread>

// enumerate alsa sequencer ports

namespace pipedal
{

    namespace impl
    {

        enum class ConnectAction
        {
            Subscribe,
            Unsubscribe
        };

        class AlsaSequencerImpl : public AlsaSequencer
        {
        public:
            AlsaSequencerImpl();

        public:
            ~AlsaSequencerImpl();

            virtual void ConnectPort(int clientId, int portId) override;
            virtual void ConnectPort(const std::string &name) override;
            virtual void SetConfiguration(const AlsaSequencerConfiguration &alsaSequencerConfiguration) override;

            // Read a single MIDI message from the sequencer input port. A timeout of -1 blocks indefinitely.
            // A timeout of 0 returns immediately.
            virtual bool ReadMessage(AlsaMidiMessage &message, int timeoutMs = -1) override;

            // Get current real-time from the queue (useful for calculating precise timing)
            virtual bool GetQueueRealtime(uint64_t *sec, uint32_t *nsec) override;

            virtual void RemoveAllConnections() override;

        private:
            void ModifyConnection(int clientId, int portId, ConnectAction action);

            // Get the current queue ID (returns -1 if no queue is active)
            int GetQueueId() const { return queueId; }

            bool WaitForMessage(int timeoutMs);
            // Create an ALSA input queue with real-time timestamps for the given client/port
            int CreateRealtimeInputQueue();

            struct Connection
            {
                int clientId;
                int portId;
            };

            int myClientId = -1;

            std::mutex connectionsMutex;
            std::vector<Connection> connections;
            std::vector<struct pollfd> pollFds; // For polling input events
            snd_seq_t *seqHandle = nullptr;
            int inPort = -1;
            int queueId = -1; // Queue for real-time timestamps
        };

        class AlsaSequencerDeviceMonitorImpl : public AlsaSequencerDeviceMonitor
        {

        public:
            virtual ~AlsaSequencerDeviceMonitorImpl() override;

            virtual void StartMonitoring(
                Callback &&onChangeCallback) override;
            virtual void StopMonitoring() override;

        private:
            int CreateInputQueue(snd_seq_t *seqHandle, int inPort);

            bool started = false;
            void ServiceProc();
            std::unique_ptr<std::jthread> serviceThread;
            std::atomic<bool> terminateThread{false};
            Callback callback;
        };

    };
    using namespace impl;

    AlsaSequencer::ptr AlsaSequencer::Create()
    {
        return std::make_shared<AlsaSequencerImpl>();
    }

    std::vector<AlsaSequencerPort> AlsaSequencer::EnumeratePorts()
    {
        std::vector<AlsaSequencerPort> ports;

        snd_seq_t *seq;
        if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0)
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
#ifdef SND_SEQ_PORT_TYPE_MIDI_UMP
                port.isUmp = (typeBits & SND_SEQ_PORT_TYPE_MIDI_UMP) != 0;
#else
                port.isUmp = false; // UMP support is not available in all versions of ALSA
#endif
                port.isSystemAnnounce = (typeBits & SND_SEQ_PORT_SYSTEM_ANNOUNCE) != 0;
                port.isMidiSynth = (typeBits & SND_SEQ_PORT_TYPE_MIDI_GENERIC) != 0 ||
                                   (typeBits & SND_SEQ_PORT_TYPE_MIDI_GM) != 0 ||
                                   (typeBits & SND_SEQ_PORT_TYPE_MIDI_GS) != 0 ||
                                   (typeBits & SND_SEQ_PORT_TYPE_MIDI_XG) != 0;
                port.isApplication = (typeBits & SND_SEQ_PORT_TYPE_APPLICATION) != 0;
                port.isSpecific = (typeBits & SND_SEQ_PORT_TYPE_SPECIFIC) != 0;
                port.isSynth = (typeBits & SND_SEQ_PORT_TYPE_SYNTH) != 0;
                port.isHardware = (typeBits & SND_SEQ_PORT_TYPE_HARDWARE) != 0;
                port.isPort = (typeBits & SND_SEQ_PORT_TYPE_PORT) != 0;
                port.isSoftware = (typeBits & SND_SEQ_PORT_TYPE_SOFTWARE) != 0;
                port.isVirtual = port.name.starts_with("VirMIDI");
                port.cardNumber = snd_seq_client_info_get_card(client_info);
                port.port = snd_seq_port_info_get_port(port_info);

                if (port.isKernelDevice && port.isPort && port.cardNumber >= 0 && port.port >= 0)
                {
                    // For kernel devices, we can construct a raw MIDI device string
                    std::string rawMidiDevice;
                    if (port.isVirtual)
                    {
                        // "VirMidI 2-1"
                        std::regex virtualDeviceRegex{"^(VirMIDI) (\\d+)-(\\d+)$"};
                        {
                            // Extract the card and device numbers from the match
                            std::smatch match;
                            std::regex_search(port.name, match, virtualDeviceRegex);
                            if (match.size() == 4)
                            {
                                try
                                {
                                    std::string devName = match[1];
                                    int card = std::stoi(match[2]);
                                    int device = std::stoi(match[3]);
                                    rawMidiDevice = SS("hw:CARD=" << devName << ",DEV=" << device);
                                }
                                catch (const std::exception &ignored)
                                {
                                }
                            }
                        }
                    }
                    else
                    {
                        rawMidiDevice = SS("hw:CARD=" << port.clientName << ",DEV=" << 0);
                        if (port.port > 0)
                        {
                            rawMidiDevice += SS("," << port.port);
                        }
                    }
                    port.rawMidiDevice = std::move(rawMidiDevice);
                }
                port.id = SS("seq:" << port.clientName << "/" << port.name);

                port.displaySortOrder = port.client * 256 + port.port;
                if (port.isVirtual)
                {
                    port.displaySortOrder += 1 * 256 * 256; // MIDI virtual ports after real ports.
                }
                else if (port.clientName == "Midi Through")
                {
                    port.displaySortOrder += 2 * 256 * 256; // MIDI Through at the very end because it's weird.
                }
                ports.push_back(std::move(port));
            }
        }

        snd_seq_close(seq);
        return ports;
    }

    AlsaSequencerImpl::AlsaSequencerImpl()
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
        size_t inputBufferSize = snd_seq_get_input_buffer_size(seqHandle);
        (void)inputBufferSize;

        rc = snd_seq_set_input_buffer_size(seqHandle, 128 * 1024);
        if (rc < 0)
        {
            Lv2Log::warning("Failed resize the ALSA sequencer input buffer: %s", snd_strerror(rc));
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
        CreateRealtimeInputQueue();

        snd_seq_nonblock(seqHandle, 1); // Set sequencer to non-blocking mode

        // Get our client and port numbers for reference
        myClientId = snd_seq_client_id(seqHandle);
        if (myClientId < 0)
        {
            throw std::runtime_error(SS("Failed to get client ID: " << snd_strerror(myClientId)));
        }
    }
    void AlsaSequencerImpl::RemoveAllConnections()
    {
        while (connections.size() != 0)
        {
            auto connection = connections.back();
            ModifyConnection(connection.clientId, connection.portId, ConnectAction::Unsubscribe);
        }
    }
    AlsaSequencerImpl::~AlsaSequencerImpl()
    {
        RemoveAllConnections();

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
            Lv2Log::debug("Closing ALSA Sequencer");
            snd_seq_close(seqHandle);
            seqHandle = nullptr;
        }
    }

    void AlsaSequencerImpl::ConnectPort(int clientId, int portId)
    {
        ModifyConnection(clientId, portId, ConnectAction::Subscribe);
    }

    void AlsaSequencerImpl::ConnectPort(const std::string &id)
    {
        auto ports = EnumeratePorts();
        for (const auto &port : ports)
        {
            if (port.id == id)
            {
                ConnectPort(port.client, port.port);
                return;
            }
        }
        throw std::runtime_error("ALSA port not found");
    }
    void AlsaSequencerImpl::SetConfiguration(const AlsaSequencerConfiguration &alsaSequencerConfiguration)
    {
        this->RemoveAllConnections(); // Currently no configuration options to set

        auto ports = EnumeratePorts();
        for (const auto &connection : alsaSequencerConfiguration.connections())
        {
            // Connect to each port specified in the configuration
            std::string id = connection.id();
            for (const auto &port : ports)
            {
                if (port.id == id)
                {
                    ConnectPort(port.client, port.port);
                    break;
                }
            }
        }
    }

    bool AlsaSequencerImpl::WaitForMessage(int timeoutMs)
    {
        while (true)
        {
            auto fdCount = snd_seq_poll_descriptors_count(seqHandle, POLLIN);
            if (fdCount == 0)
                return true;
            this->pollFds.resize(fdCount);

            snd_seq_poll_descriptors(seqHandle, pollFds.data(), fdCount, POLLIN);

            int rc = poll(pollFds.data(), fdCount, timeoutMs);
            if (rc == 0)
            {
                return false; // timed out.
            }
            else if (rc < 0)
            {
                if (errno == EINTR)
                {
                    continue;
                }
                else
                {
                    throw std::runtime_error(SS("ALSA sequencer poll error: " << strerror(errno)));
                }
            }
            else
            {
                return true;
            }
        }
    }
    bool AlsaSequencerImpl::ReadMessage(AlsaMidiMessage &message, int timeoutMs)
    {
        // Event loop
        snd_seq_event_t *event = nullptr;
        while (true)
        {
            bool success = false;
            int rc = snd_seq_event_input(seqHandle, &event);
            if (rc < 0)
            {
                if (rc == -EAGAIN)
                {
                    if (!WaitForMessage(timeoutMs))
                    {
                        return false;
                    }
                }
                else
                {
                    // Handle other errors
                    throw std::runtime_error(SS("ALSA sequencer input error: " << snd_strerror(rc)));
                }
            }
            else if (event)
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
                    message.Set(
                        (uint8_t)(0x90 | event->data.note.channel),
                        (uint8_t)(event->data.note.note),      // note
                        (uint8_t)(event->data.note.velocity)); // velocity
                    break;
                case SND_SEQ_EVENT_NOTEOFF:
                    // handle note-off
                    message.Set(
                        uint8_t(0x80 | event->data.note.channel),
                        uint8_t(event->data.note.note),
                        uint8_t(event->data.note.off_velocity)); // off velocity
                    break;
                case SND_SEQ_EVENT_KEYPRESS:
                    message.Set(
                        (uint8_t)(0xA0 | event->data.note.channel), // polyphonic key pressure
                        (uint8_t)(event->data.note.note),           // note
                        (uint8_t)(event->data.note.velocity));      // pressure
                    break;
                case SND_SEQ_EVENT_CONTROLLER:
                    message.Set(
                        (uint8_t)(0xB0 | event->data.control.channel), // control change
                        (uint8_t)(event->data.control.param),          // controller number
                        (uint8_t)(event->data.control.value));         // controller value
                    break;
                case SND_SEQ_EVENT_PGMCHANGE:
                    message.Set(
                        (uint8_t)(0xC0 | event->data.control.channel), // program change
                        (uint8_t)(event->data.control.value));
                    break;

                case SND_SEQ_EVENT_CHANPRESS:
                    message.Set(
                        uint8_t(0xD0 | event->data.control.channel),
                        uint8_t(event->data.control.value));
                    break;
                case SND_SEQ_EVENT_PITCHBEND:
                    message.Set(uint8_t(0xE0 | event->data.control.channel),
                                uint8_t((event->data.control.value >> 7) & 0x7F),
                                uint8_t(event->data.control.value & 0x7F));
                    break;
                case SND_SEQ_EVENT_CONTROL14:
                    message.size = 6;
                    message.data = message.fixedBuffer;
                    message.fixedBuffer[0] = uint8_t(0xB0 | event->data.control.channel);    // Control Change 14-bit
                    message.fixedBuffer[1] = uint8_t(event->data.control.param);             // MSB
                    message.fixedBuffer[2] = uint8_t(event->data.control.param >> 7) & 0x7F; // MSB
                    message.fixedBuffer[3] = uint8_t(0xB0 | event->data.control.channel);
                    message.fixedBuffer[4] = uint8_t(event->data.control.value + 0x20);
                    message.fixedBuffer[5] = uint8_t(event->data.control.value) & 0x7F; // MSB value
                    break;

                case SND_SEQ_EVENT_NONREGPARAM:
                    message.size = 12;
                    message.data = message.fixedBuffer;
                    message.fixedBuffer[0] = uint8_t(0xB0 | event->data.control.channel);    // Non-registered parameter
                    message.fixedBuffer[1] = 0x63;                                           // MSB
                    message.fixedBuffer[2] = uint8_t(event->data.control.param >> 7) & 0x7F; // MSB
                    message.fixedBuffer[3] = uint8_t(0xB0 | event->data.control.channel);
                    message.fixedBuffer[4] = 0x62;                                      // LSB
                    message.fixedBuffer[5] = uint8_t(event->data.control.param) & 0x7F; // LSB
                    message.fixedBuffer[6] = uint8_t(0xB0 | event->data.control.channel);
                    message.fixedBuffer[7] = uint8_t(0x06);                                  // Non-registered parameter value MSB
                    message.fixedBuffer[8] = uint8_t(event->data.control.value >> 7) & 0x7F; // MSB value
                    message.fixedBuffer[9] = uint8_t(0xB0 | event->data.control.channel);
                    message.fixedBuffer[10] = uint8_t(0x26);                             // Non-registered parameter value LSB
                    message.fixedBuffer[11] = uint8_t(event->data.control.value) & 0x7F; // LSb
                    break;
                case SND_SEQ_EVENT_REGPARAM:
                    message.size = 12;
                    message.data = message.fixedBuffer;
                    message.fixedBuffer[0] = uint8_t(0xB0 | event->data.control.channel);    // Registered parameter
                    message.fixedBuffer[1] = uint8_t(0x65);                                  // MSB
                    message.fixedBuffer[2] = uint8_t(event->data.control.param >> 7) & 0x7F; // MSB
                    message.fixedBuffer[3] = uint8_t(0xB0 | event->data.control.channel);
                    message.fixedBuffer[4] = uint8_t(0x64);                             // LSB
                    message.fixedBuffer[5] = uint8_t(event->data.control.param) & 0x7F; // LSB
                    message.fixedBuffer[6] = uint8_t(0xB0 | event->data.control.channel);
                    message.fixedBuffer[7] = uint8_t(0x6);                                   // Registered parameter value MSB
                    message.fixedBuffer[8] = uint8_t(event->data.control.value >> 7) & 0x7F; // MSB value
                    message.fixedBuffer[9] = uint8_t(0xB0 | event->data.control.channel);
                    message.fixedBuffer[10] = uint8_t(0x26);                             // Registered parameter value LSB
                    message.fixedBuffer[11] = uint8_t(event->data.control.value) & 0x7F; // LSB value
                    break;
                case SND_SEQ_EVENT_SONGPOS:
                    message.Set(
                        0xF2,
                        (uint8_t)((event->data.control.value >> 7) & 0x7F), // MSB
                        (uint8_t)(event->data.control.value & 0x7F)         // LSB
                    );
                    break;
                case SND_SEQ_EVENT_SONGSEL:
                    message.Set(
                        0xF3,
                        (uint8_t)((event->data.control.value >> 7) & 0x7F), // MSB
                        (uint8_t)(event->data.control.value & 0x7F)         // LSB
                    );
                    break;
                case SND_SEQ_EVENT_QFRAME:
                    message.Set(
                        0xF1,
                        (uint8_t)((event->data.control.value >> 7) & 0x7F), // MSB
                        (uint8_t)(event->data.control.value & 0x7F)         // LSB
                    );
                    break;
                case SND_SEQ_EVENT_START:
                    message.Set(0xFA); // MIDI Real Time Start
                    break;
                case SND_SEQ_EVENT_CONTINUE:
                    message.Set(0xFB); // MIDI Real Time Continue
                    break;
                case SND_SEQ_EVENT_STOP:
                    message.Set(0xFC); // MIDI Real Time Stop
                    break;
                case SND_SEQ_EVENT_TICK:
                    message.Set(0xF8); // MIDI Real Time Clock Tick
                    break;
                case SND_SEQ_EVENT_SENSING:
                    message.Set(0xFE); // MIDI Real Time Active Sensing
                    break;
                case SND_SEQ_EVENT_RESET:
                    message.Set(0xFF); // MIDI Real Time System Reset
                    break;
                case SND_SEQ_EVENT_SYSEX:
                    // Handle SysEx messages
                    if (event->data.ext.len > 0 && event->data.ext.len + 2 <= sizeof(message.fixedBuffer))
                    {
                        message.size = event->data.ext.len + 1; // +1 for SysEx
                        message.data = message.fixedBuffer;
                        message.fixedBuffer[0] = 0xF0; // Start of SysEx
                        memcpy(message.fixedBuffer + 1, event->data.ext.ptr, event->data.ext.len);
                        message.fixedBuffer[event->data.ext.len + 1] = 0xF7; // End of SysEx
                    }
                    else
                    {
                        message.size = event->data.ext.len;
                        message.data = (uint8_t *)event->data.ext.ptr;
                        if (message.data[0] != 0xF0)
                        {
                            throw std::logic_error("Invalid SysEx message: does not start with 0xF0");
                        }
                    }
                    break;
                case SND_SEQ_EVENT_SETPOS_TICK:
                    message.size = 3 + sizeof(event->data.queue.param.value); // Tempo events are usually 3 bytes
                    message.data = message.fixedBuffer;
                    message.fixedBuffer[0] = 0xFF;                                    // Meta event type for tempo
                    message.fixedBuffer[1] = (uint8_t)MetaEventType::SetPositionTick; // Meta event subtype for tempo
                    message.fixedBuffer[2] = (uint8_t)(event->data.queue.queue);      // MSB
                    memcpy(message.fixedBuffer + 3, &event->data.queue.param.value, sizeof(event->data.queue.param.value));
                    break;
                case SND_SEQ_EVENT_SETPOS_TIME:
                    message.size = 3 + sizeof(event->data.queue.param.time); // Tempo events are usually 3 bytes
                    message.data = message.fixedBuffer;
                    message.fixedBuffer[0] = 0xFF;                                    // Meta event type for tempo
                    message.fixedBuffer[1] = (uint8_t)MetaEventType::SetPositionTime; // Meta event subtype for tempo
                    message.fixedBuffer[2] = (uint8_t)(event->data.queue.queue);
                    memcpy(message.fixedBuffer + 3, &event->data.queue.param.time, sizeof(event->data.queue.param.time));
                    break;
                case SND_SEQ_EVENT_TEMPO:
                    // Handle tempo events
                    message.size = sizeof(event->data.queue.param.value) + 3; // Tempo events are usually 3 bytes
                    if (message.size > sizeof(message.fixedBuffer))
                    {
                        throw std::logic_error("Tempo event size exceeds fixed buffer size");
                    }
                    message.data = message.fixedBuffer;
                    message.fixedBuffer[0] = 0xFF;                          // Meta event type for tempo
                    message.fixedBuffer[1] = (uint8_t)MetaEventType::Tempo; // Meta event subtype for tempo
                    message.fixedBuffer[2] = (uint8_t)(event->data.queue.queue);
                    memcpy(message.fixedBuffer + 3, &event->data.queue.param.value, sizeof(event->data.queue.param.value));
                    break;

                case SND_SEQ_EVENT_CLOCK:
                    message.Set(0xF8); // MIDI Real Time Clock Tick
                    break;
#ifndef NDEBUG
#define MSG_DEBUG_LOG(x)                            \
    case x:                                         \
        Lv2Log::debug("ALSA Sequencer Message " #x); \
        break;
#else
#define MSG_DEBUG_LOG(x)
#endif
                    MSG_DEBUG_LOG(SND_SEQ_EVENT_CLIENT_START)
                    MSG_DEBUG_LOG(SND_SEQ_EVENT_CLIENT_EXIT)
                    MSG_DEBUG_LOG(SND_SEQ_EVENT_CLIENT_CHANGE)
                    MSG_DEBUG_LOG(SND_SEQ_EVENT_PORT_START)
                    MSG_DEBUG_LOG(SND_SEQ_EVENT_PORT_EXIT)
                    MSG_DEBUG_LOG(SND_SEQ_EVENT_PORT_CHANGE)
                    MSG_DEBUG_LOG(SND_SEQ_EVENT_PORT_SUBSCRIBED)
                    MSG_DEBUG_LOG(SND_SEQ_EVENT_PORT_UNSUBSCRIBED)

                case SND_SEQ_EVENT_KEYSIGN:
                case SND_SEQ_EVENT_TIMESIGN:
                    // and a PASSEL of others!
                default:
                    success = false;
                    break;
                }
                snd_seq_free_event(event);
            }
            if (success)
            {
                return true;
            }
        }
    }

    int AlsaSequencerImpl::CreateRealtimeInputQueue()
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
    bool AlsaSequencerImpl::GetQueueRealtime(uint64_t *sec, uint32_t *nsec)
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

    std::string RawMidiIdToSequencerId(const std::vector<AlsaSequencerPort> &seqDevices, const std::string &rawMidiId)
    {
        for (const auto &device : seqDevices)
        {
            if (device.rawMidiDevice == rawMidiId)
            {
                return device.id;
            }
        }
        return {};
    }

    void AlsaSequencerImpl::ModifyConnection(int clientId, int portId, ConnectAction action)
    {
        std::lock_guard<std::mutex> lock(connectionsMutex);

        // use our own seq handle so that we can do this free-threaded.
        snd_seq_t *seq;
        if (snd_seq_open(&seq, "default", SND_SEQ_OPEN_DUPLEX, 0) < 0)
        {
            throw std::runtime_error("Failed to open ALSA sequencer for connection");
        }
        Finally seq_finally([seq]()
                            { snd_seq_close(seq); });


        snd_seq_addr_t sender, dest;
        dest.client = myClientId;
        dest.port = 0;
        sender.client = clientId;
        sender.port = portId;

        snd_seq_port_subscribe_t *subs;
        int queue = this->queueId;

        snd_seq_port_subscribe_alloca(&subs);
        snd_seq_port_subscribe_set_sender(subs, &sender);
        snd_seq_port_subscribe_set_dest(subs, &dest);
        snd_seq_port_subscribe_set_queue(subs, queue);
        snd_seq_port_subscribe_set_exclusive(subs, 0);

        if (action == ConnectAction::Unsubscribe)
        {
            if (snd_seq_get_port_subscription(seq, subs) < 0)
            {
                Lv2Log::warning(
                    "Failed to disconnect ALSA sequencer port %d:%d. Subscripton not found.",
                    (int)clientId,
                    (int)portId);
            }
            else
            {
                int rc = snd_seq_unsubscribe_port(seq, subs);
                if (rc < 0)
                {
                    Lv2Log::warning(
                        "Failed to disconnect ALSA sequencer port %d:%d. (%s)",
                        (int)clientId,
                        (int)portId,
                        snd_strerror(rc));
                }
            }
            for (auto it = this->connections.begin(); it != this->connections.end(); ++it)
            {
                if (it->clientId == clientId && it->portId == portId)
                {
                    it = this->connections.erase(it);
                    break;
                }
            }
        }
        else
        {
            if (snd_seq_get_port_subscription(seq, subs) == 0)
            {
                Lv2Log::warning("ALSA sequencer port  %d:%d is already subscribed.", (int)clientId, (int)portId);
                return;
            }
            int rc = snd_seq_subscribe_port(seq, subs);
            if (rc < 0)
            {
                Lv2Log::error("Failed to connect ALSA sequencer port %d:%d. (%s)",
                              (int)clientId, (int)portId,
                              snd_strerror(rc));
                return;
            }
            this->connections.push_back({clientId, portId});
        }
    }

    AlsaSequencerDeviceMonitor::ptr AlsaSequencerDeviceMonitor::Create()
    {
        return std::make_shared<AlsaSequencerDeviceMonitorImpl>();
    }

    void AlsaSequencerDeviceMonitorImpl::StartMonitoring(
        Callback &&onChangeCallback)
    {
        started = true;
        this->callback = std::move(onChangeCallback);
        this->serviceThread = std::make_unique<std::jthread>(
            [this]()
            {
                ServiceProc();
            });
    }
    void AlsaSequencerDeviceMonitorImpl::ServiceProc()
    {
        int err;
        snd_seq_t *seqHandle;

        err = snd_seq_open(&seqHandle, "default", SND_SEQ_OPEN_DUPLEX, 0);

        Finally seq_finally(
            [seqHandle]()
            {
                snd_seq_close(seqHandle);
            });
        if (err < 0)
        {
            Lv2Log::error("Error opening ALSA Device Monitor sequencer: %s", snd_strerror(err));
            return;
        }
        snd_seq_set_client_name(seqHandle, "Device Monitor");

        int inPort = snd_seq_create_simple_port(
            seqHandle, "PiPedal:portMonitor",
                        SND_SEQ_PORT_CAP_WRITE | SND_SEQ_PORT_CAP_SUBS_WRITE,
                        SND_SEQ_PORT_TYPE_APPLICATION);
        if (inPort < 0)
        {
            Lv2Log::error("Error creating ALSA Device Monitor port: %s", snd_strerror(inPort));
            return;
        }
        Finally inPort_finaly {
            [seqHandle, inPort]()
            {
                snd_seq_delete_port(seqHandle, inPort);
            }
        };

        // Set client name

        int queueId = CreateInputQueue(seqHandle,inPort);
        if (queueId < 0)
        {
            Lv2Log::error("Error creating ALSA Device Monitor queue: %s", snd_strerror(queueId));
            return;
        }
        Finally queue_finally(
            [seqHandle, queueId]()
            {
                snd_seq_free_queue(seqHandle, queueId);
            }); 

        // Subscribe to system announcements
        snd_seq_port_subscribe_t *subscription;
        snd_seq_port_subscribe_alloca(&subscription);
        snd_seq_addr_t sender, dest;

        sender.client = SND_SEQ_CLIENT_SYSTEM;
        sender.port = SND_SEQ_PORT_SYSTEM_ANNOUNCE;
        dest.client = snd_seq_client_id(seqHandle);
        dest.port = inPort;

        snd_seq_port_subscribe_set_sender(subscription, &sender);
        snd_seq_port_subscribe_set_dest(subscription, &dest);
        err = snd_seq_subscribe_port(seqHandle, subscription);
        if (err < 0)
        {
            Lv2Log::error("Failed to subscribe to ALSA sequencer announcements: %s", snd_strerror(err));
            return;
        }

        // Create poll descriptors
        std::vector<struct pollfd> pollFds;

        snd_seq_nonblock(seqHandle, 1); // Set sequencer to non-blocking mode
        while (!terminateThread)
        {
            int nPollFds = snd_seq_poll_descriptors_count(seqHandle, POLLIN);
            pollFds.resize(nPollFds);
            snd_seq_poll_descriptors(seqHandle, pollFds.data(), nPollFds, POLLIN);

            // Poll for events
            if (poll(pollFds.data(), nPollFds, 100) > 0)
            {
                snd_seq_event_t *event;
                while (snd_seq_event_input(seqHandle, &event) > 0)
                {
                    if (event->type == SND_SEQ_EVENT_CLIENT_START)
                    {
                        // Get the client name for logging/debugging
                        snd_seq_client_info_t *client_info;
                        snd_seq_client_info_alloca(&client_info);
                        if (snd_seq_get_any_client_info(seqHandle, event->data.addr.client, client_info) >= 0) {
                            std::string clientName = snd_seq_client_info_get_name(client_info);
                            callback(MonitorAction::DeviceAdded, event->data.addr.client, clientName);
                        }
                    }
                    else if (event->type == SND_SEQ_EVENT_CLIENT_EXIT)
                    {
                        callback(MonitorAction::DeviceRemoved, event->data.addr.client,"");
                    }
                    snd_seq_free_event(event);
                }
            }
        }

        return;
    }

    void AlsaSequencerDeviceMonitorImpl::StopMonitoring()
    {
        if (started)
        {
            started = false;
            terminateThread = true;
            serviceThread = nullptr; // (joins)
        }
    }

    AlsaSequencerDeviceMonitorImpl::~AlsaSequencerDeviceMonitorImpl()
    {
        StopMonitoring();
    }

    int AlsaSequencerDeviceMonitorImpl::CreateInputQueue(snd_seq_t *seqHandle, int inPort)
    {
        if (!seqHandle)
        {
            throw std::runtime_error("ALSA sequencer not initialized");
        }

        // Create a new queue if we don't have one yet
        int queueId = -1;
        {
            queueId = snd_seq_alloc_named_queue(seqHandle, "PiPedal Device Monitor Queue");
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
                throw std::runtime_error(SS("Failed to set queue tempo: " << snd_strerror(rc)));
            }

            // Start the queue
            rc = snd_seq_start_queue(seqHandle, queueId, nullptr);
            if (rc < 0)
            {
                snd_seq_free_queue(seqHandle, queueId);
                throw std::runtime_error(SS("Failed to start queue: " << snd_strerror(rc)));
            }

            // Set the queue for input timestamping
            snd_seq_port_info_t *port_info;
            snd_seq_port_info_alloca(&port_info);

            rc = snd_seq_get_port_info(seqHandle, inPort, port_info);
            if (rc < 0)
            {
                snd_seq_free_queue(seqHandle, queueId);
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
                throw std::runtime_error(SS("Failed to set port timestamping: " << snd_strerror(rc)));
            }
            return queueId;
        }

        return queueId;
    }

    JSON_MAP_BEGIN(AlsaSequencerPortSelection)
    JSON_MAP_REFERENCE(AlsaSequencerPortSelection, id)
    JSON_MAP_REFERENCE(AlsaSequencerPortSelection, name)
    JSON_MAP_REFERENCE(AlsaSequencerPortSelection, sortOrder)
    JSON_MAP_END()

    JSON_MAP_BEGIN(AlsaSequencerConfiguration)
    JSON_MAP_REFERENCE(AlsaSequencerConfiguration, connections)
    JSON_MAP_END()

} // namespace pipedal
