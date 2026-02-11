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

namespace pipedal {

    class Tone3000Downloader {
    protected:
        Tone3000Downloader();
    public:
        using handle_t = int64_t;

        virtual ~Tone3000Downloader();

        class Listener {
        public:
            virtual void OnStartTone3000Download
            (
                handle_t handle,
                const std::string &title
            ) = 0;
            virtual void OnTone3000Progress(
                const Tone3000DownloadProgress& downloadProgress
            ) = 0;
            virtual void OnTone3000DownloadComplete(
                handle_t handle,
                const std::string&resultPath
            ) = 0;
            
            virtual void OnTone3000DownloadError(
                handle_t handle,
                const std::string &errorMessage
            ) = 0;

            virtual std::string Tone3000ThumbnailDirectory()  = 0;

        };


        using self = Tone3000Downloader;
        static std::shared_ptr<self> Create();

        virtual void SetListener(Listener*listener) = 0;

        virtual handle_t RequestDownload(
            Tone3000DownloadType downloadType,
            const std::string &path,
            const std::string &url
        ) = 0;

        virtual void CancelDownload(
            handle_t handle
        ) = 0;

        virtual void Close() = 0;
        virtual Tone3000DownloadProgress GetDownloadStatus() = 0;
    };
}