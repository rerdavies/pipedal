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

#include "HtmlHelper.hpp"
#include <sstream>
#include <string>
#include <cstring>

using namespace pipedal;


// non-localizable. RFC 7231 Date Format
;
static const char*INVARIANT_WEEK_DAYS[] = 
    { "Sun","Mon", "Tue", "Wed","Thu", "Fri","Sat"};
// non-localizable. RFC 7231 Date Format
static const char*INVARIANT_MONTHS[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep","Oct","Nov", "Dec"};
// non-localizable. RFC 7231 Date Format
static const char*FORMAT = "%s, %02d %s %04d %02d:%02d:%02d GMT";

std::string HtmlHelper::timeToHttpDate()
{
    return timeToHttpDate(time(nullptr));
}


std::string HtmlHelper::timeToHttpDate(std::filesystem::file_time_type time)
{

    // Convert to time_t.
    auto sctp = std::chrono::time_point_cast<std::chrono::system_clock::duration>(time - std::filesystem::file_time_type::clock::now() + std::chrono::system_clock::now());

    auto time_t_value = std::chrono::system_clock::to_time_t(sctp);
    return timeToHttpDate(time_t_value);
}

std::string HtmlHelper::timeToHttpDate(time_t time)
{
    // RFC 7231, IMF-fixdate.

    struct tm gmTm;

    if (gmtime_r(&time,&gmTm) == nullptr)
    {
        return "Mon, 01 Jan 1970 00:00:00 GMT";
    }
    char szBuffer[sizeof("Mon, 01 Jan 1970 00:00:00 GMT+")];
    snprintf(szBuffer,sizeof(szBuffer), FORMAT, 
        INVARIANT_WEEK_DAYS[gmTm.tm_wday],
        gmTm.tm_mday,
        INVARIANT_MONTHS[gmTm.tm_mon],
        gmTm.tm_year+1900,
        gmTm.tm_hour,
        gmTm.tm_min,
        gmTm.tm_sec);
    return szBuffer;

}


static int hexN(char c)
{
    if (c >= '0' && c <= '9') return c-'0';
    if (c >= 'a' && c <= 'f') return 10+c-'a';
    if (c >= 'A' && c <= 'F') return 10+c-'A';
    throw std::invalid_argument("Malformed URL.");
}
static char hexChar(char c1, char c2)
{
    return (char)((hexN(c1) << 4) + hexN(c2));
}

class SafeCharacterMap {
    bool safeCharacter[256];
public:
    SafeCharacterMap() 
    {
        for (int i = 0; i < 0x20; ++i) {
            safeCharacter[i] = false;
        }
        for (int i = 0x20; i < 0x80; ++i) {
            safeCharacter[i] = true;
        }
        for (int i = 0x80; i < 256; ++i)
        {
            safeCharacter[i] = false;
        }
        const char *unsafeCharacters = ":/+?=#%@ ";
        for (const char*p = unsafeCharacters; *p != 0; ++p)
        {
            safeCharacter[(uint8_t)*p] = false;
        }
    }


    bool isSafeCharacter(char c) { return safeCharacter[(uint8_t)c];}
} g_safeCharacterMap;


std::string HtmlHelper::encode_url_segment(const char*pStart, const char*pEnd, bool isQuerySegment )
{
    std::stringstream s;
    encode_url_segment(s,pStart,pEnd,isQuerySegment);
    return s.str();
}

static char hexChars[] = "0123456789ABCDEF";

void HtmlHelper::encode_url_segment(std::ostream&os, const char*pStart, const char *pEnd, bool isQuerySegment)
{
    while (pStart != pEnd)
    {
        char c = *pStart++;
        if (g_safeCharacterMap.isSafeCharacter(c))
        {
            os << c;
        } else if (c == ' '  && isQuerySegment)
        {
            os << '+';

            os << c;
        }  else {
            os << '%' << hexChars[(c >> 4) & 0x0f] << hexChars[c & 0x0F];
        }
    }
}




std::string HtmlHelper::decode_url_segment(const char*pStart, const char*pEnd, bool isQuery)
{
    std::stringstream s;
    auto p = pStart;
    while (p != pEnd)
    {
        char c = *p;
        ++p;
        if (c == '+')
        {
            s << ' ';
        } else if (c != '%')
        {
            s << c;
        } else {
            if (pStart+2 < pEnd)
            {
                char c1 = *p; ++p;
                char c2 = *p; ++p;
                s << hexChar(c1,c2);
            } else {
                throw std::invalid_argument("Malformed URL.");
            }
        }
    }
    return s.str();
}

std::string HtmlHelper::decode_url_segment(const char*text, bool isQuery)
{
    return decode_url_segment(text, text + strlen(text),isQuery);
}

void HtmlHelper::utf32_to_utf8_stream(std::ostream &s, uint32_t uc)
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

const std::string ESPECIALS = "()<>@,;:\"/[]?.=";

#define MAX_FILENAME_LENGTH 96
#define MAX_SEGMENT_LENGTH 74 // 75 per RFC2047.2. But leave space for a ';' to allow easier breaking of lines.

std::string HtmlHelper::Rfc5987EncodeFileName(const std::string&name)
{
    // Encode all characters. Let the browser handle safe-ifying the actual name, since that's os dependent.
    std::stringstream s;
    std::string prefix = "UTF-8''";
    std::string postfix = "";
    s << prefix;
    for (char c: name)
    {
        uint8_t uc = (uint8_t)c;
        if (uc < 0x20 || uc >= 0x80 || ESPECIALS.find(c) != std::string::npos)
        {
            s << '%' << hexChars[uc >> 4] << hexChars[uc & 0x0F];
        } else {
            s << c;
        }
    }
    s << postfix;
    return s.str();
}


#define MAX_FILE_NAME_LENGHT 96

const std::string SF_SPECIALS = " <>@;:\"\'/[]?=";

std::string HtmlHelper::SafeFileName(const std::string &name)
{
    std::stringstream s;
    for (char c : name)
    {
        uint8_t uc = (uint8_t)c;
        if (uc < 0x20 || uc >= 0x80 || SF_SPECIALS.find(c) != std::string::npos)
        {
            s << '_';
        }
        else
        {
            s << c;
        }
    }
    return s.str();
}

std::string HtmlHelper::HtmlEncode(const std::string& text)
{
    std::stringstream os;
    for (char c: text)
    {
        switch(c)
        {
            case '<': os << "&lt;"; break;
            case '>': os << "&gt;"; break;
            case '&': os << "&amp;"; break;
            default: os << c; break;

        }
    }
    return os.str();
}




