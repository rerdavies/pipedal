/*
 *   Copyright (c) 2025 Robin E. R. Davies
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

#include "CrashGuard.hpp"
#include "ofstream_synced.hpp"
#include <fstream>
#include <Lv2Log.hpp>
#include <chrono>

using namespace pipedal;
namespace fs = std::filesystem;

int CrashGuard::crashCount = 0;
std::mutex CrashGuard::mutex;
std::filesystem::path CrashGuard::fileName;
int64_t CrashGuard::depth = 0;

static uint64_t getCurrentTimeMillis()
{
    using namespace std::chrono;
    return duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}   

void CrashGuard::SetCrashGuardFileName(const std::filesystem::path &path)
{
    CrashGuard::fileName = path;

    {
        std::ifstream f{path};
        uint64_t crashTime = 0;

        uint64_t currentTime = getCurrentTimeMillis() ;

        if (f.is_open())
        {
            f >> crashCount >> crashTime;
            if (crashTime > currentTime) 
            {
                Lv2Log::info("CrashGuard: power-off reset detected. Ignoring crash count.");
                crashTime = currentTime; // Pi 4 without an RTC!
                crashCount = 0;
            }
        }

        // don't count power-off resets, since this is normal practice fo a pi.
        // we're only interested in tight-loop crashes while running under systemd.

        constexpr uint64_t TIME_LIMIT = 1000 * 60 * 10; // 10 minutes.

        uint64_t elapsed = currentTime - crashTime;
        if (elapsed > TIME_LIMIT)
        {
            crashCount = 0;
        }
        if (elapsed < 3*1000 && crashCount > 0) {
            // if less than 3 seconds, assume it's a power-off reset, since
            // the systemd retry is 5 seconds.
            Lv2Log::info("CrashGuard: power-off reset detected. Ignoring crash count.");
            crashCount = 0;
        }

        if (crashCount > 0) {
            Lv2Log::info("CrashGuard: Detected previous crash, count = %d", (int)crashCount);
        }
    }
}
bool CrashGuard::HasCrashed()
{
    return crashCount > 4;
}
void CrashGuard::ClearCrashFlag()
{
    crashCount = 0;
    try
    {
        if (fs::exists(fileName))
        {
            fs::remove(fileName);
        }
    }
    catch (const std::exception &ignored)
    {
    }
}

void CrashGuard::EnterCrashGuardZone()
{
    std::lock_guard lock{mutex};

    if (depth++ == 0)
    {
        if (!fileName.empty()) {
            ofstream_synced f{fileName};
            f << (crashCount + 1) << " " << getCurrentTimeMillis() << std::endl;
        }
    }
}
void CrashGuard::LeaveCrashGuardZone()
{
    std::lock_guard lock{mutex};
    if (--depth == 0)
    {
        try
        {
            if (!fileName.empty() && fs::exists(fileName))
            {
                fs::remove(fileName);
            }
        }
        catch (const std::exception &ignored)
        {
        }
    }
}
