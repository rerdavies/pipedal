#pragma once
#include <cstdint>
#include "json.hpp"

namespace pipedal {
    // Jack Daemon configuration.
    class JackServerSettings {
        bool valid_ = false;
        bool rebootRequired_ = false;
        uint64_t sampleRate_ = 0;
        uint32_t bufferSize_ = 0;
        uint32_t numberOfBuffers_ = 0;

    public:
        JackServerSettings();

        uint64_t GetSampleRate() const { return sampleRate_; }
        uint32_t GetBufferSize() const { return bufferSize_; }
        uint32_t GetNumberOfBuffers() const { return numberOfBuffers_; }

        void ReadJackConfiguration();

        bool IsValid() const { return valid_; }

        JackServerSettings(uint64_t sampleRate, uint32_t bufferSize, uint32_t numberOfBuffers)
        {
            this->valid_ = true;
            this->rebootRequired_ = true;
            this->sampleRate_ = sampleRate; this->bufferSize_ = bufferSize; this->numberOfBuffers_ = numberOfBuffers;
        }
        void Write(); // requires root perms.
        void SetRebootRequired(bool value)
        {
            rebootRequired_ = value;
        }
        bool Equals(const JackServerSettings&other)
        {
            return this->sampleRate_ == other.sampleRate_
            && this->bufferSize_ == other.bufferSize_
            && this->numberOfBuffers_ == other.numberOfBuffers_;
        }


        DECLARE_JSON_MAP(JackServerSettings);

    };
} // namespace