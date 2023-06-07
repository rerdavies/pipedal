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
#include <string>
#include <vector>
#include <string_view>
#include <stdexcept>
#include <sstream>
#include <cstdlib>

namespace pipedal {


class query_segment {
    
public:
    query_segment(const std::string &key, const std::string &value)
    : key(key)
    , value(value) {

    }
    const std::string key;
    const std::string value;

    bool operator==(const query_segment &other)
    {
        return key == other.key && value == other.value;
    }
};


class uri
{
private:
    std::string text;
    const char*scheme_start, *scheme_end;
    const char *user_start, *user_end;
    const char* authority_start, *authority_end;
    const char* path_start, *path_end;
    bool isRelative;
    int port_;
    const char*query_start, *query_end;
    const char*fragment_start, *fragment_end;
public:
    uri()
    {
        set("");
    }
    const std::string& str() {
        return text;
    }
    uri(const char*text)
    {
        set(text);
    }
    uri(const std::string&text)
    {
        set(text.c_str());
    }
    void set(const char*text);
    void set(const char*start, const char*end);
private:
     void set_();
    static void throwInvalid();
public:
    bool has_scheme() const { return scheme_start != scheme_end; }
    std::string scheme() const{ return std::string(scheme_start, scheme_end); }

    bool has_user() const{ return user_start != user_end; }
    std::string user() const{ return std::string(user_start,user_end); }

    bool has_authority() const{ return authority_start != authority_end; }
    std::string authority() const{ return std::string(authority_start,authority_end);}

    bool has_port() const{ return port_ != -1; }
    int port() const{ return port_; }

    bool is_relative() const { return isRelative; }

    std::string path()  const { return std::string(path_start, path_end); }

    int segment_count() const;

    std::string segment(int n) const;

    std::vector<std::string> segments();
    const std::vector<std::string> segments() const;

    int query_count() const;

    query_segment query(int index);

    bool  has_query(const char*name) const;

    std::string query(const char*name) const;
    query_segment query(int index) const;

    std::string get_extension() const;

    std::string fragment() const { return std::string(fragment_start, fragment_end);}

    std::string to_canonical_form() const;

};

class uri_builder {

private:
    std::string scheme_ = "http";
    std::string user_ = "";
    std::string authority_ = "";
    int port_ = -1;
    bool isRelative_ = false;
    std::vector<std::string> segments_;

    std::vector<query_segment> queries_;
    std::string fragment_;

public: 
    uri_builder()
    {

    }

    uri_builder(const uri&uri)
    :   scheme_(uri.scheme()),
        user_(uri.user()),
        authority_(uri.authority()),
        port_(uri.port()),
        isRelative_(uri.is_relative()),
        fragment_(uri.fragment())
    {
        for (int i = 0; i < uri.segment_count(); ++i)
        {
            segments_.push_back(uri.segment(i));
        }
        for (int i = 0; i < uri.query_count(); ++i)
        {
            queries_.push_back(uri.query(i));
        }
    }
    std::string str() const;

    const std::string & scheme() const { return scheme_; }
    void set_scheme(const std::string &scheme_) {
        this->scheme_ = scheme_;
    }
    const std::string & user() const { return this->user_;}
    void set_user(const std::string&user) { this->user_ = user; }

    const std::string &authority() const {
        return this->authority_;
    }
    void set_authority(const std::string &authority) {
        this->authority_ = authority;
    }
    int port() const { return port_; }
    void set_port(int port) {
        this->port_ = port;
    }

    bool is_relative() const { return this->isRelative_ && authority_.length() == 0; }
    void set_is_relative(bool isRelative)
    {
        this->isRelative_ = isRelative;
    }
    int segment_count() const { return (int)segments_.size(); }
    const std::string& segment(int i) const { return segments_[i];}

    void append_segment(const std::string & segment) {
        segments_.push_back(segment);
    }
    void insert_segment(int position, std::string & segment) {
        segments_.insert(segments_.begin()+position,segment);
    }
    void erase_segment(int position) 
    {
        segments_.erase(segments_.begin()+position); 
    }
    void replace_segment(int position, std::string& segment) {
        segments_[position] = segment;
    }


    int query_count() const { return (int)(queries_.size());}

    bool has_query(const std::string &key) const {
        for (size_t i = 0; i < queries_.size(); ++i)
        {
            if (queries_[i].key == key) true;
        }
        return false;

    }
    std::string query(const std::string &key) const {
        for (size_t i = 0; i < queries_.size(); ++i)
        {
            if (queries_[i].key == key) return queries_[i].value;
        }
        return "";
    }
    std::vector<std::string> queries(const std::string &key) const {
        std::vector<std::string> result;
        for (size_t i = 0; i < queries_.size(); ++i)
        {
            if (queries_[i].key == key) result.push_back(queries_[i].value);
        }
        return result;
    }
    const query_segment & query(int index) {
        return queries_[index];
    }

    const std::string&fragment() { return this->fragment_; }
    void set_fragment(const std::string & fragment) { this->fragment_ = fragment;}


};
}; // namespace pipedal.

