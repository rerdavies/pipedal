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
#include "json.hpp"

namespace pipedal
{
    class json_null
    {
    public:
        bool operator==(const json_null&other) const { return true;}
    private:
        int value = 0;
    };


    template <class T> // avoid ordering problem in declarations.
    class json_object_base: public JsonSerializable
    {
    public:
        using json_variant = T;
        json_object_base() {}

        T &operator[](const std::string &index) { return values[index]; }
        const T &operator[](const std::string &index) const { return values[index]; }

    public: 
        bool operator==(const json_object_base<T> &other) const
        {
            for (const auto &pair: this->values)
            {
                auto index = other.values.find(pair.first);
                if (index == other.values.end()) return false;
                if (!(index->second == pair.second)) return false;
            }
            for (const auto &pair: other.values)
            {
                auto index = this->values.find(pair.first);
                if (index == this->values.end()) return false;
                if (!(index->second == pair.second)) return false;
            }
            return true;
        }
    private:
        virtual void read_json(json_reader&reader) {
            reader.read(&(this->values));
        }
        virtual void write_json(json_writer&writer) const {
            writer.start_object();
            bool first = true;
            for (auto&value: values)
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
        std::map<std::string, T> values;
    };
    template <class T> // avoid ordering problem in declarations.
    class json_array_base: public JsonSerializable
    {
    public:
        json_array_base() {}

        T &operator[](size_t index) { 
            check_index(index);
            return values[index]; }
        const T &operator[](size_t &index) const { 
            check_index(index);
            return values[index]; 
        }
        void resize(size_t size)
        {
            values.resize(size);
        }
        size_t size() const { return values.size(); }
        template <typename U>
        void push_back(const U&value) { values.push_back(value); }
        template <typename U>
        void push_back(U&&value) { values.push_back(value); }
        bool operator==(const json_array_base<T>&other) const
        {
            if (!(this->size() == other.size())) return false;
            for (size_t i = 0; i < this->size(); ++i)
            {
                if (!((*this)[i] == other[i])) return false;
            }
            return true;
        }
    private:
        virtual void read_json(json_reader&reader) {
            reader.read(&(this->values));
        }    
        virtual void write_json(json_writer&writer) const {
            writer.start_array();
            bool first = true;
            for (auto&value: values)
            {
                if (!first) writer.write_raw(",");
                first = false;
                writer.writeRawWritable(value);
            }
            writer.end_array();
        }

        void check_index(size_t size) const
        {
            if (size >= values.size())
            {
                throw std::out_of_range("index out of range.");
            }
        }
        std::vector<T> values;
    };


    class concrete_json_variant_base {
    protected:
        void write_double_value(json_writer &writer,double value) const;
    };

    template <typename DUMMY = void>
    class json_variant_base 
    : public std::variant<json_null, bool, double, std::string, json_object_base<json_variant_base<DUMMY>>, json_array_base<json_variant_base<DUMMY>>>,
        public JsonSerializable,
        private concrete_json_variant_base
    {
    public:
        using base = std::variant<json_null, bool, double, std::string, json_object_base<json_variant_base<DUMMY>>, json_array_base<json_variant_base<DUMMY>>>;
        using json_object = json_object_base<json_variant_base<DUMMY>>;
        using json_array = json_array_base<json_variant_base<DUMMY>>;
        using json_variant = json_variant_base<void>;


        json_variant_base(json_null value)
        :base(value)
        {

        }
        json_variant_base(double value)
            : base(value)
        {
        }
        json_variant_base(int value)
            : base((double)value)
        {
        }
        json_variant_base(const std::string &value)
            : base(value)
        {
        }
        json_variant_base(const char*value)
        :base(std::string(value))
        {

        }
        json_variant_base()
            : base(json_null())
        {
        }
        json_variant_base(json_object &&value)
            : base(std::forward<json_object>(value))
        {
        }
        json_variant_base(json_array &&value)
        : base(std::forward<json_array>(value))
        {
        }

        template <typename U>
        bool holds_alternative() const { return std::holds_alternative<U>(*this);}

        bool IsNull() const { return holds_alternative<json_null>(); }
        bool IsBool() const { return holds_alternative<bool>(); }
        bool IsNumber() const { return holds_alternative<double>(); }
        bool IsString() const { return holds_alternative<std::string>(); }
        bool IsObject() const { return holds_alternative<json_object>(); }
        bool IsArray() const { return holds_alternative<json_array>(); }

        template <typename U>
        const U &get() const
        {
            return std::get<U>(*this);
        }
        template <typename U>
        U &get()
        {
            return std::get<U>(*this);
        }

        bool &AsBool() { return get<bool>(); }
        bool AsBool() const  { return get<bool>(); }

        double &AsNumber() { return get<double>(); }
        double AsNumber() const { return get<double>(); }
        std::string &AsString() { return get<std::string>(); }
        const std::string &AsString() const { return get<std::string>(); }

        json_object &AsObject() { return get<json_object>(); }
        const json_object &AsObject() const { return get<json_object>(); }

        std::vector<float> AsFloatArray() { return get<json_object>().AsFloatArray(); }
        std::vector<double> AsDoubleArray() { return get<json_object>().AsDoubleArray(); }

        json_array &AsArray() { return get<json_array>(); }
        const json_array &AsArray() const { return get<json_array>(); }

        // convenience methods for object and array manipulation.
        static json_variant MakeObject() { return json_variant{ json_object()};};
        static json_variant MakeArray() { return json_variant{ json_array()};};

        void resize(size_t size) { AsArray().resize(size); }
        size_t size() const { return AsArray().size(); }

        json_variant&operator[](size_t index) { return AsArray()[index];}
        const json_variant&operator[](size_t index) const { return AsArray()[index];}

        const json_variant&operator[](const std::string& index) const { return AsObject()[index];}
        json_variant&operator[](const std::string& index) { return AsObject()[index];}


    private:
        virtual void read_json(json_reader &reader)
        {
            int v = reader.peek();
            if (v == '[')
            {
                json_array array;
                reader.read(&array);
                (*this) = std::move(array);
            } else if (v == '{')
            {
                json_object object;
                reader.read(&object);
                (*this) = std::move(object);
            }
            else if (v == '\"') {
                std::string s;
                reader.read(&s);
                (*this) = std::move(s);
            } else if (v == 'n')
            {   
                reader.read_null();
                (*this) = json_null();

            } else if (v == 't' || v == 'f')
            {
                bool b;
                reader.read(&b);
                (*this) = b;

            } else {
                // it's a number.
                double v;
                reader.read(&v);
                (*this) = v;
            }
        }
        virtual void write_json(json_writer&writer) const
        {
            switch (this->index())
            {
            case 0:
                writer.write_raw("null");
                break;
            case 1:
                writer.write(this->get<bool>());
                break;
            case 2:
                write_double_value(writer,this->get<double>());
                break;
            case 3:
                writer.write(get<std::string>());
                break;
            case 4:
                writer.writeRawWritable(get<json_object>());
                break;
            case 5:
                writer.writeRawWritable(get<json_array>());
                break;
            default:
                throw std::logic_error("Invalid variant index");
            }
        }
    };

    using json_variant = json_variant_base<void>;
    using json_object = json_variant::json_object;
    using json_array = json_variant::json_array;

} // namespace pipedal