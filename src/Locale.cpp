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

#include "Locale.hpp"

#include <stdlib.h>
#include "Lv2Log.hpp"
#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/unistr.h>
#include <unicode/sortkey.h>
#include <stdexcept>
#include "ss.hpp"
#include <mutex>

// Must be UNICODE. Should reflect system locale.  (.e.g en-US.UTF8,  de-de.UTF8)

// This is correct for libstdc++; most probably not correct for Windows or CLANG. :-(
using namespace pipedal;


std::string getCurrentLocale() {
    std::string locale = setlocale(LC_ALL, nullptr);
    if (locale.empty() || locale == "C") {
        // If setlocale fails, try getting it from environment variables
        const char* lang = getenv("LC_ALL");
        if (lang) {
            locale = lang;
        } else {
            // If LANG is not set, fall back to a default
            lang = getenv("LC_COLLATE");
            if (lang)
            {
                locale = lang;
            } else{
                lang = getenv("LANG");
                if (lang) {
                    locale = lang;
                } else {
                    locale = "en_US";
                }
            }
        }
    }
    // Extract just the language and country code
    size_t dot_pos = locale.find('.');
    if (dot_pos != std::string::npos) {
        locale = locale.substr(0, dot_pos);
    }
    return locale;
}

Collator::~Collator() {

}

class LocaleImpl;

class CollatorImpl : public Collator {
public:
    CollatorImpl(std::shared_ptr<LocaleImpl> LocaleImpl,icu::Locale &locale);
    ~CollatorImpl();

    virtual int Compare(const std::string &left, const std::string&right);
private:
    icu::Collator* collator = nullptr;
    std::shared_ptr<LocaleImpl> localeImpl;
};

CollatorImpl::~CollatorImpl()
{
    delete collator;
    localeImpl = nullptr;
}
CollatorImpl::CollatorImpl(std::shared_ptr<LocaleImpl> localeImpl, icu::Locale &locale)
:localeImpl(localeImpl)
{
    UErrorCode status = U_ZERO_ERROR;

    this->collator = icu::Collator::createInstance(locale, status);

    if (U_FAILURE(status)) {
        throw std::runtime_error(SS("Failed to create collator: " << u_errorName(status)));
    }
}

int CollatorImpl::Compare(const std::string &left, const std::string&right) {
    return collator->compare(left.c_str(),right.c_str());
}
Locale::ptr g_instance;

class LocaleImpl: public Locale, public std::enable_shared_from_this<LocaleImpl>  {
public:
    LocaleImpl();
    virtual const std::string &CurrentLocale() const ;
    virtual Collator::ptr GetCollator();
private:
    std::unique_ptr<icu::Locale> locale;
    std::string currentLocale;
};

LocaleImpl::LocaleImpl()
{
    currentLocale = getCurrentLocale();
    locale = std::make_unique<icu::Locale>(currentLocale.c_str());
}
const std::string &LocaleImpl::CurrentLocale() const 
{
    return currentLocale;
}


Collator::ptr LocaleImpl::GetCollator(){
    auto pThis = shared_from_this();
    return std::shared_ptr<Collator>(new CollatorImpl(pThis,*locale));
}

static std::mutex createMutex;

Locale::~Locale() 
{

}
Locale::ptr Locale::g_instance;

Locale::ptr Locale::GetInstance()
{
    std::lock_guard lock { createMutex};

    if (g_instance)
    {
        return g_instance;
    }
    g_instance = std::make_shared<LocaleImpl>();
    return g_instance;
}


