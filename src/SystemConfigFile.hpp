// Copyright (c) 2022 Robin Davies
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

class SystemConfigFile {
    std::filesystem::path currentPath;
    std::vector<std::string> lines;
    void SetLine(int64_t lineIndex,const std::string&key,const std::string &value );
    bool LineMatches(const std::string &line, const std::string&key) const;
public:
    SystemConfigFile() {

    }
    SystemConfigFile(const std::filesystem::path& path)
    {
        Load(path);
    }

    void Load(std::istream &input);
    void Load(const std::filesystem::path&path);

    void Save(std::ostream &os);
    void Save(const std::filesystem::path&path);
    void Save();

    int64_t GetLine(const std::string &key) const;
    bool HasValue(const std::string&key) const;
    bool Get(const std::string&key,std::string*pResult) const;
    std::string Get(const std::string&key) const;
    const std::string& Get(int position) const { return lines[position]; }
    void Set(const std::string&key,const std::string &value);
    void Set(const std::string&key,const std::string &value, const std::string&comment);
    void SetDefault(const std::string&key, const std::string &value);
    void SetDefault(const std::string&key,const std::string &value, const std::string &comment);

    bool Erase(const std::string&key);
    int64_t Insert(int64_t position, const std::string&key, const std::string&value);
    int64_t Insert(int64_t position, const std::string&line);
    int GetLineNumber(const std::string&line) const;
    int GetLineNumberStartingWith(const std::string&line) const;
    bool HasLine(const std::string&line) const { return GetLineNumber(line) != -1; }
    void EraseLine(int i);
    int GetLineThatStartsWith(const std::string&text) const;
    int GetLineCount() const { return (int)lines.size();}
    const std::string & GetLineValue(int line) const { return lines[line]; }
    bool EraseLine(const std::string&line);
    void InsertLine(int position, const std::string&line);
    void AppendLine(const std::string&line);
    void SetLineValue(int index, const std::string&line) { lines[index] = line; }

    void UndoableReplaceLine(int line,const std::string&text);
    int UndoableAddLine(int line,const std::string&text);
    bool RemoveUndoableActions();
};

};