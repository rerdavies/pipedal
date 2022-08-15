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

#include "ss.hpp"
#include "Lv2Log.hpp"

namespace pipedal
{

    constexpr int RT_THREAD_PRIORITY = 80;
    /**
     * @brief RAII Priority boost to avoid Rt-thread priority inversion
     * Boosts current thread to realtime priority for the duration of the current scope.
     * Do this before acquiring a mutex that the RT thread might block on.
     *
     */

    class RtInversionGuard
    {
    private:
        int currentScheduler;
        struct sched_param currentPriority;
        ;

    public:
        RtInversionGuard()
        {
            currentScheduler = sched_getscheduler(0);
            sched_getparam(0, &currentPriority);

            struct sched_param param;
            memset(&param, 0, sizeof(param));
            param.sched_priority = RT_THREAD_PRIORITY;

            int result = sched_setscheduler(0, SCHED_RR, &param);
            if (result != 0)
            {
                Lv2Log::error(SS("Failed to set ALSA AudioThread priority. (" << strerror(result) << ")"));
            }
        }
        ~RtInversionGuard()
        {
            sched_setscheduler(0, currentScheduler, &currentPriority);
        }
    };
}