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

#include "pch.h"
#include "JackConfiguration.hpp"
#include "Lv2Log.hpp"
#include "PiPedalException.hpp"

#include <jack/jack.h>
#include <jack/types.h>
#include <jack/session.h>
#include <string.h>
#include "defer.hpp"
#include <algorithm>

using namespace pipedal;

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

JackConfiguration::JackConfiguration()
{
    this->errorStatus_ = "Not initialized.";
}

JackConfiguration::~JackConfiguration()
{
}

static void AddPorts(jack_client_t *client, std::vector<std::string> *output, const char *port_name_pattern,
                     const char *type_name_pattern,
                     unsigned long flags)
{
    const char**ports = jack_get_ports(client, port_name_pattern, type_name_pattern, flags);
    if (ports != nullptr)
    {
            for (const char **p = ports; *p != nullptr; ++p)
            {
                const char *portName = *p;
                output->push_back(portName);
            }
            jack_free(ports);

    }
}

namespace pipedal {
    std::string GetJackErrorMessage(jack_status_t status)
    {
        std::stringstream s;
        s << "Unable connect to Jack audio sever. ";
        if (status & JackVersionError) {
            s << "Jack server/client version mismatch.";
        } else if (status & JackServerError)
        {
            s << "Server error.";
        } else if (status & JackServerFailed)
        {
            s << "Unable to connect to server.";
        } else if (status & JackShmFailure)
        {
            s << "Unable to access shared memory.";
        } else if (status & JackInvalidOption)
        {
            s << "Invalid option.";
        } else if (status & JackInitFailure)
        {
            s << "Initialization error.";
        } else if (status & JackLoadFailure)
        {
            s << "Can't load client.";
        } else if (status )
        {
            s << "Unknown error.";
        }
        return s.str();

    }
}
void JackConfiguration::Initialize()
{
    jack_status_t status;

    jack_client_t *client = nullptr;
    try
    {

        client = jack_client_open("PiPedal", JackNullOption, &status);

        if (client == nullptr)
        {
            std::string error = GetJackErrorMessage(status);
            Lv2Log::error(error);
            throw PiPedalStateException(error.c_str());
        }
        //jack_set_process_callback(client, process_fn, this);

        //jack_on_shutdown(client, jack_shutdown_fn, this);

        //jack_set_session_callback(client, session_callback_fn, NULL);

        this->sampleRate_ = jack_get_sample_rate(client);
        blockLength_ = jack_get_buffer_size(client);
        midiBufferSize_ = jack_port_type_get_buffer_size(client, JACK_DEFAULT_MIDI_TYPE);
        maxAllowedMidiDelta_ = (uint32_t)(jack_nframes_t)(sampleRate_ * 0.2); // max 200ms of allowed delta

        // if (jack_activate(client))
        // {
        //     this->errorStatus_ = "Failed to activate Jack Audio client.";
        //     Lv2Log::Error("jack_activate failed.");
        //     throw PiPedalStateException("jack_activate failed.");
        // }

        // enumerate the output ports.

        AddPorts(client, &this->inputAudioPorts_, "system", JACK_DEFAULT_AUDIO_TYPE, JackPortIsOutput);
        AddPorts(client, &this->outputAudioPorts_, "system", JACK_DEFAULT_AUDIO_TYPE, JackPortIsInput);
        AddPorts(client, &this->inputMidiPorts_, "system", JACK_DEFAULT_MIDI_TYPE, JackPortIsOutput);
        AddPorts(client, &this->outputMidiPorts_, "system", JACK_DEFAULT_MIDI_TYPE, JackPortIsInput);
        isValid_ = true;
        this->errorStatus_ = "";
        jack_client_close(client);
    }
    catch (PiPedalException &e)
    {
        jack_client_close(client);
        this->errorStatus_ = e.what();
    }
}

JackChannelSelection JackChannelSelection::MakeDefault(const JackConfiguration&config){
    JackChannelSelection result;
    for (size_t i = 0; i < std::min((size_t)2,config.GetInputAudioPorts().size()); ++i)
    {
        result.inputAudioPorts_.push_back(config.GetInputAudioPorts()[i]);

    }
    for (size_t i = 0; i < std::min((size_t)2,config.GetOutputAudioPorts().size()); ++i)
    {
        result.outputAudioPorts_.push_back(config.GetOutputAudioPorts()[i]);
    }
    return result;

}

static std::vector<std::string> makeValid(const std::vector<std::string> & selected, const std::vector<std::string> &available)
{
    std::vector<std::string> result;
    for (size_t i = 0; i < selected.size(); ++i)
    {
        std::string t = selected[i];
        bool found = false;

        for (size_t j = 0; j < available.size(); ++j)
        {
            if (t == available[j])
            {
                found = true;
                break;
            }
        }
        if (found) {
            result.push_back(t);
        }
    }
    return result;
}
JackChannelSelection JackChannelSelection::RemoveInvalidChannels(const JackConfiguration&config) const
{
    JackChannelSelection result;
    result.inputAudioPorts_ = makeValid(this->inputAudioPorts_,config.GetInputAudioPorts());
    result.outputAudioPorts_ = makeValid(this->outputAudioPorts_,config.GetOutputAudioPorts());
    result.inputMidiPorts_ = makeValid(this->inputMidiPorts_, config.GetInputMidiPorts());
    if (!result.isValid())
    {
        return this->MakeDefault(config);
    }
    return result;
}
JSON_MAP_BEGIN(JackChannelSelection)
    JSON_MAP_REFERENCE(JackChannelSelection,inputAudioPorts)
    JSON_MAP_REFERENCE(JackChannelSelection,outputAudioPorts)
    JSON_MAP_REFERENCE(JackChannelSelection,inputMidiPorts)

JSON_MAP_END()

JSON_MAP_BEGIN(JackConfiguration)
    JSON_MAP_REFERENCE(JackConfiguration,isValid)
    JSON_MAP_REFERENCE(JackConfiguration,isRestarting)
    JSON_MAP_REFERENCE(JackConfiguration,errorStatus)
    JSON_MAP_REFERENCE(JackConfiguration,sampleRate)
    JSON_MAP_REFERENCE(JackConfiguration,blockLength)
    JSON_MAP_REFERENCE(JackConfiguration,midiBufferSize)
    JSON_MAP_REFERENCE(JackConfiguration,maxAllowedMidiDelta)
    JSON_MAP_REFERENCE(JackConfiguration,inputAudioPorts)
    JSON_MAP_REFERENCE(JackConfiguration,outputAudioPorts)
    JSON_MAP_REFERENCE(JackConfiguration,inputMidiPorts)
    JSON_MAP_REFERENCE(JackConfiguration,outputMidiPorts)
JSON_MAP_END()

