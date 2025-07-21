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

#pragma once

#include <mutex>
#include <atomic>
#include <filesystem>

namespace pipedal
{

    /**
     * @brief Crash protection.
     *
     * Detect the situation where PiPedal has crashed multiple times due to SIG_SEGVS caused by a plugin.
     *
     * The basic premise: mark segments of code as "crash protected". If Pipedal shuts down without exiting the
     * crash guard zone for some fixed number of occasions (3?), then set a flag that will prevent
     * loading of the default plugin when PiPedal next loads. Since PiPedal restarts indefinitely while
     * running as a service, this allows users to recover after they have committed a plugin that crashes
     * into their current preset.
     *
     */

    class CrashGuard
    {
    public:
        static void SetCrashGuardFileName(const std::filesystem::path &path);
        static bool HasCrashed();
        static void ClearCrashFlag();

        static void EnterCrashGuardZone();
        static void LeaveCrashGuardZone();

    private:
        static int crashCount;
        static std::mutex mutex;
        static std::filesystem::path fileName;
        static int64_t depth;
    };

    class CrashGuardLock {
    public:
        CrashGuardLock() {
            CrashGuard::EnterCrashGuardZone();
        }
        ~CrashGuardLock() {
            CrashGuard::LeaveCrashGuardZone();
        }
        CrashGuardLock(const CrashGuardLock &) = delete;
        CrashGuardLock &operator=(const CrashGuardLock &) = delete;
        CrashGuardLock(CrashGuardLock &&) = delete;
        CrashGuardLock &operator=(CrashGuardLock &&) = delete;
    };
};