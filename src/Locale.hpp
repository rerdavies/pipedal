#pragma once

#include <locale>

namespace pipedal {

    class Locale {
    public:
        static const std::collate<char>& collation;
    };
}
