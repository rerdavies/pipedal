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

#include "CpuGovernor.hpp"
#include "ss.hpp"
#include <fstream>
#include <filesystem>
#include "PiPedalException.hpp"
#include <unistd.h>

using namespace pipedal;


static const int SYSFS_RETRIES = 3;

bool pipedal::HasCpuGovernor()
{
#ifdef __WIN32__
    return false;
#else 
    std::filesystem::path sysFsPath = SS("/sys/devices/system/cpu/cpu" << 0 << "/cpufreq/scaling_governor");
    return std::filesystem::exists(sysFsPath);

#endif
}


std::string pipedal::GetCpuGovernor()
{
    std::string result;
    try {
        std::ifstream f("/sys/devices/system/cpu/cpu0/cpufreq/scaling_governor");
        if (!f.is_open())
        {
            return "";
        }
        f >> result;
    } catch (const std::exception &)
    {
        return result;
    }
    return result;

}

static bool writeAndVerify(std::filesystem::path&sysfsPath, const std::string &value)
{
    // return if the value has already been set.
    {
        std::ifstream f;
        f.open(sysfsPath);
        if (f.is_open())
        {
            std::string line;
            std::getline(f,line);
            if (!f.fail() && value == line)
            {
                return true;
            }
        }
    }
    for (int i = 0; i < SYSFS_RETRIES; ++i)
    {
        {
            // write the value.
            // write the value.
            std::ofstream f;
            f.open(sysfsPath);
            if (f.is_open())
            {
                f << value;
            } else {
                std::cout << SS("Can't open " << sysfsPath << " for writing.") << std::endl;
                sleep(1);
                continue;
            }
        }
        // verify that we wrote it
        {
            std::ifstream f;
            f.open(sysfsPath);
            if (f.is_open())
            {
                std::string line;
                std::getline(f,line);
                if (value == line)
                {
                    return true;
                }
                std::cout << "Read value: " << line << "Expected value: " << value << std::endl;
            }
            std::cout << "Failed to update  " << sysfsPath << std::endl;
        }
        sleep(1);
    }    
    return false;
} 

void pipedal::SetCpuGovernor(const std::string &governor) { 
    // using sysfs 
    int nCpu = 0;

    while (true)
    {
        std::filesystem::path base = SS("/sys/devices/system/cpu/cpu" << nCpu);
        if (!std::filesystem::is_directory(base)) 
            break;

        std::filesystem::path sysFsPath = SS("/sys/devices/system/cpu/cpu" << nCpu << "/cpufreq/scaling_governor");
        if (!std::filesystem::exists(sysFsPath))
        {
            return;
        }

        if (!writeAndVerify(sysFsPath,governor))
        {
            throw PiPedalException(SS("Write to " << sysFsPath << " failed."));
            break;
        }            
        ++nCpu;
    }
}
std::vector<std::string> pipedal::GetAvailableGovernors() { 
    if (!HasCpuGovernor())
    {
        return std::vector<std::string>();
    }
    return std::vector<std::string> {
        "performance",
        "ondemand",
        "powersave"
    };
}
