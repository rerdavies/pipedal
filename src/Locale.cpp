// Copyright (c) 2021 Robin Davies
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

#include "pch.h"

#include "Locale.hpp"

#include <stdlib.h>


// Must be UNICODE. Should reflect system locale.  (.e.g en-US.UTF8,  de-de.UTF8)

// This is correct for libstdc++; most probably not correct for Windows or CLANG. :-(
using namespace pipedal;

static const char*getLocale(const char*localeEnvironmentVariable)
{
    const char*result = getenv(localeEnvironmentVariable);
    if (result == nullptr)
    {
        result = getenv("LC_ALL");
    }
    if (result == nullptr) {
        result = "en_US.UTF-8";
    }
    return result;
}

static std::locale collationLocale(getLocale("LC_COLLATION"));
const std::collate<char>& Locale::collation = std::use_facet<std::collate<char> >(collationLocale);

