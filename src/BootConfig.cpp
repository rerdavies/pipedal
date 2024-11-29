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

#include "BootConfig.hpp"
#include <filesystem>
#include <stdexcept>
#include "ss.hpp"
#include <algorithm>
#include "SysExec.hpp"
#include "util.hpp"
#include <set>
#include <ranges>

namespace fs = std::filesystem;

#include <iostream>
#include <fstream>
#include <string>
#include <filesystem>
#include <cstdlib>
#include <vector>

using namespace pipedal;

static bool IsGrubBootLoader()
{
    const std::vector<std::string> grub_config_paths = {
        "/boot/grub/grub.cfg",
        "/boot/grub2/grub.cfg",
    };

    for (const auto &path : grub_config_paths)
    {
        if (std::filesystem::exists(path))
        {
            return true;
        }
    }
    return false;
}

static std::set<std::string> KNOWN_KERNEL_TYPES = {
    "PREEMPT_NONE",
    "PREEMPT_VOLUNTARY",
    "PREEMPT",
    "PREEMPT_RT",
    "PREEMPT_DYNAMIC"};

std::string GetKernelType()
{
    auto result = sysExecForOutput("uname", "-a");
    if (result.exitCode != EXIT_SUCCESS)
    {
        throw std::runtime_error("Unable to execute 'uname -a'");
    }
    std::vector<std::string> items = split(result.output, ' ');
    for (std::string &item : items)
    {
        if (KNOWN_KERNEL_TYPES.contains(item))
        {
            return item;
        }
    }
    return "Unknown";
}

std::string ReadFileLine(const fs::path &path)
{
    if (!fs::exists(path))
    {
        throw std::runtime_error(SS("File does not exist: " << path ));
    }
    
    std::ifstream f(path);
    if (!f.is_open())
    {
        throw std::runtime_error(SS("Can't read file " << path));
    }
    std::string line;

    std::getline(f, line);
    return line;
}
std::vector<std::string> ReadFileArgs(const fs::path&path)
{
    std::string line = ReadFileLine(path);

    return split(line,' ');
}

using namespace pipedal;
BootConfig::BootConfig()
{
    if (fs::exists("/boot/firmware/cmdline.txt"))
    {
        this->bootLoader = BootLoaderT::UBoot;
    }
    else if (IsGrubBootLoader())
    {
        this->bootLoader = BootLoaderT::Grub;
    }
    else
    {
        this->bootLoader = BootLoaderT::Unknown;
    }

    this->kernelType = GetKernelType();

    if (this->kernelType == "PREEMPT_DYNAMIC")
    {
        this->dynamicScheduler = DynamicSchedulerT::Voluntary; // the default.
    }

    std::string cmdLine = ReadFileLine("/proc/cmdline");
    std::vector<std::string> cmdLineArgs = split(cmdLine, ' ');

    for (const auto &arg : cmdLineArgs)
    {
        if (arg == "preempt=full")
        {
            this->dynamicScheduler = DynamicSchedulerT::Full;
        }
        else if (arg == "preempt=voluntary")
        {
            this->dynamicScheduler = DynamicSchedulerT::Voluntary;
        }
        else if (arg == "preempt=none")
        {
            this->dynamicScheduler = DynamicSchedulerT::None;
        }
        else if (arg.starts_with("preempt="))
        {
            this->dynamicScheduler = DynamicSchedulerT::Unknown;
        }
        else if (arg == "threadirqs")
        {
            this->threadedIrqs = true;
        }
    }
    this->canSetThreadIrqs = this->kernelType == "PREEMPT" || this->kernelType == "PREEMPT_DYNAMIC";
}

BootConfig::~BootConfig()
{
    this->thread = nullptr; // join and clean up the tread before deleting ANYTHING else.
}
bool BootConfig::operator==(BootConfig &other) const
{
    return this->bootLoader == other.bootLoader && this->kernelType == other.kernelType && this->preemptMode == other.preemptMode && this->threadedIrqs == other.threadedIrqs;
}

bool BootConfig::CanWriteConfig()
{
    if (!CanSetThreadIrqs() && dynamicScheduler == DynamicSchedulerT::NotApplicable)
    {
        return true;
    }
    return (this->bootLoader != BootLoaderT::Unknown);
}

void BootConfig::WriteConfiguration(std::function<void(bool success, std::string errorMessage)> onComplete)
{
    this->thread = nullptr; // wait for any previous operations to complete.
    if (!changed)
    {
        if (onComplete) onComplete(true, "");
    }
    else
    {
        changed = false;

        if (this->bootLoader == BootLoaderT::UBoot)
        {
            try
            {
                WriteUBootConfiguration(onComplete);
            }
            catch (const std::exception &e)
            {
                if (onComplete) onComplete(false, e.what());
            }
        }
        else if (this->bootLoader == BootLoaderT::Grub)
        {
            WriteGrubConfiguration(onComplete);
        }
        else
        {
            if (onComplete) onComplete(false, "Unsupported bootloader.");
        }
    }
}

static std::string commandLineOption(BootConfig::DynamicSchedulerT dynamicScheduler)
{
    switch (dynamicScheduler)
    {
        case BootConfig::DynamicSchedulerT::None:
            return "preempt=none";
        case BootConfig::DynamicSchedulerT::Voluntary:
            return "preempt=voluntary";
        case BootConfig::DynamicSchedulerT::Full:
            return "preempt=full";
        default:
            throw std::range_error("Invalid value.");

    }
}
void BootConfig::WriteUBootConfiguration(std::function<void(bool success, std::string errorMessage)> onComplete)
{
    this->thread = nullptr; // blocks if already running.

    this->thread = std::make_unique<std::jthread>(
        [this, onComplete]()
        {

            try
            {
                fs::path cmdlineFile = "/boot/firmware/cmdline.txt";
                std::vector<std::string> bootArgs = ReadFileArgs(cmdlineFile);

                std::vector<std::string> newBootArgs;
                for (const auto&value: bootArgs) 
                {
                    if (value != "threadirqs" && !value.starts_with("preempt=")) 
                    {
                        newBootArgs.push_back(value);
                    }
                }
                if (this->canSetThreadIrqs && this->threadedIrqs)
                {
                    newBootArgs.push_back("threadirqs");
                }
                if (this->dynamicScheduler != DynamicSchedulerT::NotApplicable)
                {
                    newBootArgs.push_back(commandLineOption(this->dynamicScheduler));
                }
                std::stringstream ss;
                bool firstArg = true;
                for (const std::string&arg: newBootArgs) {
                    if (!firstArg) {
                        ss << ' ';
                    }
                    firstArg = false;
                    ss << arg;
                }
                ss << '\n';

                std::string newCmdLine = ss.str();

                {
                    std::ofstream f(cmdlineFile);
                    if (!f.is_open())
                    {
                        throw std::runtime_error(SS("Can't write to " << cmdlineFile));
                    }
                    f << newCmdLine;
                }

                if (onComplete)
                {
                    onComplete(true, "");
                }
            }
            catch (const std::exception &e)
            {
                if (onComplete)
                {
                    onComplete(false, e.what());
                }
            }
        });
}
void BootConfig::WriteGrubConfiguration(std::function<void(bool success, std::string errorMessage)> onComplete)
{
    this->thread = nullptr; // blocks if already running.

    this->thread = std::make_unique<std::jthread>(
        [this, onComplete]()
        {
            sleep(1);

            if (onComplete)
            {
                onComplete(false, "Grub handler not implemeneted.");
            }
        });
}

void BootConfig::DynamicScheduler(DynamicSchedulerT value)
{
    if (value != this->dynamicScheduler)
    {
        if (this->dynamicScheduler == DynamicSchedulerT::NotApplicable)
        {
            throw std::runtime_error("Illegal operation.");
        }
        this->dynamicScheduler = value;
        this->changed = true;
    }
}
void BootConfig::ThreadedIrqs(bool value)
{
    if (value != threadedIrqs)
    {
        if (!CanSetThreadIrqs())
        {
            throw std::runtime_error("Illegal operation.");
        }
        this->threadedIrqs = value;
        this->changed = true;
    }
}
