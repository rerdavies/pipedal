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
#include <cstddef>
#include "PiPedalException.hpp"
#include <atomic>
#include <mutex>
#include <condition_variable>

#ifndef NO_MLOCK
#include <sys/mman.h>
#endif /* NO_MLOCK */

namespace pipedal
{

    enum class RingBufferStatus
    {
        Ready,
        TimedOut,
        Closed
    };
    template <bool MULTI_WRITER = false, bool SEMAPHORE_READER = false>
    class RingBuffer
    {
        char *buffer;
        bool mlocked = false;
        size_t ringBufferSize;
        size_t ringBufferMask;
        int64_t readPosition = 0;  // volatile = ordering barrier wrt writePosition
        int64_t writePosition = 0; // volatile = ordering barrier wrt/ readPosition
        std::mutex mutex;
        std::mutex writeMutex;

        bool is_open = true;
        std::condition_variable cvRead;

        size_t nextPowerOfTwo(size_t size)
        {
            size_t v = 1;
            while (v < size)
            {
                v *= 2;
            }
            return v;
        }

    public:
        RingBuffer(size_t ringBufferSize = 65536, bool mLock = true)
        {
            this->ringBufferSize = ringBufferSize = nextPowerOfTwo(ringBufferSize);
            ringBufferMask = ringBufferSize - 1;
            buffer = new char[ringBufferSize];

#ifndef NO_MLOCK
            if (mLock)
            {
                if (mlock(buffer, ringBufferSize))
                {
                    throw PiPedalStateException("Mlock failed.");
                }
                this->mlocked = true;
            }
#endif
        }

        void reset()
        {
            this->readPosition = 0;
            this->writePosition = 0;
            this->is_open = true;
            cvRead.notify_all();
        }
        void close()
        {
            if (SEMAPHORE_READER)
            {
                this->is_open = false;
                cvRead.notify_all();
            }
        }

        template <class Rep, class Period>
        RingBufferStatus readWait_for(const std::chrono::duration<Rep, Period> &timeout)
        {
            while (true)
            {
                if (SEMAPHORE_READER)
                {
                    std::unique_lock lock(mutex);
                    if (isReadReady_())
                    {
                        return RingBufferStatus::Ready;
                    }
                    if (!is_open)
                        return RingBufferStatus::Closed;

                    auto status = cvRead.wait_for(lock, timeout);
                    if (status == std::cv_status::timeout)
                    {
                        return RingBufferStatus::TimedOut;
                    }
                }
                else
                {
                    static_assert("SEMAPHORE_READER is not set to true.");
                }
            }
        }

        template <class Clock, class Duration>
        RingBufferStatus readWait_until(const std::chrono::time_point<Clock, Duration> &time_point)
        {
            while (true)
            {
                if (SEMAPHORE_READER)
                {
                    std::unique_lock lock(mutex);
                    if (isReadReady_())
                    {
                        return RingBufferStatus::Ready;
                    }
                    if (!is_open)
                        return RingBufferStatus::Closed;

                    auto status = cvRead.wait_until(lock, time_point);
                    if (status == std::cv_status::timeout)
                    {
                        return RingBufferStatus::TimedOut;
                    }
                }
                else
                {
                    static_assert("SEMAPHORE_READER is not set to true.");
                }
            }
        }
        template <class Clock, class Duration>
        RingBufferStatus readWait_until(size_t size,const std::chrono::time_point<Clock, Duration> &time_point)
        {
            while (true)
            {
                if (SEMAPHORE_READER)
                {
                    std::unique_lock lock(mutex);
                    size_t available = readSpace_();
                    if (available >= size) 
                    {
                        return RingBufferStatus::Ready;
                    }
                    if (!is_open)
                        return RingBufferStatus::Closed;

                    auto status = cvRead.wait_until(lock, time_point);
                    if (status == std::cv_status::timeout)
                    {
                        return RingBufferStatus::TimedOut;
                    }
                }
                else
                {
                    static_assert("SEMAPHORE_READER is not set to true.");
                }
            }
        }

        bool readWait()
        {
            if (SEMAPHORE_READER)
            {
                while (true)
                {
                    std::unique_lock lock(mutex);
                    if (isReadReady_())
                    {
                        return true;
                    }

                    if (!is_open)
                        return false;
                    cvRead.wait(lock);
                }
            }
            else
            {
                static_assert("SEMAPHORE_READER is not set to true.");
            }
        }
        size_t writeSpace()
        {
            // at most ringBufferSize-1 in order to
            // to distinguish the empty buffer from the full buffer.
            std::unique_lock lock(mutex);

            int64_t size = readPosition - 1 - writePosition;
            if (size < 0)
                size += this->ringBufferSize;
            return (size_t)size;
        }


        size_t readSpace()
        {
            std::unique_lock lock(mutex);
            return readSpace_();
        }
        bool write(size_t bytes, uint8_t *data)
        {
            if (MULTI_WRITER)
            {
                std::lock_guard writeLock{writeMutex};
                if (writeSpace() < bytes + sizeof(bytes))
                {
                    return false;
                }
                size_t index = this->writePosition;
                for (size_t i = 0; i < bytes; ++i)
                {
                    buffer[(index + i) & ringBufferMask] = data[i];
                }
                {
                    std::lock_guard lock(mutex);
                    this->writePosition = (index + bytes) & ringBufferMask;
                }
                if (SEMAPHORE_READER)
                {
                    cvRead.notify_all();
                }
                return true;
            }
            else
            {
                if (writeSpace() < sizeof(bytes) + bytes)
                {
                    return false;
                }
                size_t index = this->writePosition;
                for (size_t i = 0; i < bytes; ++i)
                {
                    buffer[(index + i) & ringBufferMask] = data[i];
                }
                {
                    std::lock_guard lock{mutex};
                    this->writePosition = (index + bytes) & ringBufferMask;
                }
                if (SEMAPHORE_READER)
                {
                    cvRead.notify_all();
                }
                return true;
            }
        }
        // Write two disjoint areas of memory atomically.
        bool write(size_t bytes, uint8_t *data, size_t bytes2, uint8_t *data2)
        {
            if (MULTI_WRITER)
            {
                std::lock_guard guard(writeMutex);
                if (writeSpace() <= sizeof(bytes) + bytes +bytes2)
                {
                    return false;
                }
                size_t index = this->writePosition;
                for (size_t i = 0; i < bytes; ++i)
                {
                    buffer[(index + i) & ringBufferMask] = data[i];
                }
                index = (index + bytes) & ringBufferMask;

                for (size_t i = 0; i < sizeof(bytes2); ++i)
                {
                    buffer[(index + i) & ringBufferMask] = ((char *)&bytes2)[i];
                }
                index = (index + sizeof(bytes2)) & ringBufferMask;

                for (size_t i = 0; i < bytes2; ++i)
                {
                    buffer[(index + i) & ringBufferMask] = data2[i];
                }
                {
                    std::lock_guard lock{mutex};
                    this->writePosition = (index + bytes2) & ringBufferMask;
                }
                if (SEMAPHORE_READER)
                {
                    cvRead.notify_all();
                }
                return true;
            }
            else
            {
                if (writeSpace() <= sizeof(bytes2) + bytes + bytes2)
                {
                    return false;
                }
                size_t index = this->writePosition;
                for (size_t i = 0; i < bytes; ++i)
                {
                    buffer[(index + i) & ringBufferMask] = data[i];
                }
                index = (index + bytes) & ringBufferMask;

                for (size_t i = 0; i < sizeof(bytes2); ++i)
                {
                    buffer[(index + i) & ringBufferMask] = ((char *)&bytes2)[i];
                }
                index = (index + sizeof(bytes2)) & ringBufferMask;

                for (size_t i = 0; i < bytes2; ++i)
                {
                    buffer[(index + i) & ringBufferMask] = data2[i];
                }
                {
                    std::lock_guard lock{mutex};
                    this->writePosition = (index + bytes2) & ringBufferMask;
                }

                if (SEMAPHORE_READER)
                {
                    cvRead.notify_all();
                }
                return true;
            }
        }

        bool read(size_t bytes, uint8_t *data)
        {
            if (readSpace() < bytes)
                return false;
            int64_t readPosition = this->readPosition;
            for (size_t i = 0; i < bytes; ++i)
            {
                data[i] = this->buffer[(readPosition + i) & this->ringBufferMask];
            }
            {
                std::lock_guard lock{mutex};
                this->readPosition = (readPosition + bytes) & this->ringBufferMask;
            }
            return true;
        }
        ~RingBuffer()
        {
#ifdef USE_MLOCK
            if (this->mlocked)
            {
                munlock(buffer, ringBufferSize);
            }
#endif

            delete[] buffer;
        }
        bool isReadReady() {
            std::lock_guard lock(mutex);
            if (isReadReady_()) return true;
            return !this->is_open;
        }
        bool isReadReady(size_t size) {
            size_t available = readSpace();
            return available >= size;
        }

    private:

        size_t readSpace_()
        {
            int64_t size = writePosition - readPosition;
            if (size < 0)
                size += this->ringBufferSize;
            return size_t(size);
        }

        uint32_t peekSize()
        {
            volatile uint32_t result;
            uint8_t *p = (uint8_t*)&result;
            size_t ix = this->readPosition;
            for (size_t i = 0; i < sizeof(result); ++i)
            {
                *p++ = this->buffer[(ix++) & ringBufferMask];
            }
            return result;
        }
        bool isReadReady_()
        {
            size_t available = readSpace_();
            if (available < sizeof(uint32_t)) return false;
            // peak to get the size!
            uint32_t packetSize = peekSize();
            return packetSize+sizeof(uint32_t) <= available;
        }


    };


};
