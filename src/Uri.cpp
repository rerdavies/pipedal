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

#include "pch.h"
#include "Uri.hpp"
#include "HtmlHelper.hpp"
#include "PiPedalException.hpp"

using namespace pipedal;

std::string uri::segment(int index) const
{
    const char *p = path_start;
    int segment = 0;

    if (p != path_end && *p == '/') ++p;

    while (p != path_end && *p != '?' && *p != '#')
    {
        if (segment == index)
        {
            const char*segmentStart = p;
            while(p != path_end && *p != '/' && *p != '?' && *p != '#')
            {
                ++p;
            }
            return HtmlHelper::decode_url_segment(segmentStart,p);
        } else {
            while(p != path_end && *p != '/' && *p != '?' && *p != '#')
            {
                ++p;
            }
        }
        if (p != path_end && *p == '/') ++p;
        ++segment;
    }
    throw std::invalid_argument("Invalid segement number.");
};


int uri::segment_count() const { 
    const char *p = path_start;
    int segment = 0;

    if (p != path_end && *p == '/') ++p;

    while (p != path_end && *p != '?' && *p != '#')
    {
        while(p != path_end && *p != '/' && *p != '?' && *p != '#')
        {
            ++p;
        }
        if (p != path_end && *p == '/') ++p;
        ++segment;
    }
    return segment;
}

void uri::set(const char*start, const char*end)
{
    this->text = std::string(start,end);
    set_();
}
void uri::set(const char*text)
{
    this->text = text;
    set_();
}

void uri::set_()
{
    const char*start = this->text.c_str();
    const char*end = start+this->text.size();

    const char* p = start;

    this->isRelative = true;

    bool hasScheme = false;
    while (p != end)
    {
        char c = *p;
        if (c == ':') {
            hasScheme = true;
            break;
        }
        if (c == '/' || c == '?' || c == '#')
        {
            break;
        }
        ++p;
    }
    if (hasScheme)
    {
        scheme_start = start;
        scheme_end = p;
        ++p;
    } else {
        scheme_start = start;
        scheme_end = start;
        p = start;
    }
    if (p[0] == '/' && p[1] == '/') 
    {
        this->isRelative = false;
        // authority.
        p += 2;

        user_start = p;
        user_end = p;
        authority_start = p;
        authority_end = nullptr;

        const char*port_start = nullptr;
        while (p != end)
        {
            char c = *p;
            if (c == '@')
            {
                user_end = p;
                authority_start = p+1;
                port_start = nullptr;
            } else if (c == ':')
            {
                authority_end = p;
                port_start = p+1;
            }
            if (c == '/' || c == '?' || c == '#')
            {
                break;
            }
            ++p;
        }
        if (authority_end == nullptr) authority_end = p;
        if (port_start != nullptr)
        {
            char*intEnd;
            port_ = (int)std::strtol(port_start,&intEnd,10);
            if (intEnd != p) throwInvalid();
        } else {
            port_ = -1;
        }
    } else {
        user_start = p;
        user_end = p;
        authority_start = p;
        authority_end = p;
        port_ = -1;
    }

    path_start = p;
    if (p != end && *p == '/') this->isRelative = false;

    while (p != end && *p != '?' && *p != '#')
    {
        ++p;
    }
    path_end = p;

    if (p != end && *p == '?')
    {
        ++p;
        query_start = p;
        while (p != end && *p != '#')
        {
            ++p;
        }
        query_end = p;
    } else {
        query_start = query_end = p;
    }
    if (p != end && *p == '#')
    {
        ++p;
        fragment_start = p;
        fragment_end = end;

    } else {
        fragment_start = fragment_end = p;
    }
}

void uri::throwInvalid() {
    throw std::invalid_argument("Invalid uri.");

}

static bool compare_name(const char*start, const char*end, const char*szName)
{
    while (start != end)
    {
        if (*start != *szName) return false;
        ++start; ++szName;
    }
    return *szName == 0;
}

int  uri::query_count() const
{
    int count = 0;
    const char*p = query_start;
    while (p != query_end && *p != '#')
    {
        const char*nameStart = p;
        while (p != query_end && *p != '&' && *p != '#')
        {
            ++p;
        }
        if (p != query_end && *p == '&') ++p;
        ++count;
        if (p == query_end || *p == '#')
        {
            break;

        }
    }
    return count;

}

bool  uri::has_query(const char*name) const
{
    const char*p = query_start;
    while (p != query_end && *p != '#')
    {
        const char*nameStart = p;
        while (p != query_end && *p != '&' && *p != '=' && *p != '#')
        {
            ++p;
        }
        // compare names.
        if (compare_name(nameStart,p,name))
        {
            return true;
        }
        while (p != query_end && *p != '&' && *p != '#')
        {
            ++p;
        }
        if (p != query_end && *p == '&') ++p;
    }
    return false;

}

query_segment uri::query(int index) const
{
    const char*p = query_start;
    int ix = 0;
    while (p != query_end && *p != '#')
    {
        const char*nameStart = p;
        while (p != query_end && *p != '&' && *p != '=' && *p != '#')
        {
            ++p;
        }
        // compare names.

        //if (compare_name(nameStart,p,name))
        if (ix == index)
        {
            const char *nameEnd = p;
            if (p != query_end && *p == '=')
            {
                ++p;
                const char*value_start = p;
                while(p != query_end && *p != '&' && *p != '#')
                {
                    ++p;
                }
                return query_segment(
                    std::string(nameStart,nameEnd),
                    std::string(value_start,p)
                );
                
            } else {
                return query_segment(
                    std::string(nameStart,nameEnd),
                    ""
                );
            }
        }
        while (p != query_end && *p != '&' && *p != '#')
        {
            ++p;
        }
        if (p != query_end && *p == '&') 
        {
            ++p;
        }
        ++ix;

    }
    throw PiPedalArgumentException("Index out of range.");
}

std::string uri::query(const char*name) const
{
    const char*p = query_start;
    while (p != query_end && *p != '#')
    {
        const char*nameStart = p;
        while (p != query_end && *p != '&' && *p != '=' && *p != '#')
        {
            ++p;
        }
        // compare names.
        if (compare_name(nameStart,p,name))
        {
            if (p != query_end && *p == '=')
            {
                ++p;
                const char*value_start = p;
                while(p != query_end && *p != '&' && *p != '#')
                {
                    ++p;
                }
                return HtmlHelper::decode_url_segment(value_start,p);
            } else {
                return "";
            }
        }
        while (p != query_end && *p != '&' && *p != '#')
        {
            ++p;
        }
        if (p != query_end && *p == '&') 
        {
            ++p;
        }

    }
    return "";
}

query_segment uri::query(int index)
{
    const char*p = query_start;
    int count = 0;
    while (p != query_end && *p != '#')
    {
        if (count == index)
        {
            const char*nameStart = p;
            while (p != query_end && *p != '&' && *p != '=' && *p != '#')
            {
                ++p;
            }
            const char *nameEnd = p;
            const char *valueStart, *valueEnd;

            if(p == query_end || *p != '=')
            {
                valueStart = valueEnd = p;
            } else {
                ++p;
                valueStart = p;

                while(p != query_end && *p != '&' && *p != '#')
                {
                    ++p;
                }
                valueEnd = p;
            }
            return query_segment(std::string(nameStart,nameEnd), HtmlHelper::decode_url_segment(valueStart,valueEnd));
        } else {
            while (p != query_end && *p != '&' && *p != '#')
            {
                ++p;
            }

        }
        if (p != query_end && *p == '&') ++p;
        ++count;
    }
    throw std::invalid_argument("Argument out of range.");
}

std::vector<std::string> uri::segments()
{
    std::vector<std::string> result;
    for (int i = 0; i < segment_count(); ++i)
    {
        result.push_back(segment(i));
    }
    return result;
}

std::string uri::get_extension() const
{
    const char*p = this->path_end;
    while (p != this->path_start)
    {
         char c = p[-1];
         if (c == '/') return "";
         if (c == '.') {
             --p;
             return std::string(p,this->path_end-p);
         }
         --p;
    }
    return "";
}

std::string uri::to_canonical_form() const {
    uri_builder builder(*this);
    return builder.str();
}

std::string uri_builder::str() const {
    std::stringstream s;
    if (scheme_.length() != 0)
    {
        s << scheme_ << ':';
    }
    if (authority_.length() != 0) {
        s << "//";
        if (user_.length() != 0) 
        {
            s << user_ << '@';
        }
        s << authority_;
        if (port_ != -1) {
            s << ':' << (uint16_t)port_;
        }
    }
    if ( authority_.length() != 0 || !isRelative_) { // canonical root form is http://xyz/ not httP://xyz
        
        s << '/';
    }
    for (size_t i = 0; i < segments_.size(); ++i)
    {
        if (i != 0) s << '/';
        HtmlHelper::encode_url_segment(s,segments_[i],false);
    }
    if (queries_.size() != 0)
    {
        s << '?';
        for (size_t i = 0; i < queries_.size(); ++i)
        {
            s << queries_[i].key << "=";
            HtmlHelper::encode_url_segment(s,queries_[i].value,true);
        }

    }
    if (fragment_.length() != 0)
    {
        s << "#" << fragment_;
    }
    return s.str();

}