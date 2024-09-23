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

#include <filesystem>
#include <chrono>
#include <cstdint>
#include "json.hpp"

#pragma once
namespace pipedal
{
    class GithubAsset
    {
    public:
        GithubAsset(json_variant &v);
        std::string name;
        std::string browser_download_url;
        std::string updated_at;
    };
    class GithubRelease
    {
    public:
        GithubRelease(json_variant &v);

        const GithubAsset *GetDownloadForCurrentArchitecture() const;
        const GithubAsset *GetGpgKeyForAsset(const std::string &name) const;

        bool draft = true;
        bool prerelease = true;
        std::string name;
        std::string url;
        std::string version;
        std::string body;
        std::vector<GithubAsset> assets;
        std::string published_at;
    };


    class GithubResponseHeaders
    {
    public:
        GithubResponseHeaders() {}
        GithubResponseHeaders(const std::filesystem::path path);
        int code_ = -1;
        std::chrono::system_clock::time_point date_;
        std::chrono::system_clock::time_point ratelimit_reset_;
        uint64_t ratelimit_limit_ = 60;
        uint64_t ratelimit_remaining_ = 60;
        uint64_t ratelimit_used_ = 0;
        std::string ratelimit_resource_;
        bool limit_exceeded() const { return code_ != 200 && ratelimit_limit_ != 0 && ratelimit_limit_ == ratelimit_used_; }
        void Load(const std::filesystem::path &filename);
        void Save(const std::filesystem::path &filename);
        DECLARE_JSON_MAP(GithubResponseHeaders);

    private:
        static std::filesystem::path FILENAME;
    };
}

