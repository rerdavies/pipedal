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

#pragma once

#include <memory>
#include <cstdint>
#include "Tone3000Downloader.hpp"
#include <thread>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <functional>
#include "ThreadedQueue.hpp"
#include "Tone3000Download.hpp"

namespace pipedal
{

    struct CachedPkceParams
    {
        std::chrono::steady_clock::time_point time;
        Tone3000PkceParams pkceParams;
    };

    class Tone3000AuthResponse
    {
    private:
        std::string access_token_;
        int64_t expires_in_ = -1;
        std::string refresh_token_;
        std::string token_type_;
        std::optional<std::string> tone_id_;
        std::optional<std::string> model_id_;
        bool canceled_ = false;

        using clock_t = std::chrono::steady_clock;
        clock_t::time_point expires_at_;

    public:
        Tone3000AuthResponse(const std::string &responseBody);
        JSON_GETTER_REF(access_token);
        JSON_GETTER(expires_in);
        JSON_GETTER_REF(refresh_token);
        JSON_GETTER_REF(token_type);
        JSON_GETTER_REF(expires_at);
        JSON_GETTER_REF(tone_id);
        JSON_GETTER_REF(model_id);
        JSON_GETTER_SETTER(canceled);

        DECLARE_JSON_MAP(Tone3000AuthResponse);
    };
    class Tone3000AccessTokens
    {
    private:
        std::string access_token_;
        int64_t expires_in_ = -1;
        std::string refresh_token_;
        std::string token_type_;

        using clock_t = std::chrono::steady_clock;
        clock_t::time_point expires_at_;

    public:
        Tone3000AccessTokens(const Tone3000AuthResponse &authResponse);
        JSON_GETTER_REF(access_token);
        JSON_GETTER(expires_in);
        JSON_GETTER_REF(refresh_token);
        JSON_GETTER_REF(token_type);
        JSON_GETTER_REF(expires_at);
    };

    class Tone3000DownloaderImpl : public Tone3000Downloader
    {
    public:
        Tone3000DownloaderImpl();

        virtual ~Tone3000DownloaderImpl();

        virtual void SetListener(Listener *listener) override;

        virtual void CancelDownload(
            handle_t handle) override;

        virtual void Close() override;
        virtual Tone3000DownloadProgress GetDownloadStatus() override;

        virtual handle_t RequestTone3000Download(
            const uri &uri,
            const Tone3000PkceParams &pkceParams,
            const std::string &downloadPath,
            Tone3000DownloadType downloadType) override;

    private:
        struct DownloadRequest
        {
            handle_t handle = -1;
            std::string requestUri;
            Tone3000PkceParams pkceParams;
            std::string downloadPath;
            Tone3000DownloadType downloadType = Tone3000DownloadType::Nam;
            std::atomic<bool> cancelled = false;
        };

        void DownloadTone3000ToneBg(
            std::shared_ptr<DownloadRequest> request);
        void OnTone3000DownloadError(int64_t handle, const std::string &error)
        {
            if (listener)
            {
                listener->OnTone3000DownloadError(handle, error);
            }
        }
        std::shared_ptr<Tone3000Download> GetTone3000Tone(
            int64_t toneId);
        std::string DownloadTone3000Files(
            const Tone3000FileDownloadRequest &request,
            Tone3000DownloadProgress &progress);

        void OnTone3000DownloadComplete(int64_t handle, const std::string &path)
        {
            if (listener)
            {
                listener->OnTone3000DownloadComplete(handle, path);
            }
        }
        void OnTone3000DownloadCancelled(int64_t handle)
        {
            if (listener)
            {
                listener->OnTone3000DownloadComplete(handle, "");
            }
        }
        using clock_t = std::chrono::steady_clock;
        std::recursive_mutex pkceCacheMutex;

        Listener *listener = nullptr;

        void ThreadProc();


        // Performs the actual download with cancellation support
        // std::string PerformFileDownloads(
        //     const Tone3000FileDownloadRequest &downloadRequets,
        //     Tone3000DownloadProgress &progress,
        //     std::function<void(Tone3000DownloadProgress &progress)> updateProgress,
        //     std::function<bool()> isCancelled);

        struct PostResult
        {
            bool ok = false;
            int errorCode = 0;
            std::string error;
            std::string body;
        };

        PostResult Post(
            const uri &requestedUri,
            std::vector<std::string> headers,
            const std::string &body);


        std::string Tone3000GetText(
            const uri &requestedUri
        );
        struct OAuthCallbackResult
        {
            std::optional<int64_t> toneId;
            std::optional<int64_t> modelId;
        };

        OAuthCallbackResult handleOAuthCallback(
            const uri &callbackUri,
            const Tone3000PkceParams &pkce);

        std::atomic<bool> closed = false;
        handle_t nextHandle = 1;
        handle_t NextHandle();

        ThreadedQueue<DownloadRequest> requestQueue;

        Tone3000DownloadProgress fgDownloadProgress;
        void bgUpdateDownloadProgress(const Tone3000DownloadProgress &progress);

        std::shared_ptr<DownloadRequest> activeRequest;

        std::mutex mutex;

        std::unique_ptr<std::jthread> thread;
        std::shared_ptr<Tone3000AccessTokens> accessTokens;

        std::string GetBearerToken() const;
    };
}
