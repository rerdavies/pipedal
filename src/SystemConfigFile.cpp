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

#include "pch.h"
#include "SystemConfigFile.hpp"
#include "PiPedalException.hpp"
#include <fstream>
#include <string>
#include <regex>

using namespace pipedal;
using namespace std;


static std::string makeLine(const std::string &key, const std::string &value)
{
    stringstream s;
    if (value.length() == 0)
    {
        s << key;
    }
    else
    {
        s << key << "=" << value;
    }
    return s.str();
}

void SystemConfigFile::Load(const std::filesystem::path &path)
{

    ifstream f(path);
    if (!f.is_open())
    {
        stringstream s;
        s << "File not found: " << path;
        throw PiPedalException(s.str());
    }
    Load(f);
    this->currentPath = path;
}
void SystemConfigFile::Load(std::istream&f) {
    this->lines.clear();


    while (true)
    {
        std::string line;
        std::getline(f, line);
        if (f.fail())
        {
            break;
        }
        lines.push_back(line);
    }
}

int64_t SystemConfigFile::GetLine(const std::string &key) const
{
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (LineMatches(lines[i], key))
        {
            return i;
        }
    }
    std::string commentedKey = "#" + key;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (LineMatches(lines[i], commentedKey))
        {
            return i;
        }
    }

    return -1;
}

bool SystemConfigFile::HasValue(const std::string &key) const
{
    int line = GetLine(key);
    if (line == -1)
        return false;
    return lines[line][0] != '#';
}

static inline std::string ValuePart(const std::string &line)
{
    auto pos = line.find('=');
    if (pos == std::string::npos)
        throw PiPedalException("Value not found.");
    return line.substr(pos + 1);
}
std::string SystemConfigFile::Get(const std::string &key) const
{
    int64_t lineIndex = GetLine(key);
    if (lineIndex == -1)
        throw PiPedalArgumentException("Not found.");
    return ValuePart(lines[lineIndex]);
}

bool SystemConfigFile::Get(const std::string &key, std::string *pResult) const
{
    int64_t lineIndex = GetLine(key);
    if (lineIndex == -1)
        return false;
    *pResult = ValuePart(lines[lineIndex]);
    return true;
}
void SystemConfigFile::Set(const std::string &key, const std::string &value)
{
    std::string line = makeLine(key, value);

    int lineIndex = GetLine(key);
    if (lineIndex != -1)
    {
        lines[lineIndex] = line;
    }
    else
    {
        lines.push_back(line);
    }
}
void SystemConfigFile::SetDefault(const std::string &key, const std::string &value)
{
    int lineIndex = GetLine(key);
    if (lineIndex == -1)
    {
        lines.push_back(makeLine(key, value));
    } else if (lines[lineIndex][0] != '#') {
        lines[lineIndex] = makeLine(key,value);
    }
}



bool SystemConfigFile::Erase(const std::string &key)
{
    bool matched = false;
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (LineMatches(lines[i], key))
        {
            matched = true;
            lines.erase(lines.begin() + i);
            --i;
        }
    }
    return matched;
}

int64_t SystemConfigFile::Insert(int64_t position, const std::string &key, const std::string &value)
{
    if (position < 0 || position >= lines.size())
    {
        lines.push_back(makeLine(key, value));
        return (int64_t)lines.size();
    }
    else
    {
        lines.insert(lines.begin() + position, makeLine(key, value));
        return position + 1;
    }
}
int64_t SystemConfigFile::Insert(int64_t position, const std::string &line)
{
    if (position < 0 || position >= lines.size())
    {
        lines.push_back(line);
        return (int64_t)lines.size();
    }
    else
    {
        lines.insert(lines.begin() + position, line);
        return position + 1;
    }
}

inline bool SystemConfigFile::LineMatches(const std::string &line, const std::string &key) const
{
    // (very permissive interpretation)
    int pos = 0;
    while (pos < line.length() && (line[pos] == ' ' || line[pos] == '\t'))
    {
        ++pos;
    }
    if (line.compare(pos, pos + key.length(), key) != 0)
        return false;
    pos += key.length();
    while (pos < line.length() && (line[pos] == ' ' || line[pos] == '\t'))
    {
        ++pos;
    }
    if (pos == line.length())
        return true;
    return pos < line.length() && line[pos] == '=';
}

void SystemConfigFile::SetLine(int64_t lineIndex, const std::string &key, const std::string &value)
{
    std::string line = makeLine(key, value);
    if (lineIndex >= 0 && lineIndex < this->lines.size())
    {
        this->lines[lineIndex] = line;
    }
    else
    {
        this->lines.push_back(line);
    }
}


void SystemConfigFile::SetDefault(const std::string &key, const std::string &value, const std::string &comment)
{
    int lineIndex = GetLine(key);
    if (lineIndex == -1)
    {
        lines.push_back("");
        lines.push_back("# " + comment);

        lines.push_back(makeLine(key, value));
    } else {
        if (lines[lineIndex][0] == '#')
        {
            lines[lineIndex] = makeLine(key,value);
        }
    }
}

void SystemConfigFile::Set(const std::string &key, const std::string &value, const std::string &comment)
{
    auto lineIndex = GetLine(key);
    if (lineIndex != -1)
    {
        SetLine(lineIndex, key, value);
    }
    else
    {
        lines.push_back("");
        lines.push_back("# " + comment);
        SetLine(-1, key, value);
    }
}

void SystemConfigFile::Save(std::ostream &os)
{
    for (int i = 0; i < lines.size(); ++i)
    {
        os << lines[i] << std::endl;
    }
}
void SystemConfigFile::Save(const std::filesystem::path &path)
{
    ofstream f(path);

    if (!f.is_open())
    {
        stringstream s;
        s << "Unable to write to " << path;
        throw PiPedalException(s.str());
    }
    Save(f);
    if (f.fail())
    {
        stringstream s;
        s << "Unable to write to " << path;
        throw PiPedalException(s.str());
    }
}
void SystemConfigFile::Save()
{
    Save(this->currentPath);
}

int SystemConfigFile::GetLineNumber(const std::string &line) const {
    for (int i = 0; i < lines.size(); ++i)
    {
        if (lines[i] == line)
        {
            return i;
        }
    }
    return -1;
}

int SystemConfigFile::GetLineNumberStartingWith(const std::string &line) const {
    for (int i = 0; i < lines.size(); ++i)
    {
        if (lines[i].starts_with(line))
        {
            return i;
        }
    }
    return -1;

}

void SystemConfigFile::EraseLine(int i)
{
    if (i < 0 || i >= lines.size()) throw PiPedalArgumentException("Range error.");
    lines.erase(lines.begin()+i);
}

bool SystemConfigFile::EraseLine(const std::string &line)
{
    int lineNumber = GetLineNumber(line);
    if (lineNumber == -1) return false;
    lines.erase(lines.begin() + lineNumber);
    return true;
}

void SystemConfigFile::InsertLine(int position, const std::string&line)
{
    this->lines.insert(lines.begin()+position,line);
}

int SystemConfigFile::GetLineThatStartsWith(const std::string&text) const
{
    for (size_t i = 0; i < lines.size(); ++i)
    {
        if (lines[i].rfind(text,0) != std::string::npos) {
            return (int)i;
        }
    }
    return -1;
}

void SystemConfigFile::AppendLine(const std::string &line)
{
    lines.push_back(line);
}

