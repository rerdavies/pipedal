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

#include "Tone3000Throttler.hpp"

using namespace pipedal;

static constexpr size_t THROTTLING_LIMIT = 80;
static constexpr size_t BULK_DOWNLOAD_LIMIT = 20; 


Tone3000Throttler Tone3000Throttler::instance_;

void Tone3000Throttler::ClearExpiredDownloads_()
{
    auto expiryTime = clock_t::now();
    expiryTime -= std::chrono::seconds(60);

    while (downloadTimes.size() > 0 && downloadTimes[0] < expiryTime) {
        downloadTimes.erase(downloadTimes.begin());
    }

}
void Tone3000Throttler::AddDownload()
{
    std::lock_guard lock{m_mutex};
    this->downloadTimes.push_back(clock_t::now());
}

bool Tone3000Throttler::IsThrottled() 
{
    std::lock_guard lock{m_mutex};

    ClearExpiredDownloads_();
    return this->downloadTimes.size() >= THROTTLING_LIMIT; 
}

size_t Tone3000Throttler::ReserveDownloadSlots(size_t requested)
{
    ClearExpiredDownloads_();
    size_t result;

    size_t reserved = this->downloadTimes.size();
    if (reserved == THROTTLING_LIMIT) {
        return 0;
    }
    if (reserved < BULK_DOWNLOAD_LIMIT)
    {
        if (requested+reserved >= BULK_DOWNLOAD_LIMIT)
        {
            result =  BULK_DOWNLOAD_LIMIT-reserved;
            if (result == 0)
            {
                result = 1;
            }
        } else {
            result = requested;            
        }
    } else {
        result = 1; // download one at a time!
    }
    auto now = clock_t::now();
    for (size_t i = 0; i < result; ++i)
    {
        this->downloadTimes.push_back(now);
    }
    return result;
}

size_t Tone3000Throttler::AvailableDownloadSlots() {
    ClearExpiredDownloads_();
    return THROTTLING_LIMIT-this->downloadTimes.size();
}