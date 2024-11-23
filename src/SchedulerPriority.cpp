// Copyright (c) 2024 Robin E. R. Davies
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

#include "SchedulerPriority.hpp"
#include "Lv2Log.hpp"
#include "memory.h"
#include "sched.h"
#include "ss.hpp"
#include <stdexcept>

#ifdef __linux__
#include "unistd.h" // for nice()`
#endif

using namespace pipedal;

static constexpr int EXPECTED_RT_THREAD_PRIORITY_MAX = 81;

static constexpr int RT_AUDIO_THREAD_PRIORITY = 80;
static constexpr int NICE_AUDIO_THREAD_PRIORITY = -19;

static constexpr int RT_AUDIOSERVICE_THREAD_PRIORITY = 10;
static constexpr int NICE_AUDIOSERVICE_THREAD_PRIORITY = -17;

static constexpr int RT_LV2SCHEDULER_THREAD_PRIORITY = -1;
static constexpr int NICE_LV2SCHEDULER_THREAD_PRIORITY = 2; // nice(2): don't let plugins screw things up by pinning the CPU 

static constexpr int RT_WEBSERVER_THREAD_PRIORITY = -1;
static constexpr int NICE_WEBSERVER_THREAD_PRIORITY = -2; // don't get bogged down by UI threads.

bool pipedal::IsRtPreemptKernel(SchedulerPriority priority)
{
    #ifdef __linux__
        struct oldPolicy;
        struct sched_param oldParam = {0};

        struct sched_param param = {0};

        auto oldPolicy = sched_getscheduler(0);
        if (sched_getparam(0,&oldParam) == -1)
        {
            return false;
        }
        auto priorityMin = sched_get_priority_min(SCHED_RR);
        param.sched_priority = priorityMin;
        if (sched_setscheduler(0,SCHED_RR,&param) == -1)
        {
            return false;
        }
        sched_setscheduler(0,oldPolicy,&oldParam);
        return true;

    #else
        return true; // let SetPrority sort out whether it can or can't.
    #endif
}

static void SetPriority(int realtimePriority, int nicePriority, const char *priorityName)
{

    if (realtimePriority != -1)
    {
        int initialPriority = realtimePriority;
        int min = sched_get_priority_min(SCHED_RR);
        int max = sched_get_priority_max(SCHED_RR);
        if (realtimePriority > max) {
            realtimePriority = max + EXPECTED_RT_THREAD_PRIORITY_MAX-realtimePriority;
        }
        if (realtimePriority < min) {
            realtimePriority = min;
        }
        if (initialPriority != realtimePriority)
        {
            Lv2Log::warning(
                SS("Realtime priority adjusted from " << initialPriority << " to " << realtimePriority << ". (" << priorityName << ")")
                );
        }

        struct sched_param param;
        memset(&param, 0, sizeof(param));
        param.sched_priority = realtimePriority;

        int result = sched_setscheduler(0, SCHED_RR, &param);
        if (result == 0)
        {
            Lv2Log::debug("Service thread priority successfully boosted.");
            return;
        }
        else
        {
            Lv2Log::debug(SS("Failed to set RT thread priority for " << priorityName << " (" << strerror(result) << ")"));
        }
    }
    // If the RT scheduler isn't working, fall back to using NICE priority.
    int result = nice(nicePriority);
    if (result == -1)
    {
        Lv2Log::error(SS("Failed Failed to set thread priority. (" << priorityName << ")"));
    }
}

void pipedal::SetThreadPriority(SchedulerPriority priority)
{

#if defined(__linux__)
    switch (priority)
    {
    case SchedulerPriority::RealtimeAudio:
        SetPriority(RT_AUDIO_THREAD_PRIORITY, NICE_AUDIO_THREAD_PRIORITY, "RealtimeAudio");
        break;
    case SchedulerPriority::AudioService:
        SetPriority(RT_AUDIOSERVICE_THREAD_PRIORITY, NICE_AUDIOSERVICE_THREAD_PRIORITY, "AudioService");
        break;
    case SchedulerPriority::Lv2Scheduler:
        SetPriority(RT_LV2SCHEDULER_THREAD_PRIORITY, NICE_LV2SCHEDULER_THREAD_PRIORITY, "Lv2Scheduler");
        break;
    case SchedulerPriority::WebServerThread:
        SetPriority(RT_WEBSERVER_THREAD_PRIORITY, NICE_WEBSERVER_THREAD_PRIORITY, "Lv2Scheduler");
        break;
    default:
        Lv2Log::error("Invalid scheduler priority.");
        throw std::runtime_error("Invalid value.");
    }
#elif defined(__WIN32)
// Realtime thread priority must be set using MM scheduler.
// Others must run with elevated priority, but not realtime priority (MUST run with higher priority than UI threads).
#error "Realtime scheduling not implmented for WIN32"
#else
#error "Realtime scheduling not implmented for your platform"
#endif
}
