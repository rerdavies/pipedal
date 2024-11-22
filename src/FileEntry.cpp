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

#include "FileEntry.hpp"

using namespace pipedal;

JSON_MAP_BEGIN(FileEntry)
    JSON_MAP_REFERENCE(FileEntry,pathname)
    JSON_MAP_REFERENCE(FileEntry,displayName)
    JSON_MAP_REFERENCE(FileEntry,isProtected)
    JSON_MAP_REFERENCE(FileEntry,isDirectory)
JSON_MAP_END()

JSON_MAP_BEGIN(BreadcrumbEntry)
    JSON_MAP_REFERENCE(BreadcrumbEntry,pathname)
    JSON_MAP_REFERENCE(BreadcrumbEntry,displayName)
JSON_MAP_END()


JSON_MAP_BEGIN(FileRequestResult)
    JSON_MAP_REFERENCE(FileRequestResult,files)
    JSON_MAP_REFERENCE(FileRequestResult,isProtected)
    JSON_MAP_REFERENCE(FileRequestResult,breadcrumbs)
    JSON_MAP_REFERENCE(FileRequestResult,currentDirectory)
JSON_MAP_END()
