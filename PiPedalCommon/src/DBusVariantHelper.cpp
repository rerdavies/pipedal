#include "DBusVariantHelper.hpp"
#include <sdbus-c++/Types.h>
#include <iostream>


//std::ostream &operator<<(std::ostream &s, const std::map<std::string, sdbus::Variant> &properties)
std::ostream&operator<<(std::ostream&s, const sdbus::Variant&v)
{
            // clang-format off
#define HANDLE_VARIANT_TYPE(type)      \
    if (v.containsValueOfType<type>()) \
    {                                  \
        s << v.get<type>();            \
    }

            HANDLE_VARIANT_TYPE(std::string)
            else if (v.containsValueOfType<uint8_t>())
            {
                uint8_t value = v.get<uint8_t>();
                s << (int)value;
            }
            else HANDLE_VARIANT_TYPE(uint32_t) 
            else HANDLE_VARIANT_TYPE(int32_t) 
            else HANDLE_VARIANT_TYPE(uint64_t) 
            else HANDLE_VARIANT_TYPE(int64_t) 
            else HANDLE_VARIANT_TYPE(uint16_t) 
            else HANDLE_VARIANT_TYPE(int16_t) 
            else HANDLE_VARIANT_TYPE(bool) 
            else HANDLE_VARIANT_TYPE(sdbus::Variant) 
            else if (v.containsValueOfType<sdbus::ObjectPath>())
            {
                auto objectPath = v.get<sdbus::ObjectPath>();
                s << '[' <<  objectPath.c_str() << ']';
            }
            else if (v.containsValueOfType<std::vector<sdbus::Variant>>())
            {
                auto values = v.get<std::vector<sdbus::Variant>>();
                {
                s << '[';
                bool first = true;
                for (const sdbus::Variant&value: values)
                {
                    if (!first) s << ", ";
                    first = false;
                    s << value;
                }
                s << ']';

                }

            }
            else if (v.containsValueOfType<std::vector<sdbus::ObjectPath>>())
            {
                auto paths = v.get<std::vector<sdbus::ObjectPath> >();
                {
                s << '[';
                bool first = true;
                for (const sdbus::ObjectPath&path: paths)
                {
                    if (!first) s << ", ";
                    first = false;
                    s << path.c_str();
                }
                s << ']';

                }
            }
            else if (v.containsValueOfType<std::vector<uint8_t>>()) 
            {
                std::vector<uint8_t> bytes = v.get<std::vector<uint8_t>>();
                s << '[';
                bool first = true;
                for (uint8_t byte: bytes)
                {
                    if (!first) s << ", ";
                    first = false;
                    s << (int)byte;

                }
                s << ']';

            }
            else if (v.containsValueOfType<std::vector<std::vector<uint8_t>>>()) 
            {
                auto arrayOfArrays = v.get<std::vector<std::vector<uint8_t>>>();
                s << '[';
                bool first = true;
                for (const auto& array: arrayOfArrays)
                {
                    if (!first) s << ", ";
                    first = false;
                    bool first2 = true;
                    for (auto byte: array)
                    {
                        if (!first2) s << ", ";
                        first2 = false;
                        s << byte;

                    }
                }
                s << ']';

            }
            else if (v.containsValueOfType<std::map<std::string,sdbus::Variant>>()) 
            {
                std::map<std::string,sdbus::Variant> map = v.get<std::map<std::string,sdbus::Variant>>();
                s << "{";
                bool first = true;
                for (const auto &[key, value]:  map)
                {
                    if (!first) s << ",";
                    first = false;
                    s << '\"' << key << "\": "  << value;
                }
                s << "}";
            } else if (v.containsValueOfType<std::map<std::string,std::vector<uint8_t>>>())
            {
                std::map<std::string,std::vector<uint8_t>> values 
                    = v.get<std::map<std::string,std::vector<uint8_t>>>();
                s << '{';
                bool first = true;
                for (const auto &[key,bytes]: values)
                {
                    if (!first) s << ',';
                    first = false;
                    s << "{\"" << key << "\": [";
                    bool first2 = true;
                    for (auto b : bytes)
                    {
                        if (!first2) s << ',';
                        first2 = false;
                        s << b;
                    }
                    s << "]";
                }
                s << '}';

            }

             //            else HANDLE_VARIANT_TYPE(float) 
            // else HANDLE_VARIANT_TYPE(double) 
            // else HANDLE_VARIANT_TYPE(uint64_t) 
            // else HANDLE_VARIANT_TYPE(int64_t) 
            // else HANDLE_VARIANT_TYPE(char) 
            // else HANDLE_VARIANT_TYPE(uint8_t) 
            // else HANDLE_VARIANT_TYPE(int8_t) 
            else
            {
                s << "UNKNOWN VALUE OF TYPE " << v.peekValueType();
            }
#undef HANDLE_VARIANT_TYPE
            // clang-format on
    return s;
}
std::ostream&operator<<(std::ostream&s,const std::map<std::string, sdbus::Variant>& properties)
{

    s << "{";
    bool first = true;
    for (const auto &property : properties)
    {
        if (!first)
        {
            s << ", ";
        }
        first = false;
        s << '"' << property.first << "\": ";

        const sdbus::Variant &v = property.second;
        s << v;
    }
    
    s << "}";
    return s;
}
