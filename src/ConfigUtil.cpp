/*
 * MIT License
 * 
 * Copyright (c) 2022 Robin E. R. Davies
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "ConfigUtil.hpp"
#include <fstream>
#include <sstream>

using namespace pipedal;
using namespace std;

static std::string getKey(const std::string &line, size_t length)
{
    std::stringstream s;
    size_t start = 0;
    size_t end = length;
    char c;
    while (start < end && (line[start] == ' ' || line[start] == '\t'))
        ++start;

    while (end > start && (line[end - 1] == ' ' || line[end - 1] == '\t'))
        --end;

    return line.substr(start, end - start);
}
static std::string trim(const std::string &value)
{
    size_t start = 0;
    size_t end = value.length();

    while (start < end && ( value[start] == ' ' || value[start] == '\t'))
        ++start;

    while (end > start && (value[end - 1] == ' ' || value[end - 1] == '\t'))
        --end;

    return value.substr(start, end - start);
}
static std::string unquote(const std::string &value)
{
    if (value.length() > 2)
    {
        if (
            (value[0] == '\'' && value[value.length() - 1] == '\'') || (value[0] == '\"' && value[value.length() - 1] == '\"'))
        {
            char quotChar = value[0];

            std::stringstream ss;
            for (size_t i = 1; i < value.length() - 1; ++i)
            {
                char c = value[i];
                if (c == '\\')
                {
                    ++i;
                    c = value[i];
                    switch (c)
                    {
                    case 'r':
                        c = '\r';
                        break;
                    case 'n':
                        c = '\n';
                        break;
                    case 't':
                        c = '\t';
                        break;
                    default:
                        break;
                    }
                    ss << c;
                }
                else
                {
                    ss << c;
                }
            }
            return ss.str();
        }
        
    }
    return value;
}
bool ConfigUtil::GetConfigLine(const std::string & filePath,const std::string & key, std::string *pValue)
{

    ifstream f;
    f.open(filePath);
    if (f.is_open())
    {
        std::string line;

        while (true)
        {
            if (f.eof() || f.fail())
            {
                break;
            }
            getline(f, line);
            auto pos = line.find_first_of('#');
            if (pos != std::string::npos)
            {
                line = line.substr(0, pos);
            }
            pos = line.find_first_of('=');
            if (pos != std::string::npos)
            {
                std::string thisKey = getKey(line, pos);
                if (thisKey == key)
                {
                    std::string value = line.substr(pos + 1);

                    *pValue = unquote(trim(value));
                    return true;
                }
            }
        }
    }
    return false;
}