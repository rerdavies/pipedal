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
#pragma once
#include <string>
#include <filesystem>
#include <vector>
#include <barrier>
#include <atomic>
#include <set>

namespace pipedal
{

    inline bool endsWith(const std::string &str, const std::string &suffix)
    {
        return str.size() >= suffix.size() && 0 == str.compare(str.size() - suffix.size(), suffix.size(), suffix);
    }
    void SetThreadName(const std::string &name);

    std::u32string ToUtf32(const std::string &s);

    inline bool isPathSeparator(char c) { return c == '/' || c == std::filesystem::path::preferred_separator; }

    std::string GetHostName();

    std::vector<std::string> split(const std::string &value, char delimiter);

    inline void ReadBarrier()
    {
        std::atomic_thread_fence(std::memory_order::acquire);
    }
    inline void WriteBarrier()
    {
        std::atomic_thread_fence(std::memory_order::release);
    }

    template <typename T>
    inline bool contains(const std::vector<T> &vector, const T &value)
    {
        for (auto i = vector.begin(); i != vector.end(); ++i)
        {
            if ((*i) == value)
            {
                return true;
            }
        }
        return false;
    }
    inline bool contains(const std::vector<std::string> &vector, const char *value)
    {
        return contains(vector, std::string(value));
    }

    class NoCopy
    {
    public:
        NoCopy() {}
        NoCopy(const NoCopy &) = delete;
        NoCopy &operator=(const NoCopy &) = delete;
        ~NoCopy() {}
    };
    class NoMove
    {
    public:
        NoMove() {}
        NoMove(NoMove &&) = delete;
        NoMove &operator=(NoMove &&) = delete;
        ~NoMove() {}
    };

    bool IsChildDirectory(const std::filesystem::path &path, const std::filesystem::path&rootPath);

    template <typename T>
    std::set<T> Intersect(const std::set<T> &left, const std::set<T> &right)
    {
        std::set<T> result;
        for (const T &v : left)
        {
            if (right.contains(v))
            {
                result.insert(v);
            }
        }
        return result;
    }
    template <typename T>
    bool IsSubset(const std::set<T> &subset, const std::set<T> &set)
    {
        std::set<T> result;
        for (const T &v : subset)
        {
            if (!set.contains(v)) return false;
        }
        return true;
    }

    // C locale to lower. Only does 'A'-'Z'.
    std::string ToLower(const std::string&value);

    std::filesystem::path MakeRelativePath(const std::filesystem::path &path, const std::filesystem::path&parentPath);
}
