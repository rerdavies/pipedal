/*
 *   Copyright (c) Robin E.R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.

 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#pragma once

#include <vector>
#include <chrono>
#include <mutex>

namespace pipedal
{
    class Tone3000Throttler
    {
    private:
        Tone3000Throttler() = default;
        Tone3000Throttler(const Tone3000Throttler&) = delete;
        Tone3000Throttler(Tone3000Throttler&&) = delete;
        Tone3000Throttler&operator=(const Tone3000Throttler&) = delete;

    public:
        static Tone3000Throttler&instance() { return instance_; }

        using clock_t = std::chrono::steady_clock;

        void AddDownload();
        size_t ReserveDownloadSlots(size_t requested);
        size_t AvailableDownloadSlots();
        bool IsThrottled();

    private:
        std::mutex m_mutex;
        static Tone3000Throttler instance_;

        void ClearExpiredDownloads_();
        std::vector<clock_t::time_point> downloadTimes;
    };

}