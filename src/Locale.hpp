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

#include <memory>
#include <string>

namespace pipedal {
    class Collator {
    public:
        using ptr = std::shared_ptr<Collator>;
        virtual int Compare(const std::string &left, const std::string&right) = 0;
        virtual int Compare(const std::u16string &left, const std::u16string&right) = 0;
        virtual ~Collator();
    };
    class Locale {
    protected:
        Locale() { }
    public:
        virtual ~Locale();
        // no copy, no move
        Locale(const Locale&) = delete;
        Locale(Locale&&) = delete;
        Locale&operator =(const Locale&) = delete;
        Locale&operator =(Locale&&) = delete;
        
        using ptr = std::shared_ptr<Locale>;

        static ptr GetInstance();
        static ptr GetTestInstance(const std::string&locale); // testing only. 

        virtual const std::string &CurrentLocale() const = 0;
        virtual Collator::ptr GetCollator() = 0;
    private:
        static ptr g_instance;
    };
}
