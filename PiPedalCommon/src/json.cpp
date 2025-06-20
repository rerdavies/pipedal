// Copyright (c) 2022 Robin E. R. Davies
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

#include "json.hpp"
#include <string_view>
#include <cctype>
#include "json_variant.hpp"
#include "util.hpp"
#include <string_view>

using namespace pipedal;




uint32_t json_writer::continuation_byte(std::string_view::iterator &p, std::string_view::const_iterator end)
{
    if (p == end)
        throw_encoding_error();
    uint8_t c = *p++;
    if ((c & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_BITS)
        throw_encoding_error();
    return c & 0x3F;
}
static char hex(int v)
{
    if (v < 10)
        return (char)('0' + v);
    return (char)('A' + v - 10);
}

void json_writer::throw_encoding_error()
{
    throw std::invalid_argument("Invalid UTF-8 character sequence");
}
void json_writer::write_utf16_char(uint16_t uc)
{
    os << "\\u"
       << hex((int)((uc >> 12) & 0x0F))
       << hex((int)((uc >> 8) & 0x0F))
       << hex((int)((uc >> 4) & 0x0F))
       << hex((int)((uc)&0x0F));
}

void json_writer::write(string_view v,bool enforceValidUtf8Encoding)
{
    // convert to utf-32.
    // convert utf-32 to normalized utf-16.
    // write non-7-bit and unsafe characters as \uHHHH.

    auto p = v.begin();
    os << '"';
    while (p != v.end())
    {
        uint32_t uc;
        uint8_t c = (uint8_t)*p++;
        if ((c & UTF8_ONE_BYTE_MASK) == UTF8_ONE_BYTE_BITS)
        {
            uc = c;
        }
        else
        {
            uint32_t c2 = continuation_byte(p, v.end());

            if ((c & UTF8_TWO_BYTES_MASK) == UTF8_TWO_BYTES_BITS)
            {
                uint32_t c1 = c & (uint32_t)(~UTF8_TWO_BYTES_MASK);
                if (c1 <= 1 && enforceValidUtf8Encoding)
                {
                    // overlong encoding.
                    throw_encoding_error();
                }
                uc = (c1 << 6) | c2;
            }
            else
            {
                uint32_t c3 = continuation_byte(p, v.end());

                if ((c & UTF8_THREE_BYTES_MASK) == UTF8_THREE_BYTES_BITS)
                {
                    uint32_t c1 = c & (uint32_t)~UTF8_THREE_BYTES_MASK;
                    if (c1 == 0 && c2 < 0x20 && enforceValidUtf8Encoding)
                    {
                        // overlong encoding.
                        throw_encoding_error();
                    }

                    uc = (c1) << 12 | (c2 << 6) | c3;
                }
                else
                {
                    uint32_t c4 = continuation_byte(p, v.end());
                    if ((c & UTF8_FOUR_BYTES_MASK) == UTF8_FOUR_BYTES_BITS)
                    {
                        uint32_t c1 = c & (uint32_t)~UTF8_FOUR_BYTES_MASK;
                        if (c1 == 0 && c2 < 0x10 && enforceValidUtf8Encoding)
                        {
                            // overlong encoding.
                            throw_encoding_error();
                        }
                        uc  = (c1 << 18) | (c2 << 12) | (c3 << 6) | c4;
                    } else {
                        // outside legal UCS range. 
                        throw_encoding_error();
                    }
                }
            }
        }
        if ((uc >= UTF16_SURROGATE_1_BASE && uc <= UTF16_SURROGATE_1_BASE + UTF16_SURROGATE_MASK) || (uc >= UTF16_SURROGATE_2_BASE && uc <= UTF16_SURROGATE_2_BASE + UTF16_SURROGATE_MASK))
        {
            // MUST not encode UTF16 surrogates in UTF8.
            throw_encoding_error();
        }

        if (uc == '"' || uc == '\\')
        {
            os << (char)'\\';
            os << (char)uc;
        }
        else if (uc >= 0x20 && uc < 0x80)
        {
            os << (char)uc;
        } 
        else if (uc == '\r')
        {
            os << "\\r";
        }
        else if (uc == '\n')
        {
            os << "\\n";
        }
        else if (uc == '\t')
        {
            os << "\\t";
        }
        else if (uc < 0x10000ul)
        {
            write_utf16_char((uint16_t)uc);
        }
        else
        {
            // write UTF-16 surrogate pair.
            uc -= 0x10000;
            
            uint16_t s1 = (uint16_t)(UTF16_SURROGATE_1_BASE + ((uc >> 10) & 0x3FFu));
            uint16_t s2 = (uint16_t)(UTF16_SURROGATE_2_BASE + (uc & 0x03FFu));
            // surrogate pair.
            write_utf16_char(s1);
            write_utf16_char(s2);
        }
    }
    os << '"';
}

void json_writer::indent()
{
    if (!compressed)
    {
        for (int i = 0; i < indent_level; ++i)
        {
            os << " ";
        }
    }
}

void json_writer::start_object()
{
    os << "{" << CRLF;
    indent_level += TAB_SIZE;
}
void json_writer::end_object()
{
    indent_level -= TAB_SIZE;
    os << CRLF;
    indent();
    os << "}";
}

void json_writer::start_array()
{
    indent();
    os << "[" << CRLF;
    indent_level += TAB_SIZE;
}
void json_writer::end_array()
{
    indent_level -= TAB_SIZE;
    indent();
    os << "]" << CRLF;
}


void json_reader::skip_whitespace()
{
    char c;
    while (true)
    {
        int ic = is_.peek();
        if (ic == -1)
            break;
        if (is_whitespace((char)ic))
        {
            c = get();
        }
        else if (ic == '/')
        {
            get();
            int c2 = is_.peek();
            if (c2 == '/') {
                // skip to end of line.
                get();
                while (true) {
                    c2 = is_.peek();
                    if (c2 == '\r' || c2 == '\n')
                    {
                        get(); // and continue.
                        break;
                    }
                    if (c2 == -1) 
                    {
                        break;
                    }
                    get();
                }
            } else if (c2 == '*') {
                get();
                int level = 1;
                while (true)
                {
                    c = get();
                    if (c == '*' && is_.peek() == '/')
                    {
                        get();
                        if (--level == 0)
                        {
                            break;
                        }
                    }
                    if (c == '/' && is_.peek() == '*')
                    {
                        get();
                        ++level;
                    }
                }
            } else {

                throw_format_error();
            }
        } else
        {

            break;
        }
    }
}

static void utf32_to_utf8_stream(std::ostream &s, uint32_t uc)
{
    if (uc < 0x80u)
    {
        s << (char)uc;
    } else if (uc < 0x800u) {
        s << (char)(0xC0 + (uc >> 6));
        s << (char)(0x80 + (uc & 0x3F));

    } else if (uc < 0x10000u) {
        s << (char)(0xE0 + (uc >> 12));
        s << (char)(0x80 + ((uc >> 6) & 0x3F));
        s << (char)(0x80 + (uc & 0x3F));
    } else if (uc < 0x0110000) {
        s << (char)(0xF0 + (uc >> 18));
        s << (char)(0x80 + ((uc >> 12) & 0x3F));
        s << (char)(0x80 + ((uc >> 6) & 0x3F));
        s << (char)(0x80 + (uc & 0x3F));
    } else {
        throw std::range_error("Illegal UTF-32 character.");
    }
}

std::string json_reader::read_string()
{
    // To completely normalize UTF-32 values we must covert to UTF-16, resolve surrogate pairs, and then convert UTF-32 to UTF-8.


    skip_whitespace();
    char c;
    char startingCharacter;
    startingCharacter = get();
    if (startingCharacter != '\'' && startingCharacter != '\"')
    {
        throw_format_error();
    }
    std::stringstream s;

    while (true)
    {
        c = get();
        if (c == startingCharacter)
        {
            if (is_.peek() == startingCharacter) //  "" -> "
            {
                get();
                s.put(c);
            } else {
                break;
            }
        }
        if (c != '\\')
        {
            s << (char)c;
        }
        else
        {
            c = get();
            switch (c)
            {
            case '"':
            case '\\':
            default:
                s << c;
                break;
            case 'r':
                s << '\r';
                break;
            case 'b':
                s << '\b';
                break;
            case 'f':
                s << '\f';
                break;
            case 'n':
                s << '\n';
                break;
            case 't':
                s << '\t';
                break;
            case 'u':
            {
                uint32_t uc = read_u_escape();
                if (uc >= UTF16_SURROGATE_1_BASE && uc <= UTF16_SURROGATE_1_BASE + UTF16_SURROGATE_MASK)
                {
                    // MUST be a UTF16_SURROGATE 2 to be legal.
                    c = get();
                    if (c != '\\')
                        throw_format_error("Invalid UTF16 surrogate pair");
                    c = get();
                    if (c != 'u')
                        throw_format_error("Invalid UTF16 surrogate pair");
                    uint16_t uc2 = read_u_escape();
                    if (uc2 < UTF16_SURROGATE_2_BASE || uc2 > UTF16_SURROGATE_2_BASE + UTF16_SURROGATE_MASK)
                    {
                        throw_format_error("Invalid UTF16 surrogate pair");
                    }
                    uc = ((uc & UTF16_SURROGATE_MASK) << 10) + (uc2 & UTF16_SURROGATE_MASK) + 0x10000U;
                }
                utf32_to_utf8_stream(s, uc);
            }
            break;
            }
        }
    }
    return s.str();
}
uint16_t json_reader::read_hex()
{
    char c;
    c = get();
    if (c >= '0' && c <= '9')
        return c - '0';
    if (c >= 'A' && c <= 'F')
        return c - 'A' + 10;
    if (c >= 'a' && c <= 'f')
        return c - 'a' + 10;
    throw_format_error("Invalid \\u escape character");
    return 0;
}

uint16_t json_reader::read_u_escape()
{
    uint16_t c1 = read_hex();
    uint16_t c2 = read_hex();
    uint16_t c3 = read_hex();
    uint16_t c4 = read_hex();

    return (c1 << 12) + (c2 << 8) + (c3 << 4) + c4;
}

bool json_reader::is_complete()
{
    skip_whitespace();
    return is_.peek() == -1;
}

void json_reader::consumeToken(const char*expectedToken, const char*errorMessage)
{
    skip_whitespace();
    
    const char*p = expectedToken;
    while (*p != '\0')
    {
        char expectedChar = *p++;
        int c = is_.get();
        if (expectedChar != c) {
            this->throw_format_error(errorMessage);
        }
    }
}

void json_reader::consume(char expected)
{
    skip_whitespace();
    char c;
    c = get();
    if (c != expected)
    {
        std::stringstream s;
        s << "Expecting '" << expected << "'";
        throw_format_error(s.str().c_str());
    }
}

void json_reader::skip_property()
{
    skip_whitespace();
    int c = is_.peek();
    switch (c)
    {
    case -1:
        throw_format_error("Premature end of file.");
        break;
    case '[':
        skip_array();
        break;
    case '{':
        skip_object();
        break;
    case '\"':
        skip_string();
        break;
    case 't':
    case 'f':
        read_boolean();
        break;
    case 'n':
        read_null();
        break;
    default:
        skip_number();
        break;
    }
}
void json_reader::skip_string()
{
    consume('"');
    while (true)
    {
        int c;
        c = get();
        if (c == -1) throw_format_error("Premature end of file.");

        if (c == '\"')
        {
            if (peek() == '\"')
            {
                get();
            } else {
                break;
            }
        }
        if (c == '\\')
        {
            c = get(); // all of standard escapes,  enough to get past \u
        }
    }
}

void json_reader::skip_number()
{
    skip_whitespace();
    int c;
    if (is_.peek() == '-')
    {
        get();
    }
    if (!std::isdigit(is_.peek()))
    {
        throw_format_error("Expecting a number.");
    }
    while (std::isdigit(is_.peek()))
    {
        get();
    }
    if (is_.peek() == '.')
    {
        get();
    }
    while (std::isdigit(is_.peek()))
    {
        get();
    }
    c = is_.peek();
    if (c == 'e' || c == 'E')
    {
        get();
        c = is_.peek();
        if (c == '+' || c == 'i')
        {
            get();
        }
        while (std::isdigit(is_.peek()))
        {
            get();
        }

    }
}
void json_reader::skip_array()
{
    int c;
    consume('[');

    while (true)
    {
        c = peek();
        if (c == -1) throw_format_error("Premature end of file.");

        if (c == ']')
        {
            c = get();
            break;
        }
        skip_property();
        skip_whitespace();
        if (is_.peek() == ',')
        {
            c = get();
        }
    }
}

void json_reader::skip_object()
{
    int c;
    consume('{');
    while (true)
    {
        c = peek();
        if (c == -1) throw_format_error("Premature end of file.");
        if (c == '}')
        {
            c = get();
            break;
        }
        skip_string(); // name.
        consume(':');
        skip_object();
        if (peek() == ',')
        {
            consume(',');
        }
    }
}

void json_reader::read_object_start()
{
    skip_whitespace();

    char c;
    c = get();
    if (c != '{')
        throw_format_error();
}

std::string json_reader::readToken()
{
    skip_whitespace();

    std::stringstream s;
    while (true)
    {
        int ic = is_.peek();
        if (ic == -1)
            break;
        char c = (char)ic;
        if (!std::isalpha(c))
            break;
        is_.get();
        s << c;
    }
    return s.str();

}

void json_reader::read_null()
{
    std::string s = readToken();
    if (s != "null")
    {
        throw_format_error("Format error. Expecting 'null'.");

    }
}
bool json_reader::read_boolean()
{
    std::string ss = readToken();
    if (ss == "true")
        return true;
    if (ss == "false")
        return false;

    throw_format_error("Format error. Expectiong 'true' or 'false'");
    return false;
}

void json_reader::throw_format_error(const char*error)
{
    std::stringstream s;
    s << error;
    s << ", near: '";
    skip_whitespace();
    if (is_.peek() == -1) {
        s << "<eof>";
    } else {
        for (int i = 0; i < 40; ++i)
        {
            int c = get();
            if (c == -1) break;
            if (c == '\r') {
                s << "\\r";
            } else if (c == '\n')
            {
                s << "\\n";
            } else {
                s << (char)c;
            }
        }
    }
    s << "'.";
    std::string message = s.str();
    throw JsonException(message);

}

static std::string timePointToISO8601(const std::chrono::system_clock::time_point &tp)
{
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::gmtime(&tt);
    std::stringstream ss;
    ss << std::put_time(&tm,"%Y-%m-%dT%H:%M:%S") << "Z";
    return ss.str();
    
}
static std::chrono::system_clock::time_point ISO8601ToTimePoint(const std::string&timeString)
{
    if (timeString.empty())
    {
        return std::chrono::system_clock::time_point();
    }
    if (timeString.back() != 'Z')
    {
        throw std::runtime_error("Time string is not in UTC Z timezone. Time-zones are not supported.");
    }

    std::tm tm = {};
    std::istringstream ss(timeString);
    ss >> std::get_time(&tm,"%Y-%m-%dT%H:%M:%S"); 
    if (ss.fail())
    {
        throw std::runtime_error("Failed to parse ISO 8601 time string.");
    }
    time_t tt;
    #ifdef _WIN32
    tt = _mkgmtime(&tm);
    #else 
    tt = timegm(&tm);
    #endif
    if (tt == -1)
    {
        throw std::runtime_error("Failed to conver ISO 8601 time string.");
    }
    return std::chrono::system_clock::from_time_t(tt);
}

void json_writer::write (const std::chrono::system_clock::time_point &time)
{
    std::string timeString = timePointToISO8601(time);
    write(timeString);
}

void json_reader::read(std::chrono::system_clock::time_point *value)
{
    std::string timeString;
    read(&timeString);
    *value = ISO8601ToTimePoint(timeString);
}
        


// void json_writer::write(const json_variant &value)
// {
//     ((JsonSerializable *)&value)->write_json(*this);
// }
