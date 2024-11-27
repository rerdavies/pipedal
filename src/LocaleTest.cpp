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

#include "pch.h"
#include "catch.hpp"
#include <string>
#include <memory>

#include <unicode/locid.h> // before Locale.hpp
#include <unicode/unistr.h>
#include <unicode/coleitr.h>
#include <unicode/coll.h>

#include "Locale.hpp"
#include <iostream>
#include <vector>
#include <algorithm>

using namespace pipedal;
using namespace std;


static std::string ToUtf8(const icu::UnicodeString &value)
{
    std::string result;
    icu::StringByteSink<std::string> sink(&result);
    value.toUTF8(sink);

    sink.Flush();
    return result;
}


class LocaleEnumerator {
public:
    // Enumerate all available locales
    static std::vector<std::string> getAllLocales() {
        std::vector<std::string> locales;
        
        // Get the number of available locales
        int32_t count = 0;
        const icu::Locale* availableLocales = icu::Locale::getAvailableLocales(count);
        
        for (int32_t i = 0; i < count; ++i) {
            const icu::Locale& locale = availableLocales[i];
            
            // Construct full locale name
            std::string localeName = constructLocaleName(locale);
            
            locales.push_back(localeName);
        }
        
        // Sort locales alphabetically
        std::sort(locales.begin(), locales.end());
        
        return locales;
    }

    // Detailed locale information
    static void printLocaleDetails(const std::string& localeName) {
        try {
            icu::Locale locale(localeName.c_str());
            
            // Display locale information
            std::cout << "Locale: " << localeName << "\n";
            
            // Language display name
            icu::UnicodeString languageName;
            locale.getDisplayLanguage(locale, languageName);
            std::cout << "  Language: " << ToUtf8(languageName) << "\n";
            
            // Country display name
            icu::UnicodeString countryName;
            locale.getDisplayCountry(locale, countryName);
            std::cout << "  Country: " << ToUtf8(countryName) << "\n";
            
            // Variant information
            icu::UnicodeString variantName;
            locale.getDisplayVariant(locale, variantName);
            if (!variantName.isEmpty()) {
                std::cout << "  Variant: " << ToUtf8(variantName) << "\n";
            }
            UErrorCode status = U_ZERO_ERROR;

            std::unique_ptr<icu::Collator> collator(icu::Collator::createInstance(localeName.c_str(),status));
            icu::Collator::ECollationStrength strength = collator->getStrength();
            std::cout << "  Default strength: " << strength << "\n";

        }
        catch (const std::exception& e) {
            std::cerr << "Error processing locale " << localeName << ": " << e.what() << std::endl;
        }
    }

    // Enumerate locales by language or country
    static std::vector<std::string> getLocalesByLanguage(const std::string& language) {
        std::vector<std::string> matchingLocales;
        
        int32_t count = 0;
        const icu::Locale* availableLocales = icu::Locale::getAvailableLocales(count);
        
        for (int32_t i = 0; i < count; ++i) {
            const icu::Locale& locale = availableLocales[i];
            
            if (language == locale.getLanguage()) {
                std::string localeName = constructLocaleName(locale);
                matchingLocales.push_back(localeName);
            }
        }
        
        std::sort(matchingLocales.begin(), matchingLocales.end());
        return matchingLocales;
    }

private:
    // Construct a comprehensive locale name
    static std::string constructLocaleName(const icu::Locale& locale) {
        std::string localeName;
        
        const char* lang = locale.getLanguage();
        const char* country = locale.getCountry();
        const char* variant = locale.getVariant();
        
        if (lang && *lang) {
            localeName += lang;
            
            if (country && *country) {
                localeName += "_";
                localeName += country;
                
                if (variant && *variant) {
                    localeName += ".";
                    localeName += variant;
                }
            }
        }
        
        return localeName;
    }
};

int enumerateLocales() {
    try {
        // Get and print total number of available locales
        int32_t count = 0;
        icu::Locale::getAvailableLocales(count);
        std::cout << "Total Available Locales: " << count << "\n\n";

        // Demonstrate different locale enumeration methods
        
        // 1. Get all locales
        std::vector<std::string> allLocales = LocaleEnumerator::getAllLocales();
        for (size_t i = 0; i < allLocales.size(); ++i) {
            std::cout << allLocales[i] << "\n";
        }

        // 2. Get locales by specific language (e.g., English)
        std::cout << "\nFrench Locales:\n";
        std::vector<std::string> englishLocales = LocaleEnumerator::getLocalesByLanguage("fr");
        for (const auto& locale : englishLocales) {
            std::cout << locale << "\n";
        }

        // 3. Detailed information for a specific locale
        std::cout << "\nDetailed Locale Information:\n";
        LocaleEnumerator::printLocaleDetails("en_US");
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}

void printSortOptions(const std::string&localeName)
{
    UErrorCode status = U_ZERO_ERROR;
    std::unique_ptr<icu::Collator> collator(icu::Collator::createInstance(localeName.c_str(),status));

}

void u16Test() {
    {
        Locale::ptr locale = Locale::GetTestInstance("en_US");
        REQUIRE(locale->GetCollator()->Compare(u"ÄbcAe",u"Abcde") < 0); // standard sort order, not phone-book sort order.
        REQUIRE(locale->GetCollator()->Compare(u"Äbcdefg",u"Abcde") > 0); // standard sort order, not phone-book sort order.
        REQUIRE(locale->GetCollator()->Compare(u"Ä",u"A") == 0); // standard sort order, not phone-book sort order.
        REQUIRE(locale->GetCollator()->Compare(u"A",u"b") < 0);
        REQUIRE(locale->GetCollator()->Compare(u"B",u"a") > 0);
        REQUIRE(locale->GetCollator()->Compare(u"A",u"a") == 0);
        REQUIRE(locale->GetCollator()->Compare(u"c",u"ç") == 0);
        REQUIRE(locale->GetCollator()->Compare(u"e",u"è") == 0);

    }
    {
        Locale::ptr locale = Locale::GetTestInstance("fr_FR");
        REQUIRE(locale->GetCollator()->Compare(u"Ä",u"A") == 0); // standard sort order, not phone-book sort order.
        REQUIRE(locale->GetCollator()->Compare(u"A",u"b") < 0);
        REQUIRE(locale->GetCollator()->Compare(u"B",u"a") > 0);
        REQUIRE(locale->GetCollator()->Compare(u"A",u"a") == 0);
        REQUIRE(locale->GetCollator()->Compare(u"c",u"ç") == 0);
        REQUIRE(locale->GetCollator()->Compare(u"e",u"è") == 0);
    }
    {
        Locale::ptr locale = Locale::GetTestInstance("de_DE");
        REQUIRE(locale->GetCollator()->Compare(u"Ä",u"A") == 0); // standard sort order, not phone-book sort order.
        REQUIRE(locale->GetCollator()->Compare(u"A",u"b") < 0);
        REQUIRE(locale->GetCollator()->Compare(u"B",u"a") > 0);
        REQUIRE(locale->GetCollator()->Compare(u"A",u"a") == 0);
        REQUIRE(locale->GetCollator()->Compare(u"c",u"ç") == 0);
        REQUIRE(locale->GetCollator()->Compare(u"e",u"è") == 0);
    }
    {
        Locale::ptr locale = Locale::GetTestInstance("da_DK");
        REQUIRE(locale->GetCollator()->Compare(u"Å",u"Z") > 0); 
    }
}

void u8Test() {
    {
        Locale::ptr locale = Locale::GetTestInstance("en_US");
        REQUIRE(locale->GetCollator()->Compare("Äbcae","Abcde") < 0); // standard sort order, not phone-book sort order.

        REQUIRE(locale->GetCollator()->Compare("Ä","A") == 0); // standard sort order, not phone-book sort order.
        REQUIRE(locale->GetCollator()->Compare("A","b") < 0);
        REQUIRE(locale->GetCollator()->Compare("B","a") > 0);
        REQUIRE(locale->GetCollator()->Compare("A","a") == 0);
        REQUIRE(locale->GetCollator()->Compare("c","ç") == 0);
        REQUIRE(locale->GetCollator()->Compare("e","è") == 0);

    }
    {
        Locale::ptr locale = Locale::GetTestInstance("fr_FR");
        REQUIRE(locale->GetCollator()->Compare("Ä","A") == 0); // standard sort order, not phone-book sort order.
        REQUIRE(locale->GetCollator()->Compare("A","b") < 0);
        REQUIRE(locale->GetCollator()->Compare("B","a") > 0);
        REQUIRE(locale->GetCollator()->Compare("A","a") == 0);
        REQUIRE(locale->GetCollator()->Compare("c","ç") == 0);
        REQUIRE(locale->GetCollator()->Compare("e","è") == 0);
    }
    {
        Locale::ptr locale = Locale::GetTestInstance("de_DE");
        REQUIRE(locale->GetCollator()->Compare("Ä","A") == 0); // standard sort order, not phone-book sort order.
        REQUIRE(locale->GetCollator()->Compare("A","b") < 0);
        REQUIRE(locale->GetCollator()->Compare("B","a") > 0);
        REQUIRE(locale->GetCollator()->Compare("A","a") == 0);
        REQUIRE(locale->GetCollator()->Compare("c","ç") == 0);
        REQUIRE(locale->GetCollator()->Compare("e","è") == 0);
    }
    {
        Locale::ptr locale = Locale::GetTestInstance("da_DK");
        REQUIRE(locale->GetCollator()->Compare("Å","Z") > 0); 
    }
}

TEST_CASE( "Locale test", "[locale]" ) {
    cout << "======  Locale test ===============" << endl;


    // enumerateLocales();
    // printSortOptions("de_DE");

    u8Test();
    u16Test();



}