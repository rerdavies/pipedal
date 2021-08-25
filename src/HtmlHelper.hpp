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

#include <string>

namespace pipedal {

class HtmlHelper {

public:
    static std::string timeToHttpDate();
    static std::string timeToHttpDate(time_t time);


    static std::string encode_url_segment(const char*pStart, const char*pEnd, bool isQuerySegment = false);
    static void encode_url_segment(std::ostream&os, const char*pStart, const char *pEnd, bool isQuerySegment = false);
    static void encode_url_segment(std::ostream&os, const std::string&segment, bool isQuerySegment = false)
    {
        const char*p = segment.c_str();
        encode_url_segment(os,p, p + segment.length(),isQuerySegment);
    }


    static std::string decode_url_segment(const char*pStart, const char*pEnd, bool isQuerySegment = false);

    static std::string decode_url_segment(const char*text, bool isQuerySegment = false);

    static void utf32_to_utf8_stream(std::ostream &s, uint32_t uc);

    static std::string Rfc5987EncodeFileName(const std::string&name);

    static std::string SafeFileName(const std::string &name);
    static std::string HtmlEncode(const std::string& text);

};

} // namespace.