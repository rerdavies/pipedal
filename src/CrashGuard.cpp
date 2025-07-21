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

using namespace pipedal;
namespace fs = std::filesystem;

int CrashGuard::crashCount = 0;
std::mutex CrashGuard::mutex;
std::filesystem::path CrashGuard::fileName;
int64_t CrashGuard::depth = 0;

void CrashGuard::SetCrashGuardFileName(const std::filesystem::path &path)
{
    CrashGuard::fileName = path;

    {
        std::ifstream f{path};
        if (f.is_open())
        {
            f >> crashCount;
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
            f << (crashCount + 1) << std::endl;
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
