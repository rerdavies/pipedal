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

#include "Tone3000DownloaderImpl.hpp"
#include "Tone3000DownloadProgress.hpp"
#include "Tone3000Download.hpp"
#include "Uri.hpp"
#include <functional>

using namespace pipedal;

std::shared_ptr<Tone3000Downloader> Tone3000Downloader::Create()
{
    return std::make_shared<Tone3000DownloaderImpl>();
}

Tone3000Downloader::Tone3000Downloader()
{
}

Tone3000Downloader::~Tone3000Downloader()
{
}

Tone3000DownloaderImpl::~Tone3000DownloaderImpl()
{
    Close();
    this->thread = nullptr;
}

void Tone3000DownloaderImpl::SetListener(Listener *listener)
{
    std::lock_guard<std::mutex> lockGuard{this->mutex};
    this->listener = listener;
}

Tone3000DownloaderImpl::handle_t Tone3000DownloaderImpl::NextHandle()
{
    handle_t result = this->nextHandle++;
    return result;
}
Tone3000DownloaderImpl::handle_t Tone3000DownloaderImpl::RequestDownload(
    const std::string &path,
    const std::string &url)
{
    handle_t handle;
    {
        std::lock_guard<std::mutex> lockGuard{this->mutex};
        handle = NextHandle();
        std::shared_ptr<DownloadRequest> request = std::make_shared<DownloadRequest>(false, handle, path, url);

        this->requestQueue.push_back(request);
        request = nullptr;
        if (!this->thread)
        {
            this->thread = std::make_unique<std::jthread>([this]
                                                          { this->ThreadProc(); });
        }
    }
    this->thread_cv.notify_one();
    return handle;
}

void Tone3000DownloaderImpl::CancelDownload(
    handle_t handle)
{
    std::lock_guard<std::mutex> lockGuard{this->mutex};
    for (auto it = requestQueue.begin(); it != requestQueue.end(); ++it)
    {
        if ((*it)->handle == handle)
        {
            requestQueue.erase(it);
            return;
        }
    }
    if (activeRequest)
    {
        if (activeRequest->handle == handle)
        {
            activeRequest->cancelled = true;
        }
    }
}

void Tone3000DownloaderImpl::Close()
{
    bool notify = false;
    {
        std::lock_guard<std::mutex> lockGuard{this->mutex};
        if (!closed)
        {
            closed = true;
            notify = true;
        }

        this->requestQueue.resize(0);

        Tone3000DownloadProgress progress;
        fgDownloadProgress = progress;

        if (listener)
        {
            listener->OnTone3000Progress(progress);
        }
        if (this->activeRequest != nullptr)
        {
            activeRequest->cancelled = true;
        }
    }
    if (notify)
    {
        thread_cv.notify_one();
    }
}
Tone3000DownloadProgress Tone3000DownloaderImpl::GetDownloadStatus()
{
    Tone3000DownloadProgress result;
    {
        std::lock_guard<std::mutex> lockGuard{this->mutex};
        result = fgDownloadProgress;
    }
    return result;
}

void Tone3000DownloaderImpl::bgUpdateDownloadProgress(const Tone3000DownloadProgress &progress)
{
    std::lock_guard<std::mutex> lockGuard{this->mutex};
    if (closed)
    {
        return;
    }
    if (activeRequest != nullptr && !activeRequest->cancelled)
    {
        this->fgDownloadProgress = progress;
        if (listener)
        {
            listener->OnTone3000Progress(this->fgDownloadProgress);
        }
    }
}

std::string Tone3000DownloaderImpl::PerformDownload(
    const std::string &downloadPath,
    const std::string &downloadUrl,
    int64_t downloadHandle,
    std::function<bool()> isCancelled)
{
    std::string downloadedDirectory;
    // Validate Tone3000 URL
    uri downloadUri{downloadUrl};
    if (!downloadUri.authority().ends_with(".tone3000.com"))
    {
        throw std::runtime_error("Invalid Tone3000 URL address.");
    }

    // Check for cancellation before starting
    if (isCancelled())
    {
        return downloadedDirectory;
    }

    Tone3000DownloadProgress progress;
    this->bgUpdateDownloadProgress(progress);
    // Perform the download using existing code
    downloadedDirectory = tone3000::DownloadTone3000Bundle(
        downloadPath,
        downloadUrl,
        downloadHandle,
        [this](const Tone3000DownloadProgress &progress)
        {
            this->bgUpdateDownloadProgress(progress);
        },
        [isCancelled]()
        {
            return isCancelled();
        });

    // Check for cancellation after download
    if (isCancelled())
    {
        // maybe there's a partial result.
        return downloadedDirectory; 
    }
    return downloadedDirectory;
}

void Tone3000DownloaderImpl::ThreadProc()
{
    while (true)
    {
        std::shared_ptr<DownloadRequest> request;
        Listener *currentListener = nullptr;

        Tone3000DownloadProgress progress;

        {
            std::unique_lock<std::mutex> lock{this->mutex};
            thread_cv.wait(lock, [this]()
                           { return this->closed || this->requestQueue.size() != 0; });

            if (closed)
            {
                return;
            }

            if (requestQueue.empty())
            {
                continue;
            }

            // Dequeue the next request
            request = requestQueue.front();
            requestQueue.erase(requestQueue.begin());
            activeRequest = request;
            currentListener = listener;
        }

        // Notify download started
        if (currentListener)
        {
            currentListener->OnStartTone3000Download(request->handle, request->downloadUrl);
        }

        // Create cancellation predicate that checks the atomic flag
        auto isCancelled = [request]() -> bool
        {
            return request->cancelled.load();
        };

        try
        {
            std::string outputPath = PerformDownload(request->downloadPath, request->downloadUrl, request->handle, isCancelled);

            // Notify completion if not cancelled
            {
                std::lock_guard lockGuard{mutex};
                if (!request->cancelled.load() && currentListener)
                {
                    currentListener->OnTone3000DownloadComplete(request->handle,outputPath);
                }
            }
        }
        catch (const std::exception &e)
        {
            {
                std::lock_guard lockGuard{mutex};
                // Notify error
                if (!request->cancelled.load() && currentListener)
                {
                    currentListener->OnTone3000DownloadError(request->handle, e.what());
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock{this->mutex};
            activeRequest = nullptr;
        }
    }
}