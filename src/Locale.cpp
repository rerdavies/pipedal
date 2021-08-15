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

