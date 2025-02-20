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

#pragma once


#include <cstdint>
#include <string>
#include <string_view>

namespace pipedal {
    size_t Utf8Index(size_t size, const std::string &text);
    size_t Utf8Decrement(size_t size, const std::string &text);
    size_t Utf8Increment(size_t size, const std::string &text);
    size_t Utf8Length(const std::string&text);

    std::string Utf8Erase(const std::string&text, size_t start, size_t end);
    std::string Utf8Substring(const std::string&text, size_t start, size_t end);
    inline std::string Utf8Substring(const std::string&text, size_t start) { 
        return Utf8Substring(text,start,(size_t)-1); 
    }
    namespace implementation {
        void Utf8RangeError();
    }

    std::string Utf8FromUtf32(char32_t value);


    std::u16string Utf8ToUtf16(const std::string_view&s);
    std::string Utf16ToUtf8(const std::u16string_view&s);


}

