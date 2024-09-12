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
#include <iostream>
#include <cstdint>
#include <boost/utility/string_view.hpp>
#include <boost/format.hpp>
#include <iomanip>
#include <type_traits>
#include <sstream>
#include "HtmlHelper.hpp"
#include <cctype>
#include <cmath>
#include <map>
#include <variant>
#include <concepts>
#include <limits>
#include <stdexcept>

#define DECLARE_JSON_MAP(CLASSNAME) \
    static pipedal::json_map::storage_type<CLASSNAME> jmap

#define JSON_MAP_BEGIN(CLASSNAME)                     \
    pipedal::json_map::storage_type<CLASSNAME> CLASSNAME::jmap \
    {                                                 \
        {

#define JSON_MAP_REFERENCE(class, name) \
    pipedal::json_map::reference(#name, &class ::name##_),

#define JSON_MAP_REFERENCE_CONDITIONAL(class, name, conditionFn) \
    pipedal::json_map::conditional_reference(#name, &class ::name##_, conditionFn),

#define JSON_MAP_END() \
    }                  \
    }                  \
    ;

namespace pipedal
{
    class JsonException: public std::runtime_error {
    public:
        using base = std::runtime_error;
        JsonException(const std::string&msg)
        :base(msg)
        {
        }
    };

    // a function pointer of type bool (*)(const CLASS*,const MEMBER_TYPE&value) (because pre-C++ 20 templated function pointers are *hard*)
    template <typename CLASS, typename MEMBER_TYPE>
    class JsonConditionFunction
    {
    public:
        using Pointer = bool (*)(const CLASS *self, const MEMBER_TYPE &value);
    };

    //----------------------------------------------------------------------------------

    class json_reader;
    class json_writer;

    class JsonSerializable
    {
    public:
        virtual void write_json(json_writer &writer) const = 0;
        virtual void read_json(json_reader &reader) = 0;
    };
    template <typename T>
    concept IsJsonSerializable = std::derived_from<T,JsonSerializable>;


    class JsonMemberWritable
    {
    public:
        virtual void write_members(json_writer &writer) const = 0;
    };

    template <typename CLASS>
    class json_member_reference_base
    {
    private:
        const char *name_;

    protected:
        json_member_reference_base(const char *name)
            : name_(name)
        {
        }

    public:
        virtual ~json_member_reference_base() {}
        const char *name() { return this->name_; }
        virtual bool canWrite(const CLASS *self) { return true; }
        virtual void read_value(json_reader &reader, CLASS *self) = 0;
        virtual void write_value(json_writer &writer, const CLASS *self) = 0;
    };
    template <typename CLASS, typename MEMBER_TYPE>
    class json_member_reference : public json_member_reference_base<CLASS>
    {
    public:
        virtual ~json_member_reference() {}
        json_member_reference(const char *name, MEMBER_TYPE CLASS::*member_pointer)
            : json_member_reference_base<CLASS>(name), member_pointer(member_pointer)
        {
        }
        MEMBER_TYPE CLASS::*member_pointer;

        void read_value(json_reader &reader, CLASS *self);
        void write_value(json_writer &writer, const CLASS *self);
    };
    template <typename CLASS, typename MEMBER_TYPE>
    class json_conditional_member_reference : public json_member_reference_base<CLASS>
    {
    public:
        virtual ~json_conditional_member_reference() {}

        json_conditional_member_reference(const char *name, MEMBER_TYPE CLASS::*member_pointer, typename JsonConditionFunction<CLASS, MEMBER_TYPE>::Pointer condition)
            : json_member_reference_base<CLASS>(name), member_pointer(member_pointer), condition_(condition)
        {
        }
        MEMBER_TYPE CLASS::*member_pointer;

        typename JsonConditionFunction<CLASS, MEMBER_TYPE>::Pointer condition_;

        virtual bool canWrite(const CLASS *self);

        void read_value(json_reader &reader, CLASS *self);
        void write_value(json_writer &writer, const CLASS *self);
    };

    template <typename CLASS>
    class json_enum_converter
    {
    public:
        virtual CLASS fromString(const std::string &value) const = 0;
        virtual const std::string &toString(CLASS value) const = 0;
    };

    template <typename CLASS, typename MEMBER_TYPE>
    class json_enum_member_reference : public json_member_reference_base<CLASS>
    {
    public:
        json_enum_member_reference(const char *name, MEMBER_TYPE CLASS::*member_pointer, const json_enum_converter<MEMBER_TYPE> *converter)
            : json_member_reference_base<CLASS>(name), member_pointer(member_pointer), converter_(converter)
        {
        }
        MEMBER_TYPE CLASS::*member_pointer;
        const json_enum_converter<MEMBER_TYPE> *converter_;

        void read_value(json_reader &reader, CLASS *self);
        void write_value(json_writer &writer, const CLASS *self);
    };

    //----------------------------------------------------------------------------------
    class json_map_base
    {
    };

    template <typename CLASS>
    class json_map_impl : public json_map_base
    {
    private:
        std::vector<json_member_reference_base<CLASS> *> members_;
        json_map_impl(const json_map_impl &) {} // disable copy constructor.
    public:
        using members_t = std::vector<json_member_reference_base<CLASS> *>;

        json_map_impl(const members_t &members)
            : members_(members)
        {
        }
        json_map_impl(members_t &&members)
            : members_(std::forward<members_t>(members))
        {
        }
        virtual ~json_map_impl()
        {
            for (auto member : members_)
            {
                delete member;
            }
            members_.resize(0);
        }
        void read_property(json_reader *reader, const char *memberName, CLASS *pObject);
        void write_members(json_writer *writer, const CLASS *pObject);
    };

    class json_map
    {
    public:
        template <typename CLASS>
        class storage_type : public json_map_impl<CLASS>
        {
        public:
            using members_t = std::vector<json_member_reference_base<CLASS> *>;

            storage_type(const members_t &members)
                : json_map_impl<CLASS>(members)
            {
            }
            storage_type(members_t &&members)
                : json_map_impl<CLASS>(std::forward<members_t>(members))
            {
            }
        };
        template <typename CLASS, typename MEMBER_TYPE>
        static json_member_reference<CLASS, MEMBER_TYPE> *
        reference(const char *name, MEMBER_TYPE CLASS::*member_pointer)
        {
            return new json_member_reference<CLASS, MEMBER_TYPE>(name, member_pointer);
        };
        template <typename CLASS, typename MEMBER_TYPE, typename ENUM_CONVERTER>
        static json_enum_member_reference<CLASS, MEMBER_TYPE> *
        enum_reference(
            const char *name,
            MEMBER_TYPE CLASS::*member_pointer,
            const ENUM_CONVERTER *converter)
        {
            return new json_enum_member_reference<CLASS, MEMBER_TYPE>(name, member_pointer, converter);
        };

        template <typename CLASS, typename MEMBER_TYPE>
        static json_conditional_member_reference<CLASS, MEMBER_TYPE> *
        conditional_reference(const char *name, MEMBER_TYPE CLASS::*member_pointer, typename JsonConditionFunction<CLASS, MEMBER_TYPE>::Pointer condition)
        {
            return new json_conditional_member_reference<CLASS, MEMBER_TYPE>(name, member_pointer, condition);
        };
    };

    //----------------------------------------------------------------------------------

    template <typename T>
    class HasJsonPropertyMap
    {

        template <class TYPE>
        static std::true_type test(decltype(TYPE::jmap) *) { return std::true_type(); };

        template <class TYPE>
        static std::false_type test(...);

    public:
        static constexpr bool value = decltype(test<T>(nullptr))::value;
    };

    template <typename T>
    class HasExplicitJsonWrite
    {

        template <class TYPE>
        static std::true_type test() { return std::true_type(); };

        template <class TYPE>
        static std::false_type test(...);

    public:
        static constexpr bool value = decltype(test<T>(nullptr))::value;
    };

    //----------------------------------------------------------------------------------

    class json_writer;

    class json_writer
    {
    public:
        const char *CRLF;

    private:
        bool allowNaN_ = false;
        std::ostream &os;
        int indent_level;
        bool compressed;
        const int TAB_SIZE = 2;

        const uint16_t UTF16_SURROGATE_1_BASE = 0xD800U;
        const uint16_t UTF16_SURROGATE_2_BASE = 0xDC00U;
        const uint16_t UTF16_SURROGATE_MASK = 0x3FFFU;

        static const uint32_t UTF8_ONE_BYTE_MASK = 0x80;
        static const uint32_t UTF8_ONE_BYTE_BITS = 0;
        static const uint32_t UTF8_TWO_BYTES_MASK = 0xE0;
        static const uint32_t UTF8_TWO_BYTES_BITS = 0xC0;
        static const uint32_t UTF8_THREE_BYTES_MASK = 0xF0;
        static const uint32_t UTF8_THREE_BYTES_BITS = 0xE0;
        static const uint32_t UTF8_FOUR_BYTES_MASK = 0xF8;
        static const uint32_t UTF8_FOUR_BYTES_BITS = 0xF0;
        static const uint32_t UTF8_CONTINUATION_MASK = 0xC0;
        static const uint32_t UTF8_CONTINUATION_BITS = 0x80;

    public:
        static std::string encode_string(const std::string &text)
        {
            std::stringstream s;
            json_writer writer(s);
            writer.write(text);
            return s.str();
        }
        void write_raw(const char *text)
        {
            os << text;
        }
        using string_view = boost::string_view;
        json_writer(std::ostream &os, bool compressed = true, bool allowNaN = false)
            : os(os), compressed(compressed), allowNaN_(allowNaN), indent_level(0)
        {
            this->CRLF = compressed ? "" : "\r\n";
        }
        bool allowNaN() const { return allowNaN_; }
        void allowNaN(bool allow) { allowNaN_ = allow; }

        void write(uint8_t value)
        {
            os << value;
        }
        void write(int8_t value)
        {
            os << value;
        }
        void write(short value)
        {
            os << value;
        }
        void write(unsigned short value)
        {
            os << value;
        }
        void write(long long value)
        {
            os << value;
        }
        void write(unsigned long long value)
        {
            os << value;
        }
        void write(long value)
        {
            os << value;
        }
        void write(unsigned long value)
        {
            os << value;
        }
        void write(int value)
        {
            os << value;
        }
        void write(unsigned int value)
        {
            os << value;
        }

    private:
        static void throw_encoding_error();

        static uint32_t continuation_byte(std::string_view::iterator &p, std::string_view::const_iterator end);
        void write_utf16_char(uint16_t uc);

    public:
        void indent();
        std::ostream &output_stream() { return os; }

        void write(bool value)
        {
            os << (value ? "true" : "false");
        }
        void write(
            string_view v,
            bool enforceValidUtf8 // throw errors for illegal (non-minimal) UTF-8 sequences.
        );
        void write(string_view v)
        {
            write(v, true);
        }
        void write(const char *p)
        {
            write(string_view(p));
        }

        void write(const std::string &s)
        {
            write(string_view(s.c_str()));
        }
        void write(float f)
        {
            if ((std::isnan(f) || std::isinf(f)))
            {
                if (allowNaN_)
                {
                    os << "NaN";
                } else {
                    os << std::numeric_limits<float>::max();
                }
            }
            else
            {
                os << std::setprecision(std::numeric_limits<float>::max_digits10) << f; // round-trip format
            }
        }
        void write(double f)
        {
            if ((std::isnan(f) || std::isinf(f)))
            {
                if (allowNaN_)
                {
                    os << "NaN";
                } else {
                    os << std::numeric_limits<float>::max();
                }
            }
            else
            {
                os << std::setprecision(std::numeric_limits<double>::max_digits10) << f; // round-trip format
            }
        }


        template <typename T>
        void write(const std::vector<T> &value)
        {
            if (std::is_fundamental<T>() || std::is_assignable<T, const char *>() || value.size() == 0)
            {
                // simple types: all on same line.
                os << "[ ";

                if (value.size() >= 1)
                {
                    write(value[0]);
                }
                for (size_t i = 1; i < value.size(); ++i)
                {
                    os << ",";
                    write(value[i]);
                }
                os << "]";
            }
            else
            {
                // complex types: one line per entry.
                os << "[" << CRLF;
                indent_level += TAB_SIZE;
                bool first = true;
                for (size_t i = 0; i < value.size(); ++i)
                {
                    if (!first)
                    {
                        os << ',' << CRLF;
                    }
                    first = false;
                    indent();
                    write(value[i]);
                }
                indent_level -= TAB_SIZE;
                os << CRLF;
                indent();
                os << "]";
            }
        }
    void write(const std::vector<float> &value)
        {
            // simple types: all on same line.
            os << "[ ";

            if (value.size() >= 1)
            {
                write(value[0]);
            }
            for (size_t i = 1; i < value.size(); ++i)
            {
                os << ",";
                write(value[i]);
            }
            os << "]";
        }

        // template <
        //     class Category,
        //     class T,
        //     class Distance = std::ptrdiff_t,
        //     class Pointer = T *,
        //     class Reference = T &>
        // void write(std::iterator<Category, T, Distance, Pointer, Reference> &it)
        // {
        //     start_array();
        //     bool first = true;
        //     for (Reference ref : it)
        //     {
        //         if (!first)
        //         {
        //             os << ", ";
        //             os << CRLF;
        //         }
        //         indent();
        //         os << ref;
        //     }
        //     end_array();
        // }

        void write_json_member(const char *name, const char *json_text)
        {
            write(name);
            os << ": ";
            os << json_text;
        }

        template <typename T>
        void write_member(const char *name, const T &value)
        {
            write(name);
            os << ": ";
            write(value);
        }
        void start_object();
        void end_object();
        void start_array();
        void end_array();

        /*
            void write(const json_writable *obj)
            {
                if (obj == nullptr)
                {
                    os << "null";
                } else {
                    start_object();
                    obj->write(*this);
                    end_object();
                }
            }
            void write(const json_writable &obj)
            {
                start_object();
                obj.write(*this);
                end_object();
            }
        */
    private:
        template <typename T>
        json_map::storage_type<T> &get_object_map()
        {
            // static_assert(HasJsonPropertyMap<T>::value,"Type does not have a Json propery map name jmap.");
            //  Type T must have public static propery jmap.
            return T::jmap;
        }

    public:
        void write(const JsonSerializable *pWritable)
        {
            pWritable->write_json(*this);
        }
        void write(const JsonSerializable &writable)
        {
            writable.write_json(*this);
        }

        void writeRawWritable(const JsonSerializable &writable)
        {
            writable.write_json(*this);
        }
        void write(const JsonMemberWritable *pWritable)
        {
            start_object();
            pWritable->write_members(*this);
            end_object();
        }
        template <typename T>
        void write(const std::shared_ptr<T> &obj)
        {
            write(obj.get());
        }
        template <typename T>
        void write(const std::weak_ptr<T> &obj)
        {
            auto p = obj.lock();
            write(p.get());
        }
        template <typename T>
        void write(const std::unique_ptr<T> &obj)
        {
            write(obj.get());
        }

        template <typename T>
        void write(const T *obj)
        {
            static_assert(HasJsonPropertyMap<T>::value, "Json serialization type not supported. (Json object classes must have a Json property map named jmap).");
            if (obj == nullptr)
            {
                write_raw("null");
            }
            else
            {
                write(*obj);
            }
        }

        template<typename T>
        requires IsJsonSerializable<T>
        void write(T*obj)
        {
            writeRawWritable(*obj);
        }


        template <typename T>
        void write(T *obj)
        {
            static_assert(HasJsonPropertyMap<T>::value, "Json serialization type not supported. (Json object classes must have a Json property map named jmap).");
            if (obj == nullptr)
            {
                write_raw("null");
            }
            else
            {
                write(*obj);
            }
        }

        template<typename T>
        requires IsJsonSerializable<T>
        void write(T&obj)
        {
            writeRawWritable(obj);
        }

        template<typename T>
        requires IsJsonSerializable<T>
        void write(const T&obj)
        {
            writeRawWritable(obj);
        }

        template <typename T>
        void write(const T &obj)
        {
            static_assert(HasJsonPropertyMap<T>::value, "Json serialization type not supported. (Json object classes must have a Json property map named jmap).");

            auto &map = get_object_map<T>();
            start_object();

            map.write_members(this, &obj);

            end_object();
        }
        template <typename U>
        void write(const std::map<std::string, U> &map)
        {
            start_object();

            bool firstTime = true;
            for (const auto &v : map)
            {
                if (!firstTime)
                {
                    write_raw(",");
                    write_raw(CRLF);
                }
                indent();
                firstTime = false;
                write_member(v.first.c_str(), v.second);
            }
            end_object();
        }
    };

    class json_reader
    {
    private:
        std::istream &is_;

        const uint16_t UTF16_SURROGATE_1_BASE = 0xD800U;
        const uint16_t UTF16_SURROGATE_2_BASE = 0xDC00U;
        const uint16_t UTF16_SURROGATE_MASK = 0x3FFU;
        bool allowNaN_ = true;

    public:
        json_reader(std::istream &input, bool allowNaN = true)
            : is_(input)
        {
            this->allowNaN_ = allowNaN;
        }

        bool allowNaN() const { return allowNaN_; }
        void allowNaN(bool allow) { allowNaN_ = allow; }

    private:
        void throw_format_error(const char *error);

        void throw_format_error()
        {
            throw_format_error("Invalid file format");
        }

        inline bool is_whitespace(char c)
        {
            switch (c)
            {
            case 0x20:
            case 0x0A:
            case 0x0D:
            case 0x09:
                return true;
            default:
                return false;
            }
        }
        char get()
        {
            int ic = is_.get();
            if (ic == -1)
                throw_format_error("Unexpected end of file");
            return (char)ic;
        }

        void skip_whitespace();

        void read_object_start();
        uint16_t read_hex();
        uint16_t read_u_escape();

        template <typename T>
        json_map::storage_type<T> &get_object_map()
        {
            // Type T must have public static propery map.
            return T::jmap;
        }

    public:
        void start_object() { consume('{');}
        void end_object() { consume('}');}
        void consume(char expected);
        void consumeToken(const char *expectedToken, const char *errorMessage);
        int peek()
        {
            skip_whitespace();
            return is_.peek();
        }
        template<typename U>
        void read_member(const std::string&name,U *value)
        {
            std::string v;
            read(&v);
            if (v != name) throw std::logic_error("Expecting property '" + name + "'");
            consume(':');
            read(value);

        }
    public:
        std::string read_string();

    public:
        bool is_complete();

        template <typename T>
        requires IsJsonSerializable<T> 
        void read(T*pObject)
        {
            dynamic_cast<JsonSerializable*>(pObject)->read_json(*this);
        }
        template <typename T>
        void read(T *pObject)
        {
            char c;
            consume('{');

            auto &map = get_object_map<T>();
            while (true)
            {
                c = peek();
                if (c == '}')
                {
                    c = get();
                    break;
                }
                std::string memberName = read_string();

                consume(':');
                skip_whitespace();

                map.read_property(this, memberName.c_str(), pObject);

                skip_whitespace();
                if (is_.peek() == ',')
                {
                    c = get();
                }
            }
        }
        template <typename T>
        void read(T **pObject)
        {
            int c = peek();
            if (c != '{')
            {
                if (c == 'n')
                {
                    consumeToken("null", "Expecting '{' or 'null'.");
                    *pObject = nullptr;
                    return;
                }
            }
            *pObject = new T();
            read(*pObject);
        }

        template <typename U>
        void read(std::map<std::string, U> *pMap)
        {
            char c;
            consume('{');

            while (true)
            {
                c = peek();
                if (c == '}')
                {
                    c = get();
                    break;
                }
                std::string key = read_string();

                consume(':');
                skip_whitespace();

                U u;
                this->read(&u);
                (*pMap)[key] = std::move(u);
                skip_whitespace();

                if (peek() == ',')
                {
                    c = get();
                }
            }
        }

        template <typename T>
        void read(std::unique_ptr<T> *pUniquePtr)
        {
            std::unique_ptr<T> p = std::make_unique<T>();

            read(p.get());
            *pUniquePtr = std::move(p);
        }
        template <typename T>
        void read(std::shared_ptr<T> *pUniquePtr)
        {
            std::shared_ptr<T> p = std::make_shared<T>();

            read(p.get());
            (*pUniquePtr) = std::move(p);
        }
        template <typename T>
        void read(std::weak_ptr<T> *pUniquePtr)
        {
            throw std::domain_error("Can't read std::weak_ptr");
        }

    private:
        void skip_string();
        void skip_number();
        void skip_array();
        void skip_object();
        std::string readToken();
        bool read_boolean();
        void skip_token();
    public:
        void read_null();
    public:
        void skip_property();

        void read(std::string *value)
        {
            skip_whitespace();
            *value = read_string();
        }

        void read(bool *value)
        {
            skip_whitespace();
            *value = read_boolean();
        }
        void read(uint8_t*value)
        {
            skip_whitespace();
            is_ >> *value;
            if (is_.fail())
                throw JsonException("Invalid format.");
        }
        void read(short * value)
        {
            skip_whitespace();
            is_ >> *value;
            if (is_.fail())
                throw JsonException("Invalid format.");
        }

        void read(unsigned short * value)
        {
            skip_whitespace();
            is_ >> *value;
            if (is_.fail())
                throw JsonException("Invalid format.");
        }

        void read(int *value)
        {
            skip_whitespace();
            is_ >> *value;
            if (is_.fail())
                throw JsonException("Invalid format.");
        }
        void read(long *value)
        {
            is_ >> *value;
            if (is_.fail())
                throw JsonException("Invalid format.");
        }
        void read(long long *value)
        {
            skip_whitespace();
            is_ >> *value;
            if (is_.fail())
                throw JsonException("Invalid format.");
        }
        void read(unsigned int *value)
        {
            is_ >> *value;
            if (is_.fail())
                throw JsonException("Invalid format.");
        }
        void read(unsigned long *value)
        {
            is_ >> *value;
            if (is_.fail())
                throw JsonException("Invalid format.");
        }
        void read(unsigned long long *value)
        {
            skip_whitespace();
            is_ >> *value;
            if (is_.fail())
                throw JsonException("Invalid format.");
        }

        void read(float *value)
        {
            skip_whitespace();
            if (allowNaN_)
            {
                if (peek() == 'N')
                {
                    consumeToken("NaN", "Expecting a number.");
                    *value = std::nanf("");
                    return;
                }
            }
            is_ >> *value;
            if (is_.fail())
                throw JsonException("Invalid format.");
        }
        void read(double *value)
        {
            skip_whitespace();
            if (allowNaN_)
            {
                if (peek() == 'N')
                {
                    consumeToken("NaN", "Expecting a number.");

                    *value = std::nan("");
                    return;
                }
            }

            is_ >> *value;
            if (is_.fail())
                throw JsonException("Invalid format.");
        }
        template <typename T>
        void read(std::vector<T> *value)
        {
            char c;
            std::vector<T> result;

            consume('[');
            while (true)
            {
                if (peek() == ']')
                {
                    c = get();
                    break;
                }
                T item;
                read(&item);
                result.push_back(std::move(item));
                if (peek() == ',')
                {
                    c = get();
                }
            }
            *value = std::move(result);
        }
    };

    template <typename T>
    class HasJsonRead
    {

        template <typename TYPE, typename ARG>
        static std::true_type test(decltype(json_reader(std::cin).read((ARG *)nullptr)) *v) { return std::true_type(); };

        template <typename TYPE, typename ARG>
        static std::false_type test(...);

    public:
        static constexpr bool value = decltype(test<json_reader, T>(nullptr))::value;
    };

    template <typename CLASS, typename MEMBER_TYPE>
    void json_member_reference<CLASS, MEMBER_TYPE>::read_value(json_reader &reader, CLASS *self)
    {
        static_assert(HasJsonRead<MEMBER_TYPE>::value, "Type not supported for Json serialization.");
        MEMBER_TYPE *ref = &((*self).*member_pointer);
        reader.read(ref);
    }
    template <typename CLASS, typename MEMBER_TYPE>
    void json_member_reference<CLASS, MEMBER_TYPE>::write_value(json_writer &writer, const CLASS *self)
    {
        const MEMBER_TYPE &ref = (*self).*member_pointer;
        writer.write_member(this->name(), ref);
    }

    template <typename CLASS, typename MEMBER_TYPE>
    void json_enum_member_reference<CLASS, MEMBER_TYPE>::read_value(json_reader &reader, CLASS *self)
    {
        MEMBER_TYPE *ref = &((*self).*member_pointer);
        std::string val = reader.read_string();
        *ref = converter_->fromString(val);
    }

    template <typename CLASS, typename MEMBER_TYPE>
    void json_enum_member_reference<CLASS, MEMBER_TYPE>::write_value(json_writer &writer, const CLASS *self)
    {
        const MEMBER_TYPE &ref = (*self).*member_pointer;
        std::string val = converter_->toString(ref);
        writer.write_member(this->name(), val);
    }

    template <typename CLASS>
    void json_map_impl<CLASS>::read_property(json_reader *reader, const char *memberName, CLASS *pObject)
    {
        for (json_member_reference_base<CLASS> *member : members_)
        {
            if (strcmp(member->name(), memberName) == 0)
            {
                member->read_value(*reader, pObject);
                return;
            }
        }
        reader->skip_property();
    }

    template <typename CLASS, typename MEMBER_TYPE>
    void json_conditional_member_reference<CLASS, MEMBER_TYPE>::read_value(json_reader &reader, CLASS *self)
    {
        static_assert(HasJsonRead<MEMBER_TYPE>::value, "Type not supported for Json serialization.");
        MEMBER_TYPE *ref = &((*self).*member_pointer);
        reader.read(ref);
    }

    template <typename CLASS, typename MEMBER_TYPE>
    bool json_conditional_member_reference<CLASS, MEMBER_TYPE>::canWrite(const CLASS *self)
    {
        const MEMBER_TYPE &ref = (*self).*member_pointer;
        return this->condition_(self, ref);
    }

    template <typename CLASS, typename MEMBER_TYPE>
    void json_conditional_member_reference<CLASS, MEMBER_TYPE>::write_value(json_writer &writer, const CLASS *self)
    {
        const MEMBER_TYPE &ref = (*self).*member_pointer;
        writer.write_member(this->name(), ref);
    }

    template <typename CLASS>
    void json_map_impl<CLASS>::write_members(json_writer *writer, const CLASS *pObject)
    {
        bool first = true;
        for (json_member_reference_base<CLASS> *member : members_)
        {
            if (member->canWrite(pObject))
            {
                if (!first)
                {
                    writer->output_stream() << ',';
                    writer->output_stream() << writer->CRLF;
                }
                first = false;
                writer->indent();
                member->write_value(*writer, pObject);
            }
        }
    }

} // namespace pipedal