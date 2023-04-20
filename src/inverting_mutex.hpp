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

/// @brief A mutex that handles priority-inversion.
/// The thread priority of a thread that holds the mutex is boosted to the highest priority of waiting threads, thereby avoiding priority inversion.
///

#ifdef WIN32
static_assert("Fix me!");
/// Windows has no such concept. The strategy will probably be to boost the priority of worker threads from
/// Nice(2) to something realtime, or work out a non-locking alternative.
/// Currently, the principle problem is the LV2 Worker thread (Pipedal project), which runs at nice(2) priority, which may cause priority inversions on
/// the realtime thread. Of some concern would be threads of BalancedConvolution (ToobAmp project). Longer convolution sections run below the
/// priority of the ALSA threads on linux, while shorter sections run above the priority of the ALSA thread. The convolution threads run
/// at high priority anyway, so priority inversion probably isn't a problem, even on Windows.
#endif

#include <pthread.h>
#include <stdexcept>
#include <string.h>
#include <chrono>
#include <ratio>
#include <condition_variable>

class inverting_mutex
{
public:
    using native_handle_type = pthread_mutex_t *;

    inverting_mutex()
    {

        pthread_mutexattr_t mta;

        int rc = pthread_mutexattr_init(&mta);
        if (rc != 0)
            throw_system_error(rc);

        rc = pthread_mutexattr_setprotocol(&mta, PTHREAD_PRIO_INHERIT);
        if (rc != 0)
            throw_system_error(rc);

        rc = pthread_mutex_init(&mutex, &mta);
        if (rc != 0)
            throw_system_error(rc);
    }
    ~inverting_mutex()
    {
        pthread_mutex_destroy(&mutex);
    }

    inverting_mutex(const inverting_mutex &) = delete;
    inverting_mutex &operator=(const inverting_mutex &) = delete;

    void
    lock()
    {
        int e = pthread_mutex_lock(&mutex);

        // EINVAL, EAGAIN, EBUSY, EINVAL, EDEADLK(may)
        if (e)
            throw_system_error(e);
    }

    bool
    try_lock() noexcept
    {
        // XXX EINVAL, EAGAIN, EBUSY
        int rc = pthread_mutex_trylock(&mutex);
        switch (rc)
        {
        case 0:
            return true;
        case EBUSY:
            return false;
        default:
            throw_system_error(rc);
            return false;
        }
    }

    template <class Rep, class Period>
    bool
    try_lock_for(const std::chrono::duration<Rep, Period> &rtime)
    {
        using clock = std::chrono::steady_clock;
        auto rt = std::chrono::duration_cast<clock::duration>(rtime);
        if (std::ratio_greater<clock::period, Period>())
            ++rt;

        auto t = clock::now() + rt;
        return try_lock_until(t);
    }

    template <class Clock, class Duration>
    bool
    try_lock_until(const std::chrono::time_point<Clock, Duration> &atime)
    {
        auto s = std::chrono::time_point_cast<std::chrono::seconds>(atime);
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(atime - s);

        timespec ts = {
            static_cast<std::time_t>(s.time_since_epoch().count()),
            static_cast<long>(ns.count())};

        return !pthread_mutex_timedlock(&mutex, &ts);
    }

    // template <class Rep, class Period>
    // std::cv_status cond_wait_for(std::condition_variable &cond, const std::chrono::duration<Rep, Period> &rtime)
    // {
    //     using clock = std::chrono::steady_clock;
    //     auto rt = std::chrono::duration_cast<clock::duration>(rtime);
    //     if (std::ratio_greater<clock::period, Period>())
    //         ++rt;

    //     auto t = clock::now() + rt;
    //     return cont_wait_until(cond,t);

    // }
    // template <class Clock, class Duration>
    // std::cv_status cont_wait_until(std::condition_variable &cond,const std::chrono::time_point<Clock, Duration> &atime)
    // {
    //     auto s = std::chrono::time_point_cast<std::chrono::seconds>(atime);
    //     auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(atime - s);

    //     auto now = Clock::now();
    //     auto sNow = std::chrono::time_point_cast<std::chrono::seconds>(now);
    //     auto nsNow = std::chrono::duration_cast<std::chrono::nanoseconds>(now - s);

    //     timespec tsNow = {
    //         static_cast<std::time_t>(sNow.time_since_epoch().count()),
    //         static_cast<long>(nsNow.count())};
    //     (void)tsNow;



    //     timespec ts = {
    //         static_cast<std::time_t>(s.time_since_epoch().count()),
    //         static_cast<long>(ns.count())};
        
    //     int rc = pthread_cond_timedwait(cond.native_handle(),this->native_handle(),&ts);
    //     switch (rc)
    //     {
    //         case 0:
    //             return std::cv_status::no_timeout;
    //         case ETIMEDOUT:
    //             return std::cv_status::timeout;
    //         default:
    //             throw_system_error(rc);
    //             return std::cv_status::timeout;

    //     }
    // }
    void
    unlock()
    {
        // XXX EINVAL, EAGAIN, EBUSY
        int rc = pthread_mutex_unlock(&mutex);
        if (rc != 0)
        {
            throw_system_error(rc);
        }
    }

    native_handle_type
    native_handle() noexcept
    {
        return &mutex;
    }

private:
    void throw_system_error(int e)
    {
        throw std::logic_error(strerror(e));
    }
    pthread_mutex_t mutex;
};