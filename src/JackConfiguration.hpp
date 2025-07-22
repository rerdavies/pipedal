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

#include <vector>
#include <string>
#include "json.hpp"
#include "JackServerSettings.hpp"
#include "PiPedalAlsa.hpp"


namespace pipedal
{


    class JackConfiguration
    {
    private:
        bool isValid_ = false;
        std::string errorStatus_;

        bool isRestarting_ = false;
        bool isOnboarding_ = true;

        uint32_t sampleRate_ = 48000;
        size_t blockLength_ = 1024;
        size_t midiBufferSize_ = 16*1024;
        uint32_t maxAllowedMidiDelta_ = 0;

        std::vector<std::string> inputAudioPorts_;
        std::vector<std::string> outputAudioPorts_;
        std::vector<AlsaMidiDeviceInfo> inputMidiDevices_;
    public:
        JackConfiguration();

        void AlsaInitialize(const JackServerSettings &jackServerSettings); // from alsa config settings.
        
        void JackInitialize(); // from jack server instance.
        ~JackConfiguration();
        bool isValid() const { return isValid_;}
        void isValid(bool value) { isValid_ = value; }
        bool isOnboarding() const { return isOnboarding_;}
        void isOnboarding(bool value) { isOnboarding_ = value; }
        bool isRestarting() const { return isRestarting_; }
        void isRestarting(bool value) { isRestarting_ = value; }

        uint32_t sampleRate() const { return sampleRate_; }
        size_t blockLength() const { return blockLength_; }
        size_t midiBufferSize() const { return midiBufferSize_;}
        double maxAllowedMidiDelta() const { return maxAllowedMidiDelta_; }
        void setErrorStatus(const std::string&message) { this->errorStatus_ = message; }

        const std::vector<std::string> &inputAudioPorts() const { return inputAudioPorts_; }
        const std::vector<std::string> &outputAudioPorts() const { return outputAudioPorts_; }
        const std::vector<AlsaMidiDeviceInfo> &inputMidiDevices() const { return inputMidiDevices_; }

        DECLARE_JSON_MAP(JackConfiguration);

    };
    class JackChannelSelection {
    private:
        std::vector<std::string> inputAudioPorts_;
        std::vector<std::string> outputAudioPorts_;
        std::vector<AlsaMidiDeviceInfo> inputMidiDevices_;
    public:
        JackChannelSelection()
        {
        }
        JackChannelSelection(
            std::vector<std::string> inputAudioPorts,
            std::vector<std::string> outputAudioPorts,
            std::vector<AlsaMidiDeviceInfo> inputMidiDevices
        ) : inputAudioPorts_(inputAudioPorts),
            outputAudioPorts_(outputAudioPorts),
            inputMidiDevices_(inputMidiDevices)
        {

        }

        bool isValid() const {
            return inputAudioPorts_.size() != 0 && outputAudioPorts_.size() != 0;
        }

        const std::vector<std::string>& GetInputAudioPorts() const
        {
            return inputAudioPorts_;
        }
        const std::vector<std::string>& GetOutputAudioPorts() const
        {
            return outputAudioPorts_;
        }

        // replaced with AlsaSequencerConfiguration 
        const std::vector<AlsaMidiDeviceInfo>& LegacyGetInputMidiDevices() const
        {
            return inputMidiDevices_;
        }
        // replaced with AlsaSequencerConfiguration
        std::vector<AlsaMidiDeviceInfo>& LegacyGetInputMidiDevices() 
        {
            return inputMidiDevices_;
        }
        JackChannelSelection RemoveInvalidChannels(const JackConfiguration&config) const;

        static JackChannelSelection MakeDefault(const JackConfiguration&config);

        DECLARE_JSON_MAP(JackChannelSelection);
    };


} // namespace. 