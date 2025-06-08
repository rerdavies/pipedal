/*
 *   Copyright (c) 2023 Robin E. R. Davies
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
#include "util.hpp"
#include "ss.hpp"

#include <pthread.h>
#include <thread>

#include <unistd.h> // for gettid()
#include <codecvt>
#include <sstream>

using namespace pipedal;


void pipedal::SetThreadName(const std::string &name)
{
    std::string threadName = "ppdl_" + name;
    if (threadName.length () > 15)
    {
        threadName = threadName.substr(0,15);
    }
    pthread_t pid = pthread_self();
    pthread_setname_np(pid,threadName.c_str());
}


static const uint8_t utf8extraBytes[256] = {
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0, 0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
    1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1, 1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
    2,2,2,2,2,2,2,2,2,2,2,2,2,2,2,2, 3,3,3,3,3,3,3,3,4,4,4,4,5,5,5,5
};

constexpr char32_t ILLEGAL_CHAR32 = U'⊗';
static const uint8_t utf8Offset[] = { 0,0b11000000, 0b11100000,0b11110000,0b11111000,0b11111100};

std::u32string pipedal::ToUtf32(const std::string &s)
{
    std::basic_stringstream<char32_t> result;

    auto p = s.begin();
    auto end = s.end();

    while (p != end)
    {
        uint8_t c = (uint8_t)(*p++);
        if (c < 0x80)
        {
            result << (char32_t)c;
        } else {
            auto extraBytes = utf8extraBytes[c];
            if (extraBytes == 0)
            {
                result << ILLEGAL_CHAR32;
            } else if (p+extraBytes > end)
            {
                result << ILLEGAL_CHAR32;
                break;
            } else {
                char32_t cResult = c -= utf8Offset[extraBytes];
                while (extraBytes != 0)
                {
                    c = *p++;
                    --extraBytes;
                    if (c < 0x80 || c >= 0xC0)
                    {
                        result << ILLEGAL_CHAR32;
                        p += extraBytes;
                        break;
                    }
                    cResult = (cResult << 6) + (c & 0x3F);
                }
                result << cResult;
            }
        }
    }
    return result.str();
}


std::string pipedal::GetHostName()
{
    char buffer[1024];
    if (gethostname(buffer,1024) != 0)
    {
        buffer[0] = '\0';
    }
    buffer[1023] = '\0';
    return buffer;
}


std::vector<std::string> pipedal::split(const std::string &value, char delimiter)
{
    size_t start = 0;
    std::vector<std::string> result;
    while (start < value.length())
    {
        size_t pos = value.find_first_of(delimiter, start);
        if (pos == std::string::npos)
        {
            result.push_back(value.substr(start));
            break;
        }
        result.push_back(value.substr(start, pos - start));
        start = pos + 1;
        if (start == value.length())
        {
            // ends with delimieter? Then there's an empty-length value at the end.
            result.push_back("");
        }
    }
    return result;
}


bool pipedal::IsChildDirectory(const std::filesystem::path &path, const std::filesystem::path&rootPath)
{
    auto iter = path.begin();

    for (auto i = rootPath.begin(); i != rootPath.end(); ++i)
    {
        if (iter == path.end() || *iter != *i)
        {
            return false;
        }
        ++iter;
    }
    return true;
}


std::string pipedal::ToLower(const std::string&value)
{
    std::string result;
    result.resize(value.length());
    for (size_t i = 0; i < value.length(); ++i)
    {
        result[i] = (char)tolower(value[i]);
    }
    return result;
}


std::filesystem::path pipedal::MakeRelativePath(const std::filesystem::path &path, const std::filesystem::path&parentPath)
{
    if (path.is_relative()) return path;

    auto iter = path.begin();
    
    for (auto i = parentPath.begin(); i != parentPath.end(); ++i)
    {
        if (iter == path.end() || *iter != *i) {
            // not a child directory of parent directory.
            throw std::runtime_error("Not a child directory.");
        }
        ++iter;
    }
    std::filesystem::path remander;
    while (iter != path.end())
    {
        remander /= *iter++;
    }
    return remander;

}

bool pipedal::IsSubdirectory(const std::filesystem::path &path, const std::filesystem::path &basePath)
{
    auto iPath = path.begin();
    for (auto i = basePath.begin(); i != basePath.end(); ++i)
    {
        if (iPath == path.end())
        {
            return false;
        }

        if ((*i) != (*iPath))
        {
            return false;
        }
        ++iPath;
    }
    while (iPath != path.end())
    {
        if (iPath->string() == "..")
        {
            return false;
        }
        ++iPath;
    }
    return true;
}


bool pipedal::HasWritePermissions(const std::filesystem::path &path)
{
    // posix, but may not work on windows. 
    // allegedly windows provies an _access function, which is probably a superset of
    // access.
    return access(path.c_str(), W_OK) == 0;
}

