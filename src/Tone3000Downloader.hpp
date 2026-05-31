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
#include "Tone3000DownloadProgress.hpp"
#include "Tone3000DownloadType.hpp"
#include "json.hpp"
#include "Uri.hpp"

namespace pipedal
{

    class Tone3000PkceParams
    {
    private:
        std::string publishableKey_;
        std::string redirectUrl_;
        std::string codeVerifier_;
        std::string codeChallenge_;
        std::string state_;

    public:
        Tone3000PkceParams() = default;
        Tone3000PkceParams(const std::string &redirectUrl);

        JSON_GETTER_SETTER_REF(publishableKey);
        JSON_GETTER_SETTER_REF(redirectUrl);
        JSON_GETTER_SETTER_REF(codeVerifier);
        JSON_GETTER_SETTER_REF(codeChallenge);
        JSON_GETTER_SETTER_REF(state);

        DECLARE_JSON_MAP(Tone3000PkceParams);
    };
    class Tone3000ModelInfo
    {
    private:
        std::string url_;
        std::string name_;

    public:
        JSON_GETTER_SETTER_REF(url);
        JSON_GETTER_SETTER_REF(name);

        DECLARE_JSON_MAP(Tone3000ModelInfo);
    };

    std::string Sha256Base64Url(const std::string&input);
    
    // class Tone3000DownloadRequest
    // {
    // private:
    //     int64_t downloadType_;
    //     std::string downloadPath_;
    //     std::string codeVerifier_;
    //     std::string state_;
    //     std::string codeChallenge_;

    //     std::string authToken_;
    //     int64_t id_;
    //     std::string title_;
    //     std::string description_;
    //     std::string imageUrl_;
    //     std::vector<Tone3000ModelInfo> models_;

    //     std::string updated_at_;
    //     std::string user_;
    //     std::vector<std::string> sizes_;
    //     std::string platform_;
    //     std::string gear_;
    //     std::string license_;
    //     std::vector<std::string> links_;

    // public:
    //     Tone3000DownloadType downloadType() const { return (Tone3000DownloadType)downloadType_;}
    //     void downloadType(Tone3000DownloadType value) { downloadType_= (int64_t)value;}
    //     JSON_GETTER_SETTER_REF(downloadPath);
    //     JSON_GETTER_SETTER(id);
    //     JSON_GETTER_SETTER(codeVerifier);
    //     JSON_GETTER_SETTER(state);
    //     JSON_GETTER_SETTER(codeChallenge);
    //     JSON_GETTER_SETTER(authToken);
    //     JSON_GETTER_SETTER_REF(title);
    //     JSON_GETTER_SETTER_REF(description)
    //     JSON_GETTER_SETTER_REF(imageUrl);
    //     JSON_GETTER_SETTER_REF(models);

    //     JSON_GETTER_SETTER_REF(updated_at);
    //     JSON_GETTER_SETTER_REF(user);
    //     JSON_GETTER_SETTER_REF(sizes);
    //     JSON_GETTER_SETTER_REF(platform);
    //     JSON_GETTER_SETTER_REF(gear);
    //     JSON_GETTER_SETTER_REF(license);
    //     JSON_GETTER_SETTER_REF(links);

    //     DECLARE_JSON_MAP(Tone3000DownloadRequest);
    // };

    class Tone3000Downloader
    {
    protected:
        Tone3000Downloader();

    public:
        using handle_t = int64_t;

        virtual ~Tone3000Downloader();

        class Listener
        {
        public:
            virtual void OnStartTone3000Download(
                handle_t handle,
                const std::string &title) = 0;
            virtual void OnTone3000Progress(
                const Tone3000DownloadProgress &downloadProgress) = 0;
            virtual void OnTone3000DownloadComplete(
                handle_t handle,
                const std::string &resultPath) = 0;

            virtual void OnTone3000DownloadError(
                handle_t handle,
                const std::string &errorMessage) = 0;

            virtual std::string Tone3000ThumbnailDirectory() = 0;
            virtual std::string OldTone3000ThumbnailDirectory() = 0;
        };

        using self = Tone3000Downloader;
        static std::shared_ptr<self> Create();

        virtual void SetListener(Listener *listener) = 0;

        virtual void CancelDownload(
            handle_t handle) = 0;

        virtual handle_t RequestTone3000Download(
            const uri &uri,
            const Tone3000PkceParams &pkceParams,
            const std::string &downloadPath,
            Tone3000DownloadType downloadType) = 0;

        virtual void Close() = 0;
        virtual Tone3000DownloadProgress GetDownloadStatus() = 0;
    };
}