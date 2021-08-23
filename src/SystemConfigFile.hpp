// Copyright (c) 2021 Robin Davies
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
#include <boost/filesystem.hpp>

namespace pipedal {

class SystemConfigFile {
    boost::filesystem::path currentPath;
    std::vector<std::string> lines;
    void SetLine(int64_t lineIndex,const std::string&key,const std::string &value );
    bool LineMatches(const std::string &line, const std::string&key) const;
public:
    SystemConfigFile() {

    }
    SystemConfigFile(const boost::filesystem::path& path)
    {
        Load(path);
    }

    void Load(const boost::filesystem::path&path);

    void Save(std::ostream &os);
    void Save(const boost::filesystem::path&path);
    void Save();

    int64_t GetLine(const std::string &key) const;
    bool HasValue(const std::string&key) const;
    bool Get(const std::string&key,std::string*pResult) const;
    std::string Get(const std::string&key) const;
    void Set(const std::string&key,const std::string &value);
    void Set(const std::string&key,const std::string &value, bool overwrite);
    void Set(const std::string&key,const std::string &value, const std::string&comment);
    bool Erase(const std::string&key);
    int64_t Insert(int64_t position, const std::string&key, const std::string&value);
    int64_t Insert(int64_t position, const std::string&line);

};

};