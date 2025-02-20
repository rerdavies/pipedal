// Copyright (c) 2023 Robin E. R. Davies
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

#include "Utf8Utils.hpp"
#include "stdexcept"
#include <sstream>

using namespace std;
namespace pipedal
{


    static constexpr uint16_t UTF16_SURROGATE_TAG_MASK = 0xFC00;
    static constexpr uint16_t UTF16_SURROGATE_1_BASE = 0xD800U;
    static constexpr uint16_t UTF16_SURROGATE_2_BASE = 0xDC00U;


    static constexpr uint32_t UTF8_ONE_BYTE_MASK = 0x80;
    static constexpr uint32_t UTF8_ONE_BYTE_BITS = 0;
    static constexpr uint32_t UTF8_TWO_BYTES_MASK = 0xE0;
    static constexpr uint32_t UTF8_TWO_BYTES_BITS = 0xC0;
    static constexpr uint32_t UTF8_THREE_BYTES_MASK = 0xF0;
    static constexpr uint32_t UTF8_THREE_BYTES_BITS = 0xE0;
    static constexpr uint32_t UTF8_FOUR_BYTES_MASK = 0xF8;
    static constexpr uint32_t UTF8_FOUR_BYTES_BITS = 0xF0;
    static constexpr uint32_t UTF8_CONTINUATION_MASK = 0xC0;
    static constexpr uint32_t UTF8_CONTINUATION_BITS = 0x80;
    static constexpr bool enforceValidUtf8Encoding = false;

    namespace implementation
    {
        void Utf8RangeError()
        {
            throw std::range_error("String index out of range.");
        }
    }
    size_t Utf8Index(size_t size, const std::string &text)
    {
        size_t index = 0;
        for (size_t i = 0; i < size; ++i)
        {
            index = Utf8Increment(index, text);
            if (index >= text.size())
                break;
        }
        return index;
    }
    size_t Utf8Length(const std::string &text)
    {
        size_t index = 0;
        size_t length = 0;
        while (index < text.length())
        {
            index = Utf8Increment(index, text);
            ++length;
        }
        return length;
    }
    size_t Utf8Increment(size_t size, const std::string &text)
    {
        if (size >= text.length())
        {
            implementation::Utf8RangeError();
        }
        uint8_t uc = (uint8_t)text[size];
        if (uc < 0x80U)
        {
            return size + 1;
        }
        else
        {
            ++size;
            while (size < text.size() && (((uint8_t)text[size]) & 0xC0) == 0x80)
            {
                ++size;
            }
            return size;
        }
    }
    size_t Utf8Decrement(size_t size, const std::string &text)
    {
        if (size == 0)
        {
            implementation::Utf8RangeError();
        }
        uint8_t c = text[size - 1];
        if (c < 0x80U)
        {
            return size - 1;
        }
        else
        {
            while ((uint8_t)(text[size - 1] & 0xC0) == 0x80)
            {
                --size;
            }
            if (size != 0 && (uint8_t)(text[size - 1]) > 0x80)
            {
                --size;
            }
            else
            {
                implementation::Utf8RangeError();
            }

            return size;
        }
    }

    std::string Utf8Erase(const std::string &text, size_t start, size_t end)
    {
        size_t uStart = Utf8Index(start, text);
        size_t uEnd = Utf8Index(end, text);
        std::string result;
        result.reserve(uStart + text.size() - uEnd);
        result.append(text.begin(), text.begin() + uStart);
        result.append(text.begin() + uEnd, text.end());
        return result;
    }
    std::string Utf8Substring(const std::string &text, size_t start, size_t end)
    {
        size_t uStart = Utf8Index(start, text);
        size_t uEnd;
        if (end == (size_t)-1)
        {
            uEnd = text.length();
        }
        else
        {
            uEnd = Utf8Index(end, text);
        }
        return text.substr(uStart, uEnd - uStart);
    }

    static char32_t bits(int nBits, int shift, char32_t c)
    {
        return (c >> shift) & ((1 << nBits) - 1);
    }
    std::string Utf8FromUtf32(char32_t value)
    {
        std::stringstream ss;
        if (value < 0x7F)
        {
            ss << (char)value;
        }
        else if (value <= 0x7FF)
        {
            ss << (char)(0xC0 + bits(5, 6, value));
            ss << (char)(0x80 + bits(6, 0, value));
        }
        else if (value <= 0xFFFFul)
        {
            ss << (char)(0xE0 + bits(4, 12, value));
            ss << (char)(0x80 + bits(6, 6, value));
            ss << (char)(0x80 + bits(6, 0, value));
        }
        else if (value <= 0x10FFFFul)
        {
            ss << (char)(0xF0 + bits(3, 18, value));
            ss << (char)(0x80 + bits(6, 12, value));
            ss << (char)(0x80 + bits(6, 6, value));
            ss << (char)(0x80 + bits(6, 0, value));
        }
        else
        {
            throw std::runtime_error("Invalid unicode character.");
        }
        return ss.str();
    }

    std::string Utf16ToUtf8(const std::u16string_view&v)
    {
        std::stringstream ss;
        for (auto i = v.begin(); i != v.end(); /**/)
        {
            char32_t value = *i++;
            if ((value & UTF16_SURROGATE_TAG_MASK) == UTF16_SURROGATE_1_BASE)
            {
                if (i == v.end())
                {
                    throw std::runtime_error("Invalid UTF32 character sequence.");
                }
                char32_t value2 = *i++;
                if ((value & UTF16_SURROGATE_TAG_MASK) != UTF16_SURROGATE_2_BASE)
                {
                    throw std::runtime_error("Invalid UTF32 character sequence.");
                }
                value = (value << 10) + value2;
            }

            if (value < 0x7F)
            {
                ss << (char)value;
            }
            else if (value <= 0x7FF)
            {
                ss << (char)(0xC0 + bits(5, 6, value));
                ss << (char)(0x80 + bits(6, 0, value));
            }
            else if (value <= 0xFFFFul)
            {
                ss << (char)(0xE0 + bits(4, 12, value));
                ss << (char)(0x80 + bits(6, 6, value));
                ss << (char)(0x80 + bits(6, 0, value));
            }
            else if (value <= 0x10FFFFul)
            {
                ss << (char)(0xF0 + bits(3, 18, value));
                ss << (char)(0x80 + bits(6, 12, value));
                ss << (char)(0x80 + bits(6, 6, value));
                ss << (char)(0x80 + bits(6, 0, value));
            }
            else
            {
                throw std::runtime_error("Invalid unicode character.");
            }
        }
        return ss.str();
    }


    [[noreturn]] static void throw_encoding_error()
    {
        throw std::runtime_error("Invalid UTF8 character.");
    }

    static uint32_t continuation_byte(std::string_view::iterator &p, std::string_view::const_iterator end)
    {
        if (p == end)
            throw_encoding_error();
        uint8_t c = *p++;
        if ((c & UTF8_CONTINUATION_MASK) != UTF8_CONTINUATION_BITS)
            throw_encoding_error();
        return c & 0x3F;
    }

    std::u16string Utf8ToUtf16(const std::string_view &v)
    {
        std::basic_stringstream<char16_t> os;
        // convert to utf-32.
        // convert utf-32 to normalized utf-16.
        // write non-7-bit and unsafe characters as \uHHHH.

        auto p = v.begin();
        while (p != v.end())
        {
            uint32_t uc;
            uint8_t c = (uint8_t)*p++;
            if ((c & UTF8_ONE_BYTE_MASK) == UTF8_ONE_BYTE_BITS)
            {
                uc = c;
            }
            else
            {
                uint32_t c2 = continuation_byte(p, v.end());

                if ((c & UTF8_TWO_BYTES_MASK) == UTF8_TWO_BYTES_BITS)
                {
                    uint32_t c1 = c & (uint32_t)(~UTF8_TWO_BYTES_MASK);
                    if (c1 <= 1 && enforceValidUtf8Encoding)
                    {
                        // overlong encoding.
                        throw_encoding_error();
                    }
                    uc = (c1 << 6) | c2;
                }
                else
                {
                    uint32_t c3 = continuation_byte(p, v.end());

                    if ((c & UTF8_THREE_BYTES_MASK) == UTF8_THREE_BYTES_BITS)
                    {
                        uint32_t c1 = c & (uint32_t)~UTF8_THREE_BYTES_MASK;
                        if (c1 == 0 && c2 < 0x20 && enforceValidUtf8Encoding)
                        {
                            // overlong encoding.
                            throw_encoding_error();
                        }

                        uc = (c1) << 12 | (c2 << 6) | c3;
                    }
                    else
                    {
                        uint32_t c4 = continuation_byte(p, v.end());
                        if ((c & UTF8_FOUR_BYTES_MASK) == UTF8_FOUR_BYTES_BITS)
                        {
                            uint32_t c1 = c & (uint32_t)~UTF8_FOUR_BYTES_MASK;
                            if (c1 == 0 && c2 < 0x10 && enforceValidUtf8Encoding)
                            {
                                // overlong encoding.
                                throw_encoding_error();
                            }
                            uc = (c1 << 18) | (c2 << 12) | (c3 << 6) | c4;
                        }
                        else
                        {
                            // outside legal UCS range.
                            throw_encoding_error();
                        }
                    }
                }
            }
            if (uc < 0x10000ul)
            {
                os << (char16_t)uc;
            }
            else
            {
                // write UTF-16 surrogate pair.
                uc -= 0x10000;

                char16_t s1 = (char16_t)(UTF16_SURROGATE_1_BASE + ((uc >> 10) & 0x3FFu));
                char16_t s2 = (char16_t)(UTF16_SURROGATE_2_BASE + (uc & 0x03FFu));
                // surrogate pair.
                os << s1;
                os << s2;
            }
        }
        return os.str();
    }


 
}