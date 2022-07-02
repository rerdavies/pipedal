/*
 * MIT License
 * 
 * Copyright (c) 2022 Robin E. R. Davies
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/* Status of Jack support.

PiPedal was originally written for Jack, but subsequently ported to Alsa because running
Jack as a systemd daemon proved to be unsupportable,. 



*/

#include "pch.h"
#include "JackDriver.hpp"

#include <jack/jack.h>
#include <jack/types.h>
#include <jack/session.h>
#include <jack/midiport.h>
#include "Lv2Log.hpp"

#if JACK_HOST

namespace pipedal {


std::string JackStatusToString(jack_status_t status)
{
    switch (status)
    {
    default:
        return "Unknown Jack Audio error.";
    case JackStatus::JackFailure:
        return "Jack Audio operation failed.";
    case JackStatus::JackInvalidOption:
        return "Invalid Jack Audio operation.";
    case JackStatus::JackNameNotUnique:
        return "Jack Audio name in use.";
    case JackStatus::JackServerStarted:
        return "Jack Audio Server started.";
    case JackStatus::JackServerFailed:
        return "Jack Audio Server failed.";
    case JackStatus::JackServerError:
        return "Can't connect to the Jack Audio Server.";
    case JackStatus::JackNoSuchClient:
        return "Jack Audio client not found.";
    case JackLoadFailure:
        return "Jack Audio client failed to load.";
    case JackStatus::JackInitFailure:
        return "Failed to initialize the Jack Audio client.";
    case JackStatus::JackShmFailure:
        return "Jack Audio failed to access shared memory.";
    case JackStatus::JackVersionError:
        return "Jack Audio Server version error.";
    case JackStatus::JackClientZombie:
        return "JackClientZombie error.";
    }
};

/* 

    This code relies on the fact that jack_midi_event_t and pipedal::MidiEvent are binary-compatible.
    (the default build does not require Jack Audio headers to be present).

 
    Test to make sure this is still true.
*/


static_assert(sizeof(jack_midi_event_t) == sizeof(pipedal::MidiEvent));
 
#define CHECK_FIELD(fieldName) \
    static_assert(offsetof(jack_midi_event_t,fieldName) == offsetof(pipedal::MidiEvent,fieldName)); \
    static_assert(sizeof(jack_midi_event_t::fieldName) == sizeof(pipedal::MidiEvent::fieldName)); 

CHECK_FIELD(size)
CHECK_FIELD(buffer)
CHECK_FIELD(time)


// private import from JackConfiguration.cpp
std::string GetJackErrorMessage(jack_status_t status);


// forward decl
static int process_fn(jack_nframes_t nframes, void *arg);

static void jack_error_fn(const char *msg)
{
    Lv2Log::error("Jack - %s", msg);
}
static void jack_info_fn(const char *msg)
{
    Lv2Log::info("Jack - %s", msg);
}


class JackDriverImpl : public AudioDriver {
private:
    jack_client_t *client = nullptr;
    std::vector<jack_port_t *> inputPorts;
    std::vector<jack_port_t *> outputPorts;
    std::vector<jack_port_t *> midiInputPorts;
    uint32_t sampleRate = 0;

    AudioDriverHost *driverHost = nullptr;

public:
    JackDriverImpl(AudioDriverHost*driverHost)
    : driverHost(driverHost)
    {
        jack_set_error_function(jack_error_fn);
        jack_set_info_function(jack_info_fn);

    }
    virtual ~JackDriverImpl()
    {
        jack_set_error_function(nullptr);
        jack_set_info_function(nullptr);

    }
private:

    void OnShutdown()
    {
        Lv2Log::info("Jack Audio Server has shut down.");
    }

    static void
    jack_shutdown_fn(void *arg)
    {
        ((JackDriverImpl *)arg)->OnShutdown();
    }


    static int jackInstanceId; 

    static int xrun_callback_fn(void *arg)
    {
        ((AudioDriverHost *)arg)->OnUnderrun();
        return 0;
    }

    virtual uint32_t GetSampleRate() {
        return this->sampleRate;
    }

    bool open = false;
    virtual void Open(const JackServerSettings&jackServerSettings,const JackChannelSelection &channelSelection)  {
        if (open) throw PiPedalStateException("Already open");
        open = true;

        this->channelSelection = channelSelection;
        jack_status_t status;
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

        jack_set_process_callback(client, process_fn, this->driverHost);

        jack_on_shutdown(client, jack_shutdown_fn, this);

        jack_set_xrun_callback(client, xrun_callback_fn, this->driverHost);

        this->sampleRate = jack_get_sample_rate(client);


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
        auto &selectedOutputPorts = channelSelection.GetOutputAudioPorts();
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
        auto &selectedMidiPorts = channelSelection.GetInputMidiDevices();

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
    }

    bool jackActive = false;

    JackChannelSelection channelSelection;
    virtual void Activate() 
    {
        int activateResult = jack_activate(client);
        if (activateResult == 0)
        {
            jackActive = true;

            auto &selectedInputPorts = channelSelection.GetInputAudioPorts();

            for (int i = 0; i < selectedInputPorts.size(); ++i)
            {
                auto result = jack_connect(client, selectedInputPorts[i].c_str(), jack_port_name(this->inputPorts[i]));
                if (result)
                {
                    Lv2Log::error("Can't connect input port %s", selectedInputPorts[i].c_str());
                    throw PiPedalStateException("Jack Audio port connection failed.");
                }
            }

            auto &selectedOutputPorts = channelSelection.GetOutputAudioPorts();

            for (int i = 0; i < selectedOutputPorts.size(); ++i)
            {
                auto result = jack_connect(client, jack_port_name(this->outputPorts[i]), selectedOutputPorts[i].c_str());
                if (result)
                {
                    Lv2Log::error("Can't connect output port %s", selectedOutputPorts[i].c_str());
                    throw PiPedalStateException("Jack Audio port connection failed.");
                }
            }

            auto &selectedMidiPorts = channelSelection.GetInputMidiDevices();

            for (int i = 0; i < selectedMidiPorts.size(); ++i)
            {
                auto result = jack_connect(client, selectedMidiPorts[i].name_.c_str(), jack_port_name(this->midiInputPorts[i]));
                if (result)
                {
                    Lv2Log::error("Can't connect midi input port %s", selectedMidiPorts[i].name_.c_str());
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

    void FillMidiBuffers()
    {
            // do nothing. Jack does this for us.
    }
    virtual void Deactivate()
    {
        if (!jackActive)
        {
            return;
        }
        jackActive = false;
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
                jack_disconnect(client, jack_port_name(port),channelSelection.GetOutputAudioPorts()[i].c_str());
            }
        }
        for (size_t i = 0; i < midiInputPorts.size(); ++i)
        {
            auto port = midiInputPorts[i];
            if (port != nullptr)
            {
                jack_disconnect(client, channelSelection.GetInputMidiDevices()[i].name_.c_str(), jack_port_name(port));
            }
        }

        if (jackActive)
        {
            jack_deactivate(client);
            jackActive = false;
        }

        jack_set_process_callback(client, nullptr, nullptr);
        jack_on_shutdown(client, nullptr, nullptr);
        #if JACK_SESSION_CALLBACK // (deprecated, and not actually useful)
            jack_set_session_callback(client, nullptr, nullptr);
        #endif
        jack_set_xrun_callback(client, nullptr, nullptr);

    }

    virtual size_t MidiInputBufferCount() const { return this->midiInputPorts.size(); }
    virtual void*GetMidiInputBuffer(size_t channel, size_t nFrames) {
        return (void*)jack_port_get_buffer(this->midiInputPorts[channel], nFrames);
    }

    virtual size_t GetMidiInputEventCount(void *portBuffer) 
    {
        return jack_midi_get_event_count(portBuffer);
    }
    virtual bool GetMidiInputEvent(MidiEvent*event, void*portBuf, size_t nFrame)
    {
        return jack_midi_event_get((jack_midi_event_t*)event,portBuf,(uint32_t)nFrame);
    }




    virtual size_t InputBufferCount() const { return inputPorts.size(); }
    virtual float*GetInputBuffer(size_t channel, size_t nFrames) {
        return (float*)jack_port_get_buffer(this->inputPorts[channel], nFrames);
    }

    virtual size_t OutputBufferCount() const { return outputPorts.size(); }
    virtual float*GetOutputBuffer(size_t channel, size_t nFrames)
    {
        return (float*)jack_port_get_buffer(this->outputPorts[channel], nFrames);
    }

    virtual void Close()
    {
        if (!open) return;
        open = false;

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
        if (client)
        {
            jack_client_close(client);
            client = nullptr;
        }

    }

    virtual float CpuOverhead() { return 0; }
    virtual float CpuUse()
    {

        if (client)
        {
            return jack_cpu_load(this->client);
        }
        else
        {
            return 0;
        }
    }

    static int process_fn(jack_nframes_t nframes, void *arg)
    {
        ((AudioDriverHost *)arg)->OnProcess(nframes);
        return 0;
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


    static void
    session_callback_fn(jack_session_event_t *event, void *arg)
    {
        ((JackDriverImpl *)arg)->OnSessionCallback(event);
    };


};

AudioDriver* CreateJackDriver(AudioDriverHost*driverHost)
{
    return new JackDriverImpl(driverHost);
}


int JackDriverImpl::jackInstanceId = 0;


} // namespace



#endif