/*
 * MIT License
 *
 * Copyright (c) 2022 Robin E. R. Davies
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

#include <string>
#include <sstream>
#include <exception>
#include "autoptr_vector.h"
#include <unordered_map>
#include <fstream>
#include "ofstream_synced.hpp"
#include "util.hpp"

using namespace pipedal;

namespace config_serializer
{
    namespace detail
    {
        std::string trim(const std::string &v);

        /**
         * @brief Convert string to config file format.
         *
         * Adds quotes and escapes, but only if neccessary.
         *
         * Source text is assumed to be UTF-8. Escapes for the
         * following values only: \r \n \t \" \\.
         *
         *
         * @param s
         * @return std::string Encoded string.
         */
        std::string EncodeString(const std::string &s);

        /**
         * @brief Config file format to string.
         *
         * Decodes quotes and escapes, but only if neccessary.
         *
         * Source text is assumed to be UTF-8. Escapes for the
         * following values only: \r \n \t \" \\.
         *
         * @param s
         * @return std::string
         */
        std::string DecodeString(const std::string &s);

        /**
         * @brief Convert a string to an integer.
         *
         * @tparam T A signed or unsigned integral type.
         * @param value The string to parse.
         * @return T The parsed result.
         * @throws std::invalid_argument if the supplied string is not a valid integer.
         */
        template <class T>
        T ToInt(const std::string &value)
        {
            std::stringstream s(value);
            T result;
            s >> result;
            if (s.fail())
            {
                throw std::invalid_argument("Invalid integer value.");
            }
            return result;
        }

    }

    using namespace detail;

    template <class OBJ>
    class ConfigSerializerBase
    {
    protected:
        ConfigSerializerBase(const std::string &name, const std::string &comment)
            : name(name),
              comment(comment)
        {
        }

    public:
        using Obj = OBJ;
        virtual ~ConfigSerializerBase() {}

        const std::string &GetName() const { return name; }
        const std::string &GetComment() const { return comment; }
        virtual void SetValue(Obj &configurable, const std::string &value) const = 0;
        virtual const std::string GetValue(Obj &configurable) const = 0;

    private:
        std::string name;
        std::string comment;
    };

    template <class T>
    class ConfigConverter
    {
    public:
        static T FromString(const std::string &value)
        {
            T result;
            std::stringstream s(value);
            s >> result;
            return result;
        }

        static std::string ToString(const T &value)
        {
            std::stringstream s;
            s << value;
            return s.str();
        }
    };

    // string specializations.
    template <>
    inline std::string ConfigConverter<std::string>::FromString(const std::string &value)
    {
        return DecodeString(value);
    }
    template <>
    inline std::string ConfigConverter<std::string>::ToString(const std::string &config)
    {
        return EncodeString(config);
    }
    // bool specializations.
    template <>
    inline bool ConfigConverter<bool>::FromString(const std::string &value)
    {
        bool result = false;
        if (value == "")
        {
            result = false;
        }
        else if (value == "true")
        {
            result = true;
        }
        else if (value == "false")
        {
            result = false;
        }
        else
        {
            try
            {
                int t = ToInt<int>(value);
                result = (t != 0);
            }
            catch (const std::exception &)
            {
                throw std::invalid_argument("Expecting true or false.");
            }
        }
        return result;
    }
    template <>
    inline std::string ConfigConverter<bool>::ToString(const bool &value)
    {
        return value ? "true" : "false";
    }

    template <class OBJ, class T>
    class ConfigSerializer : public ConfigSerializerBase<OBJ>
    {
    public:
        using Obj = OBJ;
        using pointer_type = T Obj::*;
        using base = ConfigSerializerBase<Obj>;
        using converter = ConfigConverter<T>;

        ConfigSerializer(const std::string &name,
                         pointer_type pMember,
                         const std::string &comment)
            : base(name, comment),
              pMember(pMember)
        {
        }
        virtual void SetValue(Obj &configurable, const std::string &value) const
        {
            configurable.*pMember = ConfigConverter<T>::FromString(value);
        }
        virtual const std::string GetValue(Obj &configurable) const
        {
            return ConfigConverter<T>::ToString(configurable.*pMember);
        }

    private:
        pointer_type pMember;
    };

    template <class OBJ>
    class ConfigSerializable
    {
    private:
        using Obj = OBJ;

        using serializer_t = ConfigSerializerBase<OBJ>;
        using serializers_t = p2p::autoptr_vector<serializer_t>;

        const serializers_t &serializers;

    protected:
        ConfigSerializable(const serializers_t &serializers)
            : serializers(serializers)
        {
        }

    public:
        virtual void Save(std::ostream &f)
        {
            bool firstLine = true;
            for (const serializer_t *serializer : serializers)
            {
                if (serializer->GetComment().length() != 0)
                {
                    if (!firstLine)
                    {
                        f << std::endl;
                    }
                    firstLine = false;
                    std::vector<std::string> comments = split(serializer->GetComment(), '\n');

                    if (comments.size() != 0)
                        for (const std::string &comment : comments)
                        {
                            f << "# " << comment << std::endl;
                        }
                }

                f << serializer->GetName() << '=' << serializer->GetValue((Obj &)*this) << std::endl;
            }
        }
        void Save(const std::string &path)
        {
            pipedal::ofstream_synced f;
            f.open(path);
            if (!f.is_open())
            {
                throw std::invalid_argument("Can't write to file " + path);
            }
            try
            {
                Save(f);
            }
            catch (const std::exception &e)
            {
                throw std::invalid_argument("Error writing to '" + path + "'. " + e.what());
            }
        }

        virtual void Load(std::istream &f)
        {
            std::unordered_map<std::string, serializer_t *> index;

            for (serializer_t *serializer : serializers)
            {
                index[serializer->GetName()] = serializer;
            }

            std::string line;
            int nLine = 0;
            while (true)
            {
                if (f.eof())
                    break;
                getline(f, line);
                ++nLine;

                // remove comments.
                auto npos = line.find_first_of('#');
                if (npos != std::string::npos)
                {
                    line = line.substr(0, npos);
                }
                size_t start = 0;
                while (start < line.size() && line[start] == ' ')
                {
                    ++start;
                }
                if (start == line.size())
                {
                    continue;
                }

                auto pos = line.find_first_of('=', start);
                if (pos == std::string::npos)
                {
                    std::stringstream msg;

                    msg << "(" << nLine << "," << (start + 1) << "): Syntax error. Expecting '='.";
                    throw std::logic_error(msg.str());
                }
                std::string label = trim(line.substr(start, pos - start));
                std::string value = trim(line.substr(pos + 1));

                if (index.contains(label))
                {
                    serializer_t *serializer = index[label];
                    try
                    {
                        serializer->SetValue((Obj &)*this, value);
                    }
                    catch (const std::exception &e)
                    {
                        std::stringstream msg;
                        msg << "(" << nLine << ',' << (pos + 1) << "): " << e.what();
                        throw std::logic_error(msg.str());
                    }
                }
                else
                {
                    std::stringstream msg;
                    msg << "(" << nLine << ',' << (start) << "): "
                        << "Invalid property: " + label;
                    throw std::logic_error(
                        msg.str());
                }
            }
        }

        void Load(const std::string &path, bool throwIfNotFound = false)
        {
            try
            {
                std::ifstream f;
                f.open(path);
                if (!f.is_open())
                {
                    if (throwIfNotFound)
                    {
                        throw std::logic_error("Can't open file.");
                    }
                    return;
                }
                Load(f);
            }
            catch (const std::exception &e)
            {
                std::stringstream msg;
                msg << "Error: " << path << " " << e.what();
                throw std::logic_error(msg.str());
            }
        }
    };
}
