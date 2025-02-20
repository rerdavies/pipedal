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

#include "FilePropertyDirectoryTree.hpp"
#include <filesystem>

using namespace pipedal;
namespace fs = std::filesystem;

FilePropertyDirectoryTree::FilePropertyDirectoryTree()
{
}
FilePropertyDirectoryTree::FilePropertyDirectoryTree(const std::string &directoryName)
    : directoryName_(directoryName),
      displayName_(fs::path(directoryName).filename().string())
{
}

FilePropertyDirectoryTree::FilePropertyDirectoryTree(const std::string &directoryName, const std::string &displayName)
    : directoryName_(directoryName),
      displayName_(displayName)
{
}

JSON_MAP_BEGIN(FilePropertyDirectoryTree)
JSON_MAP_REFERENCE(FilePropertyDirectoryTree, directoryName)
JSON_MAP_REFERENCE(FilePropertyDirectoryTree, displayName)
JSON_MAP_REFERENCE(FilePropertyDirectoryTree, isProtected)
JSON_MAP_REFERENCE(FilePropertyDirectoryTree, children)
JSON_MAP_END()
