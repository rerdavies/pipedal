#pragma once

#include <vector>
#include <string>
#include "json.hpp"

namespace pipedal
{


    class JackConfiguration
    {
    private:
        bool isValid_ = false;
        std::string errorStatus_;

        bool isRestarting_ = false;

        uint32_t sampleRate_ = 48000;
        size_t blockLength_ = 1024;
        size_t midiBufferSize_ = 16*1024;
        uint32_t maxAllowedMidiDelta_ = 0;

        std::vector<std::string> inputAudioPorts_;
        std::vector<std::string> outputAudioPorts_;
        std::vector<std::string> inputMidiPorts_;
        std::vector<std::string> outputMidiPorts_;
    public:
        JackConfiguration();
        void Initialize();
        ~JackConfiguration();
        bool isValid() const { return isValid_;}
        bool isRestarting() const { return isRestarting_; }
        void SetIsRestarting(bool value) { isRestarting_ = value; }
        uint32_t GetSampleRate() const { return sampleRate_; }
        size_t GetBlockLength() const { return blockLength_; }
        size_t GetMidiBufferSize() const { return midiBufferSize_;}
        double GetMaxAllowedMidiDelta() const { return maxAllowedMidiDelta_; }

        const std::vector<std::string> &GetInputAudioPorts() const { return inputAudioPorts_; }
        const std::vector<std::string> &GetOutputAudioPorts() const { return outputAudioPorts_; }
        const std::vector<std::string> &GetInputMidiPorts() const { return inputMidiPorts_; }
        const std::vector<std::string> &GetOutputMidiPorts() const { return outputMidiPorts_; }

        DECLARE_JSON_MAP(JackConfiguration);

    };
    class JackChannelSelection {
    private:
        std::vector<std::string> inputAudioPorts_;
        std::vector<std::string> outputAudioPorts_;
        std::vector<std::string> inputMidiPorts_;
    public:
        JackChannelSelection()
        {
        }

        bool isValid() const {
            return inputAudioPorts_.size() != 0 && outputAudioPorts_.size() != 0;
        }

        const std::vector<std::string>& GetInputAudioPorts() const
        {
            return inputAudioPorts_;
        }
        const std::vector<std::string>& getOutputAudioPorts() const
        {
            return outputAudioPorts_;
        }

        const std::vector<std::string>& GetInputMidiPorts() const
        {
            return inputMidiPorts_;
        }
        JackChannelSelection RemoveInvalidChannels(const JackConfiguration&config) const;

        static JackChannelSelection MakeDefault(const JackConfiguration&config);

        DECLARE_JSON_MAP(JackChannelSelection);
    };


} // namespace.