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

#pragma once
#include <vector>
#include <variant>
#include <map>

#include <string>
#include <stdexcept>
#include <utility>
#include "json.hpp"

namespace pipedal
{
    class json_null
    {
    public:
        static json_null instance;
        bool operator==(const json_null &other) const { return true; }
        bool operator!=(const json_null &other) const { return (!((*this) == other)); }

    private:
        int value = 0;
    };

    class json_object;
    class json_array;

    class json_variant
        : public JsonSerializable
    {
    public:
        enum class ContentType
        {
            Null,
            Bool,
            Number,
            String,
            Object,
            Array
        };
        using object_ptr = std::shared_ptr<json_object>;
        using array_ptr = std::shared_ptr<json_array>;

    private:
    public:
        ~json_variant();
        json_variant();
        json_variant(json_reader &reader);
        json_variant(json_variant &&);
        json_variant(const json_variant &);

        json_variant(json_null value);
        json_variant(bool value);
        json_variant(double value);
        json_variant(const std::string &value);
        json_variant(std::shared_ptr<json_object> &&value);
        json_variant(const std::shared_ptr<json_object> &value);
        json_variant(std::shared_ptr<json_array> &&value);
        json_variant(const std::shared_ptr<json_array> &value);
        json_variant(json_array &&array);
        json_variant(json_object &&object);
        json_variant(const char *sz);

        json_variant(const void *) = delete; // do NOT allow implicit conversion of pointers to bool

        json_variant &operator=(json_variant &&value);
        json_variant &operator=(const json_variant &value);
        json_variant &operator=(bool value);
        json_variant &operator=(double value);
        json_variant &operator=(const std::string &value);
        json_variant &operator=(std::string &&value);
        json_variant &operator=(json_object &&value);
        json_variant &operator=(json_array &&value);

        json_variant &operator=(const char *sz) { return (*this) = std::string(sz); }

        json_variant &operator=(void *) = delete; // do NOT allow implicit conversion of pointers to bool

        void require_type(ContentType content_type) const
        {
            if (this->content_type != content_type)
            {
                throw std::logic_error("Content type is not valid.");
            }
        }
        bool is_null() const { return content_type == ContentType::Null; }
        bool is_bool() const { return content_type == ContentType::Bool; }
        bool is_number() const { return content_type == ContentType::Number; }
        bool is_string() const { return content_type == ContentType::String; }
        bool is_object() const { return content_type == ContentType::Object; }
        bool is_array() const { return content_type == ContentType::Array; }

        const json_null &as_null() const
        {
            require_type(ContentType::Bool);
            return json_null::instance;
        }
        json_null &as_null()
        {
            require_type(ContentType::Null);
            return json_null::instance;
        }

        bool as_bool() const
        {
            require_type(ContentType::Bool);
            return content.bool_value;
        }
        bool &as_bool()
        {
            require_type(ContentType::Bool);
            return content.bool_value;
        }

        double as_number() const
        {
            require_type(ContentType::Number);
            return content.double_value;
        }
        double &as_number()
        {
            require_type(ContentType::Number);
            return content.double_value;
        }

        int8_t as_int8() const { return (int8_t)as_number(); }

        uint8_t as_uint8() const { return (uint8_t)as_number(); }

        int16_t as_int16() const { return (int16_t)as_number(); }

        uint16_t as_uint16() const { return (uint16_t)as_number(); }

        int32_t as_int32() const { return (int32_t)as_number(); }

        uint32_t as_uint32() const { return (uint32_t)as_number(); }

        int64_t as_int64() const { return (int64_t)as_number(); }

        uint64_t as_uint64() const { return (uint64_t)as_number(); }

        const std::string &as_string() const;
        std::string &as_string();

        const std::shared_ptr<json_object> &as_object() const;
        std::shared_ptr<json_object> &as_object();

        const std::shared_ptr<json_array> &as_array() const;
        std::shared_ptr<json_array> &as_array();

        // convenience methods for object and array manipulation.
        static json_variant make_object();
        static json_variant make_array();

        void resize(size_t size);
        size_t size() const;

        bool contains(const std::string &index) const;

        json_variant &at(size_t index);
        const json_variant &at(size_t index) const;

        json_variant &operator[](size_t index);
        const json_variant &operator[](size_t index) const;

        json_variant &operator[](const std::string &index);
        const json_variant &operator[](const std::string &index) const;

        bool operator==(const json_variant &other) const;
        bool operator!=(const json_variant &other) const;

        std::string to_string() const;

    private:
        void free();
        void write_double_value(json_writer &writer, double value) const;
        void write_float_value(json_writer &writer, double value) const;
        virtual void read_json(json_reader &reader);
        virtual void write_json(json_writer &writer) const;

        static constexpr size_t stringSize = sizeof(std::string);
        static constexpr size_t objectSize = sizeof(std::shared_ptr<json_variant>);
        static constexpr size_t memSize = stringSize > objectSize ? stringSize : objectSize;
        union Content
        {
            bool bool_value;
            double double_value;
            float float_value;
            int32_t int32_value;
            uint8_t mem[memSize];
        };

        ContentType content_type = ContentType::Null;
        Content content;

        std::string &memString();
        object_ptr &memObject();
        array_ptr &memArray();

        const std::string &memString() const;
        const object_ptr &memObject() const;
        const array_ptr &memArray() const;
    };
    class json_array : public JsonSerializable
    {
    private:
        json_array(const json_array &) {} // deleted.
    public:
        using ptr = std::shared_ptr<json_array>;

        json_array() { ++allocation_count_; }
        json_array(json_array &&other);
        ~json_array() { --allocation_count_; }

        json_variant &at(size_t index);
        const json_variant &at(size_t index) const;

        json_variant &operator[](size_t index);
        const json_variant &operator[](size_t &index) const;

        void resize(size_t size) { values.resize(size); }
        size_t size() const { return values.size(); }
        void push_back(json_variant &&value) { values.push_back(std::move(value)); }
        template <typename U>
        void push_back(U &&value) { values.push_back(value); }
        void push_back(double value) { values.push_back(json_variant{value}); }
        void push_back(const std::string &value) { values.push_back(json_variant{value}); }
        void push_back(bool value) { values.push_back(json_variant{value}); }
        void push_back(const std::shared_ptr<json_array> &value) { values.push_back(json_variant(value)); }
        void push_back(const std::shared_ptr<json_object> &value) { values.push_back(json_variant(value)); }

        bool operator==(const json_array &other) const;
        bool operator!=(const json_array &other) const { return (!((*this) == other)); }

        // Strictly for testing purposes. Not thread-safe.
        static int64_t allocation_count()
        {
            return allocation_count_;
        }
        using iterator = std::vector<json_variant>::iterator;
        using const_iterator = std::vector<json_variant>::const_iterator;

        iterator begin() { return values.begin(); }
        iterator end() { return values.end(); }
        const_iterator begin() const { return values.begin(); }
        const_iterator end() const { return values.end(); }

    private:
        static int64_t allocation_count_; // strictly for testing purposes. not thread safe.

        virtual void read_json(json_reader &reader);
        virtual void write_json(json_writer &writer) const;

        void check_index(size_t size) const;

        std::vector<json_variant> values;
    };
    class json_object : public JsonSerializable
    {
    private:
        json_object(const json_object &) {} // deleted.
    public:
        using ptr = std::shared_ptr<json_object>;

        json_object() { ++allocation_count_; }
        json_object(json_object &&other);
        ~json_object() { --allocation_count_; }

        size_t size() const { return values.size(); }
        json_variant &at(const std::string &index);
        const json_variant &at(const std::string &index) const;

        json_variant &operator[](const std::string &index);
        const json_variant &operator[](const std::string &index) const;

        bool operator==(const json_object &other) const;
        bool operator!=(const json_object &other) const { return (!((*this) == other)); }
        bool contains(const std::string &index) const;

        using values_t = std::vector<std::pair<std::string, json_variant>>;
        using iterator = values_t::iterator;
        using const_iterator = values_t::const_iterator;

        iterator begin() { return values.begin(); }
        iterator end() { return values.end(); }
        const_iterator begin() const { return values.begin(); }
        const_iterator end() const { return values.end(); }

        iterator find(const std::string &key);
        const_iterator find(const std::string &key) const;

        // strictly for testing purposes. Not thread-safe.
        static int64_t allocation_count()
        {
            return allocation_count_;
        }

    private:
        virtual void read_json(json_reader &reader);
        virtual void write_json(json_writer &writer) const;

        static int64_t allocation_count_;
        values_t values;
    };

    ////////////////////////////////////////////////

    inline std::string &json_variant::memString() { return *(std::string *)content.mem; }
    inline const std::string &json_variant::memString() const { return *(const std::string *)content.mem; }

    inline json_variant::object_ptr &json_variant::memObject()
    {
        return *(object_ptr *)content.mem;
    }
    inline json_variant::array_ptr &json_variant::memArray()
    {
        return *(array_ptr *)content.mem;
    }

    inline const json_variant::object_ptr &json_variant::memObject() const
    {
        return *(const object_ptr *)content.mem;
    }
    inline const json_variant::array_ptr &json_variant::memArray() const
    {
        return *(const array_ptr *)content.mem;
    }

    inline json_variant::json_variant(json_array &&array)
    {
        this->content_type = ContentType::Null;
        new (content.mem) std::shared_ptr<json_array>{new json_array(std::move(array))};
        this->content_type = ContentType::Array;
    }
    inline json_variant::json_variant(json_object &&object)
    {
        this->content_type = ContentType::Null;
        new (content.mem) std::shared_ptr<json_object>{new json_object(std::move(object))};
        this->content_type = ContentType::Object;
    }

    inline json_variant::json_variant(const std::string &value)
    {
        this->content_type = ContentType::Null;
        new (content.mem) std::string(value); // placement new.
        content_type = ContentType::String;
    }

    inline json_variant::json_variant(std::shared_ptr<json_object> &&value)
    {
        this->content_type = ContentType::Null;
        new (content.mem) std::shared_ptr<json_object>(std::move(value)); // placement new.
        content_type = ContentType::Object;
    }
    inline json_variant::json_variant(const std::shared_ptr<json_object> &value)
    {
        // don't deep copy!
        std::shared_ptr<json_object> t = const_cast<std::shared_ptr<json_object> &>(value);
        this->content_type = ContentType::Null;
        new (content.mem) std::shared_ptr<json_object>(t); // placement new.
        content_type = ContentType::Object;
    }

    inline json_variant::json_variant(const std::shared_ptr<json_array> &value)
    {
        // Make sure we don't deep copy!
        std::shared_ptr<json_array> t = const_cast<std::shared_ptr<json_array> &>(value);
        this->content_type = ContentType::Null;
        new (content.mem) std::shared_ptr<json_array>(t); // placement new.
        content_type = ContentType::Array;
    }
    inline json_variant::json_variant(array_ptr &&value)
    {
        this->content_type = ContentType::Null;
        new (content.mem) std::shared_ptr<json_array>(std::move(value)); // placement new.
        content_type = ContentType::Array;
    }
    inline json_variant::json_variant()
    {
        content_type = ContentType::Null;
    }
    inline json_variant::json_variant(json_null value)
    {
        content_type = ContentType::Null;
    }
    inline json_variant::json_variant(bool value)
    {
        content_type = ContentType::Bool;
        content.bool_value = value;
    }

    inline json_variant::json_variant(double value)
    {
        content_type = ContentType::Number;
        content.double_value = value;
    }
    inline std::string &json_variant::as_string()
    {
        require_type(ContentType::String);
        return memString();
    }

    inline const std::string &json_variant::as_string() const
    {
        require_type(ContentType::String);
        return memString();
    }

    inline const json_variant::object_ptr &json_variant::as_object() const
    {
        require_type(ContentType::Object);
        return memObject();
    }

    inline json_variant::object_ptr &json_variant::as_object()
    {
        require_type(ContentType::Object);
        return memObject();
    }

    inline const json_variant::array_ptr &json_variant::as_array() const
    {
        require_type(ContentType::Array);
        return memArray();
    }

    inline json_variant::array_ptr &json_variant::as_array()
    {
        require_type(ContentType::Array);
        return memArray();
    }

    inline /*static*/ json_variant json_variant::make_object()
    {
        return json_variant{std::make_shared<json_object>()};
    };
    inline /*static */ json_variant json_variant::make_array()
    {
        return json_variant{std::make_shared<json_array>()};
    };

    inline void json_variant::resize(size_t size)
    {
        as_array()->resize(size);
    }

    inline json_variant &json_variant::at(size_t index)
    {
        return as_array()->at(index);
    }
    inline const json_variant &json_variant::at(size_t index) const
    {
        return as_array()->at(index);
    }

    inline json_variant &json_variant::operator[](size_t index)
    {
        return as_array()->at(index);
    }
    inline const json_variant &json_variant::operator[](size_t index) const
    {
        return as_array()->at(index);
    }

    inline const json_variant &json_variant::operator[](const std::string &index) const
    {
        return (*as_object())[index];
    }
    inline json_variant &json_variant::operator[](const std::string &index)
    {
        return (*as_object())[index];
    }

    inline const json_variant &json_array::operator[](size_t &index) const
    {
        check_index(index);
        return values[index];
    }

    inline json_variant &json_array::operator[](size_t index)
    {
        return at(index);
    }

    inline void json_array::check_index(size_t size) const
    {
        if (size >= values.size())
        {
            throw std::out_of_range("index out of range.");
        }
    }

    inline bool json_variant::operator!=(const json_variant &other) const
    {
        return !(*this == other);
    }

    // Holds a string but is json_read and json_written as an unqoted json object.
    class raw_json_string : public JsonSerializable
    {
    public:
        raw_json_string() {}
        raw_json_string(const std::string &value) : value(value) {}
        const std::string &as_string() const { return value; }

        void Set(const std::string &value) { this->value = value; }

    private:
        virtual void write_json(json_writer &writer) const
        {
            writer.write_raw(value.c_str());
        }
        virtual void read_json(json_reader &reader)
        {
            throw std::logic_error("Not implemented.");
        }

        std::string value;
    };

} // namespace pipedal