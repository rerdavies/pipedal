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

#include "pch.h"
#include "JackConfiguration.hpp"
#include "AudioConfig.hpp"
#include "AlsaDriver.hpp"
#include "Lv2Log.hpp"
#include "PiPedalException.hpp"
#include "AlsaSequencer.hpp"


#if JACK_HOST
#include <jack/jack.h>
#include <jack/types.h>
#include <jack/session.h>
#endif

#include <string.h>
#include "defer.hpp"
#include <algorithm>

using namespace pipedal;


JackConfiguration::JackConfiguration()
{
    this->errorStatus_ = "Not initialized.";
}

JackConfiguration::~JackConfiguration()
{
}

#if JACK_HOST
static void AddPorts(jack_client_t *client, std::vector<std::string> *output, const char *port_name_pattern,
                     const char *type_name_pattern,
                     unsigned long flags)
{
    #if JACK_HOST
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
    #else 
        throw PiPedalStateException("JACK_HOST not enabled at compile time.");
    #endif
}
#endif

namespace pipedal {
#if JACK_HOST
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
#endif
}

static std::vector<AlsaMidiDeviceInfo> GetAlsaSequencers() {
    std::vector<AlsaMidiDeviceInfo> result;
    try {
        auto sequencer = AlsaSequencer::Create();
        if (sequencer)
        {
            std::vector<AlsaSequencerPort> ports = sequencer->EnumeratePorts();
            for (const auto &port : ports)
            {
                result.push_back(AlsaMidiDeviceInfo(port.id.c_str(), port.name.c_str()));
            }
        }
    } catch (const std::exception& e) {
        Lv2Log::error("Failed to enumerate ALSA sequencers: %s", e.what());
    }
    return result;
}
void JackConfiguration::AlsaInitialize(
    const JackServerSettings &jackServerSettings)
{
    this->isValid_ = false;
    this->errorStatus_ = "";

    if (jackServerSettings.IsDummyAudioDevice())
    {
        this->inputMidiDevices_.clear();
    } else {
        this->inputMidiDevices_ = GetAlsaSequencers(); // NB: Sequencers, not rawmidi devices, anymore.
    }
    if (jackServerSettings.IsValid())
    {
        this->blockLength_ = jackServerSettings.GetBufferSize();
        this->sampleRate_ = jackServerSettings.GetSampleRate();

        try {
        if (GetAlsaChannels(jackServerSettings, 
            inputAudioPorts_,outputAudioPorts_))
        {
            this->isValid_ = true;
        }
        } catch (const std::exception& /*ignore*/)
        {
            throw;
        }

    } 
    if (!this->isValid_)
    {
        this->isValid_ = false;
        this->errorStatus_ = "Not configured.";
    }

}
void JackConfiguration::JackInitialize()
{
    #if JACK_HOST
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
        isValid_ = (this->inputAudioPorts_.size() > 0 && this->outputAudioPorts_.size() > 0);
        this->errorStatus_ = "";
        jack_client_close(client);
    }
    catch (PiPedalException &e)
    {
        jack_client_close(client);
        this->errorStatus_ = e.what();
    }
    #else
        throw PiPedalStateException("JACK_HOST not enabled at compile time.");
    #endif
}

JackChannelSelection JackChannelSelection::MakeDefault(const JackConfiguration&config){
    JackChannelSelection result;
    for (size_t i = 0; i < std::min((size_t)2,config.inputAudioPorts().size()); ++i)
    {
        result.inputAudioPorts_.push_back(config.inputAudioPorts()[i]);

    }
    for (size_t i = 0; i < std::min((size_t)2,config.outputAudioPorts().size()); ++i)
    {
        result.outputAudioPorts_.push_back(config.outputAudioPorts()[i]);
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

    // if the result is empty, generate a default selection.
    if (result.size() == 0)
    {
        size_t n = std::min(available.size(),(size_t)2);
        for (size_t i = 0; i < n; ++i)
        {
            result.push_back(available[i]);
        }
    }
    return result;
}
static std::vector<AlsaMidiDeviceInfo> makeValid(const std::vector<AlsaMidiDeviceInfo> & selected, const std::vector<AlsaMidiDeviceInfo> &available)
{
    // Matching rule: 
    // same name AND same description (unchanged)
    // different name and same description: use the new device name. (i.e. same device plugged into different USB port)
    // otherwise discard.

    std::vector<AlsaMidiDeviceInfo> sourceDevices = available;
    std::vector<AlsaMidiDeviceInfo> result;
    std::vector<AlsaMidiDeviceInfo> partialMatch;

    for (auto & sel : selected)
    {
        size_t matchIndex = -1;
        for (size_t i = 0; i < sourceDevices.size(); ++i)
        {
            const AlsaMidiDeviceInfo &candidate = sourceDevices[i];
            if (candidate.description_ == sel.description_)
            {
                if (candidate.name_ == sel.name_)
                {
                    result.push_back(candidate);
                } else {
                    partialMatch.push_back(candidate);
                }
                sourceDevices.erase(sourceDevices.begin()+i);
                break;
            }
        }
    }
    for (auto &sel : partialMatch)
    {
        for (size_t i = 0; i < sourceDevices.size(); ++i)
        {
            const AlsaMidiDeviceInfo &candidate = sourceDevices[i];
            if (candidate.description_ == sel.description_)
            {
                result.push_back(candidate); // same description, usb address has changed. includeit.
                sourceDevices.erase(sourceDevices.begin()+i);
                break;
            }
        }
    }
    return result;
}


JackChannelSelection JackChannelSelection::RemoveInvalidChannels(const JackConfiguration&config) const
{
    JackChannelSelection result;
    result.inputAudioPorts_ = makeValid(this->inputAudioPorts_,config.inputAudioPorts());
    result.outputAudioPorts_ = makeValid(this->outputAudioPorts_,config.outputAudioPorts());
    result.inputMidiDevices_ = makeValid(this->inputMidiDevices_, config.inputMidiDevices());
    if (!result.isValid())
    {
        return this->MakeDefault(config);
    }
    return result;
}
JSON_MAP_BEGIN(JackChannelSelection)
    JSON_MAP_REFERENCE(JackChannelSelection,inputAudioPorts)
    JSON_MAP_REFERENCE(JackChannelSelection,outputAudioPorts)
    JSON_MAP_REFERENCE(JackChannelSelection,inputMidiDevices)

JSON_MAP_END()

JSON_MAP_BEGIN(JackConfiguration)
    JSON_MAP_REFERENCE(JackConfiguration,isValid)
    JSON_MAP_REFERENCE(JackConfiguration,isOnboarding)
    JSON_MAP_REFERENCE(JackConfiguration,isRestarting)
    JSON_MAP_REFERENCE(JackConfiguration,errorStatus)
    JSON_MAP_REFERENCE(JackConfiguration,sampleRate)
    JSON_MAP_REFERENCE(JackConfiguration,blockLength)
    JSON_MAP_REFERENCE(JackConfiguration,midiBufferSize)
    JSON_MAP_REFERENCE(JackConfiguration,maxAllowedMidiDelta)
    JSON_MAP_REFERENCE(JackConfiguration,inputAudioPorts)
    JSON_MAP_REFERENCE(JackConfiguration,outputAudioPorts)
    JSON_MAP_REFERENCE(JackConfiguration,inputMidiDevices)
JSON_MAP_END()

