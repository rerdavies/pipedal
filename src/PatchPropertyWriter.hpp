// Copyright (c) 2024 Robin Davies
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
#include <cstdint>
#include <atomic>
#include <stdexcept>
#include "util.hpp"
#include <lv2/atom/atom.h>
namespace pipedal
{

    // A mechanism for writing (path) patch properties back to the non-realtime threads.
    // Has the following characteristics:
    // 1. Only one property in flight to the non-realtime thread at any givien moment.
    // 2. No blocking, no spinning on the realtime thread. RT operations require nothing other than atomic operations.
    // 3. Multiple responses in the same RT frame are coaslesced into one response (the latest).
    // 4. Responses that are found while a property notification is in flight are coaslesced.

    class IPatchWriterCallback;

    class PatchPropertyWriter
    {
    private:
        enum class StateT : char
        {
            Empty,
            WriteStarted,
            WriteLocked, // written data is available.
            ReadLocked,  // data is in flight between the realtime thread and non-realtime service thread.
            Deleted      // detect use after free.
        };

    public:
        PatchPropertyWriter(int64_t instanceId, LV2_URID patchPropertyUrid)
            : instanceId(instanceId), patchPropertyUrid(patchPropertyUrid),
              buffer0(new Buffer(instanceId, patchPropertyUrid)),
              buffer1(new Buffer(instanceId, patchPropertyUrid))
        {
        }
        // no copy.
        PatchPropertyWriter(const PatchPropertyWriter&) = delete;
        // move
        PatchPropertyWriter(PatchPropertyWriter&&other) {
            std::swap(this->buffer0,other.buffer0);
            std::swap(this->buffer1,other.buffer1);
            this->currentWriteBuffer = nullptr;
            this->instanceId = other.instanceId;
            this->patchPropertyUrid = other.patchPropertyUrid;

        }

        ~PatchPropertyWriter()
        {
            delete buffer0;
            delete buffer1;
        }
        class Buffer
        {
        public:
            std::atomic<StateT> state{StateT::Empty};

            Buffer(int64_t instanceId, LV2_URID propertyUrid)
                : instanceId(instanceId), patchPropertyUrid(propertyUrid)
            {
                memory.reserve(1024);
            }

            void OnBufferWriteStarted()
            {
                if (state != StateT::Empty && state != StateT::WriteLocked)
                {
                    throw std::runtime_error("Bad state.");
                }
                state = StateT::WriteStarted;
            }
            void OnBufferWritten()
            {
                if (state != StateT::WriteStarted)
                {
                    throw std::runtime_error("Bad state.");
                }
                state = StateT::ReadLocked;
                WriteBarrier();
            }

            void OnBufferReadStarted()
            {
                if (state != StateT::ReadLocked)
                {
                    throw std::runtime_error("Bad state.");
                }
                ReadBarrier();
            }
            void OnBufferReadComplete()
            {
                if (state != StateT::ReadLocked)
                {
                    throw std::runtime_error("Bad state.");
                }
                state = StateT::Empty;
            }
            int64_t instanceId;
            LV2_URID patchPropertyUrid;

            std::vector<uint8_t> memory;
        };

        // RT thread only.
        Buffer *AquireWriteBuffer()
        {
            // RT only.
            if (currentWriteBuffer)
            {
                return currentWriteBuffer;
            }
            if (buffer0->state == StateT::Empty)
            {
                buffer0->OnBufferWriteStarted();
                currentWriteBuffer = buffer0;
                return currentWriteBuffer;
            }
            if (buffer1->state == StateT::Empty)
            {
                buffer1->OnBufferWriteStarted();
                currentWriteBuffer = buffer1;
                return currentWriteBuffer;
            }
            throw std::runtime_error("Bad state. Unable to aquire a write buffer.");
        }
        // RT thread ony.
        void OnBufferWritten()
        {
            if (currentWriteBuffer)
            {
                currentWriteBuffer->OnBufferWritten();
                currentWriteBuffer = nullptr;
            }
        }
        Buffer *GetCurrentWriteBuffer()
        {
            return currentWriteBuffer;
        }

        void FlushWrites(IPatchWriterCallback*cbWrite);

        int64_t instanceId;
        LV2_URID patchPropertyUrid;

    private:
        Buffer *buffer0 = 0;
        Buffer *buffer1 = 0;
        Buffer *currentWriteBuffer = nullptr;
    };

    class IPatchWriterCallback {
    public:
        virtual void OnWritePatchPropertyBuffer(
            PatchPropertyWriter::Buffer *
        ) = 0;
    };

    inline void PatchPropertyWriter::FlushWrites(IPatchWriterCallback*cbWrite)
    {
        auto buffer = GetCurrentWriteBuffer();
        if (buffer)
        {   
            // is the OTHER buffer empty?
            if (buffer0->state == StateT::Empty || buffer1->state == StateT::Empty)
            {
                OnBufferWritten();
                cbWrite->OnWritePatchPropertyBuffer(buffer);
            } else {
                // we have an update in flight, so just keep accumulating, and maybe write next time.
            }
        }
    }


}