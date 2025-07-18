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

#include "json.hpp"
#include "AudioFileMetadata.hpp"

namespace pipedal {

    class FileEntry {
    public:
        FileEntry() { }
        FileEntry(const std::string&pathname,const std::string &displayName,bool isDirectory, bool isProtected = false)
        :pathname_(pathname), displayName_(displayName), isDirectory_(isDirectory),isProtected_(isProtected)
        {

        }
        FileEntry(const std::string&pathname,const std::string &displayName, bool isProtected, 
            std::shared_ptr<AudioFileMetadata> metadata
        )
        :pathname_(pathname), displayName_(displayName), isDirectory_(false),isProtected_(isProtected),metadata_(metadata)
        {

        }
        std::string pathname_;
        std::string displayName_;
        bool isDirectory_ = false;
        bool isProtected_ = false;
        std::shared_ptr<AudioFileMetadata> metadata_;

        DECLARE_JSON_MAP(FileEntry);

    };
    class BreadcrumbEntry {
    public:
        std::string pathname_;
        std::string displayName_;

        DECLARE_JSON_MAP(BreadcrumbEntry);
    };

    class FileRequestResult {
    public:
        std::vector<FileEntry> files_;
        bool isProtected_ = false;
        std::vector<BreadcrumbEntry> breadcrumbs_;
        std::string currentDirectory_;
        DECLARE_JSON_MAP(FileRequestResult);

    };
}