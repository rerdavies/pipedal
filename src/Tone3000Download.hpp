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
#include <cstdint>
#include <string>
#include <vector>
#include "json.hpp"
#include <filesystem>
#include <chrono>

namespace pipedal
{
    namespace tone3000
    {
        using tone3000_time_point = std::string; // Date in ISO 8601 format. (C++ doesn't handle timezones)
        class Tone3000Model
        {
        private:
            int64_t id_ = -1;
            std::string name_;
            std::string model_url_;
            tone3000_time_point created_at_;
            std::string size_;
            std::string user_id_;

        public:
            JSON_GETTER_SETTER(id)
            JSON_GETTER_SETTER_REF(name)
            JSON_GETTER_SETTER_REF(model_url)
            JSON_GETTER_SETTER_REF(created_at)
            JSON_GETTER_SETTER_REF(size)
            JSON_GETTER_SETTER_REF(user_id)

            DECLARE_JSON_MAP(Tone3000Model);
        };

        class Tone3000User
        {
        private:
            std::string id_;
            std::string avatar_url_;
            std::string username_;
            std::string url_;

        public:
            JSON_GETTER_SETTER_REF(id)
            JSON_GETTER_SETTER(username)
            JSON_GETTER_SETTER(url)

            DECLARE_JSON_MAP(Tone3000User);
        };

        class Tone3000Download
        {
        private:
            int64_t id_ = -1;
            std::string user_id_;
            std::string title_;
            std::string platform_;
            std::string description_;
            tone3000_time_point created_at_;
            tone3000_time_point updated_at_;
            std::string gear_;
            std::vector<std::string> images_;
            bool is_public_ = true;
            std::vector<std::string> links_;
            int64_t model_count_ = 0;
            int64_t favorites_count_ = 0;
            std::string license_;
            std::vector<std::string> sizes_;
            Tone3000User user_;
            std::vector<Tone3000Model> models_;

        public:
            JSON_GETTER_SETTER(id)
            JSON_GETTER_SETTER_REF(user_id)
            JSON_GETTER_SETTER_REF(title)
            JSON_GETTER_SETTER_REF(platform)
            JSON_GETTER_SETTER_REF(description)
            JSON_GETTER_SETTER_REF(created_at)
            JSON_GETTER_SETTER_REF(updated_at)
            JSON_GETTER_SETTER_REF(gear)
            JSON_GETTER_SETTER_REF(images)
            JSON_GETTER_SETTER(is_public)
            JSON_GETTER_SETTER_REF(links)
            JSON_GETTER_SETTER(model_count)
            JSON_GETTER_SETTER(favorites_count)
            JSON_GETTER_SETTER_REF(license)
            JSON_GETTER_SETTER_REF(sizes)
            JSON_GETTER_SETTER_REF(user)
            JSON_GETTER_SETTER_REF(models)

            DECLARE_JSON_MAP(Tone3000Download);
        };

        class Tone3000DownloadResult {
        private:
            bool success_;
            std::string errorMessage_;
        public:
            JSON_GETTER_SETTER(success);
            JSON_GETTER_SETTER_REF(errorMessage);

            Tone3000DownloadResult();
            Tone3000DownloadResult(const std::exception &e);

            DECLARE_JSON_MAP(Tone3000DownloadResult);
        };

        extern void DownloadTone3000Bundle
        (
            const std::filesystem::path&destinationFolder,
            const std::string &downloadUrl
        );
    }

}