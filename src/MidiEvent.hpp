/*
 * MIT License
 * 
* Copyright (c) 2026 Robin E. R. Davies
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
#ifndef PIPEDAL_MidiEvent_HPP
#define PIPEDAL_MidiEvent_HPP

#include <cstdint>

namespace pipedal {
    struct MidiTimestamp
    {
        uint64_t seconds;
        uint32_t nanoseconds;

        MidiTimestamp()
            : seconds(0), nanoseconds(0)
        {
        }
        MidiTimestamp(uint64_t sec, uint32_t nsec)
            : seconds(sec), nanoseconds(nsec)
        {
        }

        bool isEmpty() const { 
            return seconds == 0 && nanoseconds == 0;
        }
        double timeDiff(const MidiTimestamp &other) const
        {
            double secDiff = (double)(int64_t)(this->seconds - other.seconds);
            double nsecDiff = (double)((int64_t)this->nanoseconds - (int64_t)other.nanoseconds) * 1e-9;
            return secDiff + nsecDiff;
        }
    };

    struct MidiEvent
    {
        MidiTimestamp timeStamp; /**< Real-time timestamp of the event */
        uint32_t    frame;   /**< Sample frame at which event is valid */
        uint32_t  size;   /**< Number of bytes of data in \a buffer */
        uint8_t  *buffer; /**< Raw MIDI data */
    };



};

#endif
