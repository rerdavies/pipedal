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
#include <cstdint>
#include "json.hpp"
#include "AudioConfig.hpp"

namespace pipedal
{
    // Jack Daemon configuration.
    class JackServerSettings
    {
        bool valid_ = false;
        bool isOnboarding_ = true;
        bool isJackAudio_ = JACK_HOST ? true : false;
        bool rebootRequired_ = false;
        std::string alsaInputDevice_;
        std::string alsaOutputDevice_;
        std::string alsaDevice_; // legacy
        uint64_t sampleRate_ = 0;
        uint32_t bufferSize_ = 64;
        uint32_t numberOfBuffers_ = 3;

    public:
    void SetAlsaInputDevice(const std::string &d){ alsaInputDevice_ = d; }
    void SetAlsaOutputDevice(const std::string &d){ alsaOutputDevice_ = d; }
        JackServerSettings();
        JackServerSettings(
            const std::string &alsaInputDevice,
            const std::string &alsaOutputDevice,
            uint64_t sampleRate,
            uint32_t bufferSize,
            uint32_t numberOfBuffers)
            : valid_(true),
              alsaInputDevice_(alsaInputDevice),
              alsaOutputDevice_(alsaOutputDevice),
              sampleRate_(sampleRate),
              bufferSize_(bufferSize),
              numberOfBuffers_(numberOfBuffers),
              isOnboarding_(false)
        {
        }

        uint64_t GetSampleRate() const { return sampleRate_; }

        uint32_t GetBufferSize() const { return bufferSize_; }
        uint32_t GetNumberOfBuffers() const { return numberOfBuffers_; }
        const std::string &GetAlsaInputDevice()  const { return alsaInputDevice_; }
        const std::string &GetAlsaOutputDevice() const { return alsaOutputDevice_; }
        const std::string &GetLegacyAlsaDevice() const { return alsaDevice_; } //legacy
        void SetLegacyAlsaDevice(const std::string &d) { alsaDevice_ = d; }
        void UseDummyAudioDevice() {
            this->valid_ = true;
            if (sampleRate_ == 0) sampleRate_ = 48000;
            this->alsaInputDevice_  = "__DUMMY_AUDIO__dummy:channels_2";
            this->alsaOutputDevice_ = "__DUMMY_AUDIO__dummy:channels_2";
        }
        bool IsDummyAudioDevice() const {
            return this->alsaInputDevice_.starts_with("__DUMMY_AUDIO__")
                || this->alsaOutputDevice_.starts_with("__DUMMY_AUDIO__");
        }

        void ReadJackDaemonConfiguration();

        bool IsValid() const { return valid_; }

        void WriteDaemonConfig(); // requires root perms.
        void SetRebootRequired(bool value)
        {
            rebootRequired_ = value;
        }
        void SetIsOnboarding(bool value)
        {
            isOnboarding_ = value;
        }

        bool Equals(const JackServerSettings &other) const
        {
        return this->alsaInputDevice_  == other.alsaInputDevice_ &&
                   this->alsaOutputDevice_ == other.alsaOutputDevice_ &&
                   this->alsaDevice_       == other.alsaDevice_ &&
                   this->sampleRate_       == other.sampleRate_ &&
                   this->bufferSize_       == other.bufferSize_ &&
                   this->numberOfBuffers_  == other.numberOfBuffers_;
        }

        DECLARE_JSON_MAP(JackServerSettings);
    };
} // namespace