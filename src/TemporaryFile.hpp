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
#include <filesystem>
namespace pipedal {
    class TemporaryFile {
    public:
        TemporaryFile() {}
        TemporaryFile(const TemporaryFile&) = delete;
        TemporaryFile&operator=(const TemporaryFile&) = delete;
        TemporaryFile(TemporaryFile&&other);
        TemporaryFile&operator=(TemporaryFile&&other);
    public:
        explicit TemporaryFile(const std::filesystem::path&parentDirectory);

        void Attach(const std::filesystem::path&path) {
            this->path = path;
            deleteFile = true; // default to deleting the file on destruction.
        }
        void SetNonDeletedPath(const std::filesystem::path&path);

        bool DeleteFile() const { return deleteFile; }
        std::filesystem::path Detach();
        ~TemporaryFile();
        const std::filesystem::path&Path()const { return path;}
        std::string str() const { return path.c_str(); }
        const char*c_str() const { return path.c_str(); }
    private:
        bool deleteFile = true;
        std::filesystem::path path;
    };
}