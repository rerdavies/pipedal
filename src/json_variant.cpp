/*
 * MIT License
 *
 * Copyright (c) 2023 Robin E. R. Davies
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

#include "json_variant.hpp"
#include <limits>
#include <cmath>
#include <cstddef>
#include <memory>

using namespace pipedal;

json_variant::~json_variant()
{
    free();
}

void json_variant::free()
{
    // in-place deletion.
    switch (content_type)
    {
    case ContentType::String:
        memString().std::string::~string();
        break;
    case ContentType::Object:
        memObject().std::shared_ptr<json_object>::~shared_ptr();
        break;
    case ContentType::Array:
        memArray().std::shared_ptr<json_array>::~shared_ptr();
        break;
    }
    content_type = ContentType::Null;
}


json_variant::json_variant(json_variant &&other)
{
    this->content_type = ContentType::Null;
    switch (other.content_type)
    {
    case ContentType::Null:
        break;
    case ContentType::Bool:
        this->content.bool_value = other.content.bool_value;
        break;
    case ContentType::Number:
        this->content.double_value = other.content.double_value;
        break;
    case ContentType::String:
        new (content.mem) std::string(std::move(other.memString()));
        this->content_type = ContentType::String;
        return;
    case ContentType::Object:
        new (content.mem) std::shared_ptr<json_object>{std::move(other.memObject())};
        this->content_type = ContentType::Object;
        return;
    case ContentType::Array:
        new (content.mem) std::shared_ptr<json_array>{std::move(other.memArray())};
        this->content_type = ContentType::Array;
        return;
    }
    this->content_type = other.content_type;
    other.content_type = ContentType::Null;
}

json_variant &json_variant::operator=(const json_variant &other)
{
    free();
    switch (other.content_type)
    {
    case ContentType::Null:
        break;
    case ContentType::Bool:
        this->content.bool_value = other.content.bool_value;
        break;
    case ContentType::Number:
        this->content.double_value = other.content.double_value;
        break;
    case ContentType::String:
        new (content.mem) std::string(other.memString());
        break;
    case ContentType::Object:
        new (content.mem) std::shared_ptr<json_object>{other.memObject()};
        break;
    case ContentType::Array:
        new (content.mem) std::shared_ptr<json_array>{other.memArray()};
        this->content_type = ContentType::Array;
        break;
    }
    this->content_type = other.content_type;
    return *this;

}

json_variant::json_variant(const json_variant &other)
{
    this->content_type = ContentType::Null;
    switch (other.content_type)
    {
    case ContentType::Null:
        break;
    case ContentType::Bool:
        this->content.bool_value = other.content.bool_value;
        break;
    case ContentType::Number:
        this->content.double_value = other.content.double_value;
        break;
    case ContentType::String:
        new (content.mem) std::string(other.memString());
        this->content_type = ContentType::String;
        return;
    case ContentType::Object:
        new (content.mem) std::shared_ptr<json_object>{other.memObject()};
        this->content_type = ContentType::Object;
        return;
    case ContentType::Array:
        new (content.mem) std::shared_ptr<json_array>{other.memArray()};
        this->content_type = ContentType::Array;
        return;
    }
    this->content_type = other.content_type;
}

void json_variant::write_float_value(json_writer &writer, double value) const
{
    writer.write(value);
}

void json_variant::write_double_value(json_writer &writer, double value) const
{
    writer.write(value);
}

void json_variant::read_json(json_reader &reader)
{
    int v = reader.peek();
    if (v == '[')
    {
        json_array array;
        reader.read(&array);
        (*this) = std::move(array);
    }
    else if (v == '{')
    {
        json_object object;
        reader.read(&object);
        (*this) = std::move(object);
    }
    else if (v == '\"')
    {
        std::string s;
        reader.read(&s);
        (*this) = std::move(s);
    }
    else if (v == 'n')
    {
        reader.read_null();
        (*this) = json_null();
    }
    else if (v == 't' || v == 'f')
    {
        bool b;
        reader.read(&b);
        (*this) = b;
    }
    else
    {
        // it's a number.
        double v;
        reader.read(&v);
        (*this) = v;
    }
}
void json_variant::write_json(json_writer &writer) const
{
    switch (content_type)
    {
    case ContentType::Null:
        writer.write_raw("null");
        break;
    case ContentType::Bool:
        writer.write(as_bool());
        break;
    case ContentType::Number:
        write_double_value(writer, as_number());
        break;
    case ContentType::String:
        writer.write(as_string());
        break;
    case ContentType::Object:
        writer.write(as_object());
        break;
    case ContentType::Array:
        writer.write(as_array());
        break;
    default:
        throw std::logic_error("Invalid variant type");
    }
}

bool json_array::operator==(const json_array &other) const
{
    if (!(this->size() == other.size()))
        return false;
    for (size_t i = 0; i < this->size(); ++i)
    {
        if (!((*this)[i] == other[i]))
            return false;
    }
    return true;
}

void json_array::read_json(json_reader &reader)
{
    reader.read(&(this->values));
}

void json_array::write_json(json_writer &writer) const
{
    writer.start_array();
    bool first = true;
    for (auto &value : values)
    {
        if (!first)
            writer.write_raw(",");
        first = false;
        writer.write(value);
    }
    writer.end_array();
}


json_object::iterator json_object::find(const std::string& key)
{
    for (auto i = begin(); i != end(); ++i)
    {
        if (i->first == key)
        {
            return i;
        }
    }
    return end();
}
json_object::const_iterator json_object::find(const std::string&key) const
{
    for (auto i = begin(); i != end(); ++i)
    {
        if (i->first == key)
        {
            return i;
        }
    }
    return end();

}



bool json_object::operator==(const json_object &other) const
{
    
    for (const auto &pair : this->values)
    {
        auto index = other.find(pair.first);
        if (index == other.end())
            return false;
        if (!(index->second == pair.second))
            return false;
    }
    for (const auto &pair : other.values)
    {
        auto index = this->find(pair.first);
        if (index == this->end())
            return false;
        if (!(index->second == pair.second))
            return false;
    }
    return true;
}



void json_object::read_json(json_reader &reader)
{
    reader.start_object();

    while (true)
    {
        if (reader.peek() == '}') break;

        std::string key;
        json_variant value;
        reader.read(&key);
        reader.consume(':');
        reader.read(&value);

        (*this)[key] = value;
        if (reader.peek() == ',') 
        {
            reader.consume(',');
        }
    }

    reader.end_object();
}
void json_object::write_json(json_writer &writer) const
{
    writer.start_object();
    bool first = true;
    for (auto &value : values)
    {
        if (!first)
        {
            writer.write_raw(",");
        }
        first = false;
        writer.write(value.first);
        writer.write_raw(": ");
        writer.writeRawWritable(value.second);
    }
    writer.end_object();
}

json_variant &json_array::at(size_t index)
{
    check_index(index);
    return values.at(index);
}
const json_variant &json_array::at(size_t index) const
{
    check_index(index);
    return values.at(index);
}

size_t json_variant::size() const
{
    if (content_type == ContentType::Array)
        return as_array()->size();
    if (content_type == ContentType::Object)
    {
        return as_object()->size();
    }
    throw std::logic_error("Not supported.");
}

json_variant &json_object::at(const std::string &index) { 
    for (auto&entry: values)
    {
        if (entry.first == index)
        {
            return entry.second;
        }
    }
    throw std::logic_error("Not found.");
    values.push_back(std::pair(index,json_variant()));
    return values[values.size()-1].second;
}
const json_variant &json_object::at(const std::string &index) const { 
    for (const auto&entry: values)
    {
        if (entry.first == index)
        {
            return entry.second;
        }
    }
    throw std::logic_error("Not found.");
}

json_variant &json_object::operator[](const std::string &index)
{
    for (auto&entry: values)
    {
        if (entry.first == index)
        {
            return entry.second;
        }
    }
    values.push_back(std::pair(index,json_variant()));
    return values[values.size()-1].second;
}
const json_variant &json_object::operator[](const std::string &index) const
{
    return at(index);
}
bool json_object::contains(const std::string &index) const
{
    return find(index) != end();
}

json_variant &json_variant::operator=(bool value)
{
    free();
    this->content_type = ContentType::Bool;
    this->content.bool_value = value;
    return *this;
}

json_variant &json_variant::operator=(double value)
{
    free();
    this->content_type = ContentType::Number;
    this->content.double_value = value;
    return *this;
}
json_variant &json_variant::operator=(const std::string &value)
{
    free();
    this->content_type = ContentType::String;
    new (this->content.mem) std::string(value); // in-place constructor.
    return *this;
}
json_variant &json_variant::operator=(std::string &&value)
{
    free();
    this->content_type = ContentType::String;
    new (this->content.mem) std::string(std::move(value)); // in-place constructor.
    return *this;
}
json_variant &json_variant::operator=(json_object &&value)
{
    free();
    new (this->content.mem) std::shared_ptr<json_object>{new json_object(std::move(value))};
    this->content_type = ContentType::Object;
    return *this;
}
json_variant &json_variant::operator=(json_array &&value)
{
    free();
    this->content_type = ContentType::Array;
    new (this->content.mem) std::shared_ptr<json_array>{new json_array(std::move(value))};
    return *this;
}
json_variant &json_variant::operator=(json_variant &&value)
{
    if (this->content_type == value.content_type)
    {
        switch (this->content_type)
        {
        case ContentType::String:
            std::swap(this->memString(), value.memString());
            return *this;
        case ContentType::Array:
            std::swap(this->memArray(), value.memArray());
            return *this;
        case ContentType::Object:
            std::swap(this->memObject(), value.memObject());
            return *this;
        }
    }
    free();
    switch (value.content_type)
    {
    case ContentType::String:
        new (content.mem) std::string(std::move(value.memString()));
        value.memString().std::string::~string();
        break;
    case ContentType::Object:
        new (content.mem) std::shared_ptr<json_object>(std::move(value.memObject()));
        value.memObject().std::shared_ptr<json_object>::~shared_ptr();
        break;
    case ContentType::Array:
        new (content.mem) std::shared_ptr<json_array>(std::move(value.memArray()));
        value.memArray().std::shared_ptr<json_array>::~shared_ptr();
        break;
    default:
        // undifferentiated copy of POD types.
        *(uint64_t *)(this->content.mem) = *(uint64_t *)(value.content.mem);
        break;
    }
    this->content_type = value.content_type;
    value.content_type = ContentType::Null;
    return *this;
}

bool json_variant::operator==(const json_variant &other) const
{
    if (this->content_type != other.content_type)
        return false;

    switch (this->content_type)
    {
    case ContentType::Null:
        return true;
    case ContentType::Bool:
        return as_bool() == other.as_bool();
        break;
    case ContentType::Number:
        return as_number() == other.as_number();
    case ContentType::String:
        return as_string() == other.as_string();
    case ContentType::Array:
        return *(as_array().get()) == *(other.as_array().get());
    case ContentType::Object:
        return *(as_object().get()) == *(other.as_object().get());;
    default:
        throw std::logic_error("Invalid content_type.");
    }
}
json_array::json_array(json_array&&other)
: values(std::move(other.values))
{
    ++allocation_count_;
}
json_object::json_object(json_object&&other)
:values(std::move(other.values))
{
    ++allocation_count_;
}

bool json_variant::contains(const std::string &index) const
{
    if (is_object())
    {
        return as_object()->contains(index);
    }
    return false;
}

std::string json_variant::to_string() const
{
    std::stringstream ss;
    json_writer writer(ss);
    writer.write(*this);
    return ss.str();
}

json_variant::json_variant(const char*sz)
: json_variant(std::string(sz))
{
    
}



/*static*/ json_null json_null::instance;
/*static*/ int64_t json_array::allocation_count_ = 0; // strictly for testing purposes. not thread safe.
/*static*/ int64_t json_object::allocation_count_ = 0; // strictly for testing purposes. not thread safe.

