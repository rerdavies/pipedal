// Copyright (c) 2024 Robin Davies
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
#include <memory>
#include <thread>
#include <mutex>
#include "json.hpp"
#include <chrono>
#include "UpdaterStatus.hpp"


namespace pipedal
{

    class UpdaterImpl;
    class GithubRelease;


    class Updater 
    {
    protected:
        Updater() {};
    public:
        virtual ~Updater() noexcept {}

        using self = Updater;
        using ptr = std::unique_ptr<self>;

        static Updater::ptr Create();
        static void ValidateSignature(const std::filesystem::path&file, const std::filesystem::path&signatureFile);
        
        using UpdateListener = std::function<void(const UpdateStatus &upateResult)>;
        virtual void SetUpdateListener(UpdateListener &&listener) = 0;
        virtual void CheckNow() = 0;
        virtual void Stop() = 0;

        virtual UpdatePolicyT GetUpdatePolicy() = 0;
        virtual void SetUpdatePolicy(UpdatePolicyT updatePolicy) = 0;
        virtual void ForceUpdateCheck() = 0;
        virtual void DownloadUpdate(const std::string &url, std::filesystem::path*file, std::filesystem::path*signatureFile) = 0;
        UpdateStatus virtual GetCurrentStatus() const = 0;


    };


}