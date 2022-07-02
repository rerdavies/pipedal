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
#pragma once

#include "JackConfiguration.hpp"
#include <functional>




namespace pipedal {


    using ProcessCallback = std::function<void (size_t)>;



    struct MidiEvent
    {
        /* BINARY COMPATIBLE WITH jack_midi_event_t;! (in case we ever want to resurrect Jack support) */
        uint32_t    time;   /**< Sample index at which event is valid */
        size_t            size;   /**< Number of bytes of data in \a buffer */
        uint8_t *buffer; /**< Raw MIDI data */

    };


    class AudioDriverHost {
    public:
        virtual void OnProcess(size_t nFrames) = 0;
        virtual void OnUnderrun() = 0;
        virtual void OnAudioStopped() = 0;


    };
    class AudioDriver {
    public:

        virtual ~AudioDriver() { }

        virtual float CpuUse() = 0;
        virtual float CpuOverhead() = 0;

        virtual uint32_t GetSampleRate() = 0;

        virtual size_t MidiInputBufferCount() const = 0;
        virtual void*GetMidiInputBuffer(size_t channel, size_t nFrames) = 0;
        virtual size_t GetMidiInputEventCount(void*buffer) = 0;
        virtual bool GetMidiInputEvent(MidiEvent*event, void*portBuf, size_t nFrame) = 0;

        virtual void FillMidiBuffers() = 0;

        virtual size_t InputBufferCount() const = 0;
        virtual float*GetInputBuffer(size_t channel, size_t nFrames) = 0;


        virtual size_t OutputBufferCount() const = 0;
        virtual float*GetOutputBuffer(size_t channel, size_t nFrames) = 0;

        virtual void Open(const JackServerSettings & jackServerSettings,const JackChannelSelection &channelSelection) = 0;

        virtual void Activate() = 0;
        virtual void Deactivate() = 0;
        virtual void Close() = 0;

    };

};