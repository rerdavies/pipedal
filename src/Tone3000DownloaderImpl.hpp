/*
 *   Copyright (c) 2026 Robin E. R. Davies
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

#include <memory>
#include <cstdint>
#include "Tone3000Downloader.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>

namespace pipedal {

    class Tone3000DownloaderImpl: public Tone3000Downloader {
    public:
        virtual ~Tone3000DownloaderImpl();
        
        virtual void SetListener(Listener*listener) override;

        virtual handle_t RequestDownload(
            const std::string &path,
            const std::string &url
        ) override;

        virtual void CancelDownload(
            handle_t handle
        ) override;

        virtual void Close() override;
        virtual Tone3000DownloadProgress GetDownloadStatus() override;

    private:
        Listener *listener = nullptr;

        void ThreadProc();
        
        // Performs the actual download with cancellation support
        std::string PerformDownload(
            const std::string &downloadPath,
            const std::string &downloadUrl,
            int64_t downloadHandle,
            std::function<bool()> isCancelled
        );
        
        struct DownloadRequest {
            std::atomic<bool> cancelled = false;
            handle_t handle;
            const std::string downloadPath;
            const std::string downloadUrl;
        };

        std::atomic<bool> closed = false;
        handle_t nextHandle = 1;
        handle_t NextHandle();

        std::vector<std::shared_ptr<DownloadRequest>> requestQueue;

        Tone3000DownloadProgress fgDownloadProgress;
        void bgUpdateDownloadProgress(const Tone3000DownloadProgress &progress);

        std::shared_ptr<DownloadRequest> activeRequest;

        std::mutex mutex;
        std::condition_variable thread_cv;

        
        std::unique_ptr<std::jthread> thread;
    };
}