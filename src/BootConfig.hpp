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


#pragma once

#include <string>
#include <functional>
#include <thread>
#include <memory>

namespace pipedal {
    class BootConfig {
    public:
        BootConfig();
        ~BootConfig();

        enum class BootLoaderT {
            UBoot,
            Grub,
            Unknown
        };

        enum class DynamicSchedulerT {
            None = 0,
            Voluntary = 1,
            Full = 2,

            NotApplicable = -1,
            Unknown = -2,

        };

        // BEWARE: The onComplete function gets called from off the main thread on machines with GRUB bootloaders!
        void WriteConfiguration(std::function<void(bool success,std::string errorMessage)> onComplete);
        BootLoaderT BootLoader() const { return bootLoader; }
        const std::string&KernelType() const { return kernelType; }

        bool CanSetThreadIrqs() const { return canSetThreadIrqs; }
        bool ThreadedIrqs() const { return threadedIrqs; }
        void ThreadedIrqs(bool value);

        const DynamicSchedulerT DynamicScheduler() const { return dynamicScheduler; }
        void DynamicScheduler(DynamicSchedulerT value);


        bool  CanWriteConfig();

        bool Changed() const { return changed; }

        bool operator==(BootConfig&other) const;
    private:
        std::unique_ptr<std::jthread> thread;

        void WriteUBootConfiguration(std::function<void(bool success,std::string errorMessage)> onComplete);
        void WriteGrubConfiguration(std::function<void(bool success,std::string errorMessage)> onComplete);

        bool changed = false;
        BootLoaderT bootLoader = BootLoaderT::Unknown;
        std::string kernelType;
        std::string preemptMode;
        DynamicSchedulerT dynamicScheduler = DynamicSchedulerT::NotApplicable;
        bool canSetThreadIrqs = false;
        bool threadedIrqs = false;

    };
};