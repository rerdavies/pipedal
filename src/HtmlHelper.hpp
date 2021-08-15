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

    static std::string SafeFileName(const std::string name);

};

} // namespace.