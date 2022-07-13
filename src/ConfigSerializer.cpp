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

#include "ConfigSerializer.h"


using namespace config_serializer;
using namespace config_serializer::detail;
using namespace std;

std::string config_serializer::detail::trim(const std::string &v)
{
    size_t start = v.find_first_not_of(' ');
    size_t end = v.find_last_not_of(' ');
    if (start >= end+1)
        return "";
    return v.substr(start, end+1 - start);
}

std::vector<std::string> config_serializer::detail::split(const std::string &value, char delimiter)
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



std::string config_serializer::detail::EncodeString(const std::string &s)
{
    bool requiresEncoding = false;

    for (char c : s)
    {
        switch (c)
        {
        case '\0':
            break;
        case '\r':
        case '\t':
        case '\\':
        case ' ':
        case '\"':
            requiresEncoding = true;
            break;
        default:
            break;
        }
    }
    if (!requiresEncoding)
        return s;

    std::stringstream ss;

    ss << '"';
    for (char c : s)
    {
        switch (c)
        {
        case '\0':
            // discard.
            break;
        case '\r':
            ss << "\\r";
            break;
        case '\t':
            ss << "\\t";
            break;
        case '\\':
            ss << "\\\\";
            break;
        case '\"':
            ss << "\\\"";
            break;
        default:
            ss << c;
        }
    }
    ss << '"';
    return ss.str();
}

std::string config_serializer::detail::DecodeString(const std::string &value)
{
    std::stringstream s;
    const char *p = value.c_str();
    char quoteChar;
    if (*p == '\'' || *p == '\"')
    {
        quoteChar = *p;
    }
    else
    {
        return value;
    }
    if (*p != quoteChar)
        return value;
    if (value.at(value.length()-1) != quoteChar)
    {
        throw std::invalid_argument("Invalid quoted string.");
    }
    ++p;
    while (*p != 0 && *p != quoteChar)
    {
        char c = *p++;
        if (c == '\\')
        {
            c = *p++;
            switch (c)
            {
            case 'r':
                s << '\r';
                break;
            case 't':
                s << '\t';
                break;
            case 'n':
                s << '\n';
                break;
            case '\0':
                throw std::invalid_argument("Invalid quoted string.");
                break;
            default:
                s << c;
            }
            if (c != 0)
            {
                s << c;
            }
        }
        else
        {
            s << c;
        }
    }
    return s.str();
}

