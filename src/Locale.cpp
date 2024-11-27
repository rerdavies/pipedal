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

/*
    libicu does major version bumps without regard to whether api changes are major or minor. Documention
    does, in fact, guarantee binary compatibility for the APIs that we are using. The major version bump
    creates absolute havoc with .deb packaging, because the major version bumps on roughluy a monthyly basis.
    This makes it impossible to generate .deb packages that are plausible portable between debian-derived distros.

    The alternative: dynmaically link to the APIs that we are using, referencing non-major-versioned references
    to the libicu dlls (e.g. libicuuc.lib instead of libicuuc.lib.74). Given how few APIs we are actually using,
    this seems perfectly reasonable. fwiw, .net runtime uses a similar approach.

*/

#include "pch.h"
#include "Locale.hpp"
#include "ss.hpp"
#include <mutex>
#include "Utf8Utils.hpp"
#include <filesystem>

#define U_SHOW_CPLUSPLUS_API 0

#include <stdlib.h>
#include "Lv2Log.hpp"
#include <unicode/utypes.h>
#include <unicode/ucol.h>
#include <unicode/unistr.h>
#include <unicode/uvernum.h>
#include <stdexcept>
#include "ss.hpp"
#include <mutex>
#include <dlfcn.h>

using namespace pipedal;
using namespace std;
namespace fs = std::filesystem;

static inline bool isDigit(char c)
{
    return c >= '0' && c <= '9';
}
static bool getSoVersionNumber(const std::string&fileName, int *version)
{
    auto fnamePos = fileName.find_last_of('/');
    if (fnamePos == std::string::npos)
    {
        fnamePos = 0;
    }
    auto extensionPos = fileName.find(".so.",fnamePos);
    if (extensionPos == std::string::npos)
    {
        return false;
    }


    const char *p = fileName.c_str() + extensionPos + 4;

    if (!isDigit(*p)) {
        return false;
    }
    int n = 0;
    while (isDigit(*p))
    {
        n = n*10 + *p-'0';
        ++p;
    }
    if (*p != '\0' && *p != '.')
    {
        return false;
    }
    *version = n;
    return true;
}
// Function to get ICU version dynamically
static int getICUVersion(void* libHandle) {

    // pares the dlerror to get the version number. :-/
    dlerror(); // clear the error.
    void * nonExistentFunction = dlsym(libHandle, "nonExistentFunction");

    std::string error = dlerror();
    // "/lib/aarch64-linux-gnu/libicui18n.so.74: undefined symbol: nonExistentFunction"

    auto nPos = error.find(":");
    if (nPos == std::string::npos)
    {
        return false;
    }
    std::string fileName = error.substr(nPos);

    int version = -1;

    if (!getSoVersionNumber(fileName,&version))
    {
        fs::path basePath = fileName;
        fs::path parentDirectory = basePath.parent_path();
        auto fileName = basePath.filename().string();
        for (auto&entry: fs::directory_iterator(parentDirectory))
        {
            if (entry. path().filename().string().starts_with(fileName))
            {
                if (getSoVersionNumber(entry.path().string(),&version))
                {
                    break;
                }

            }
        }
        if (version == -1)
        {
            throw std::runtime_error(SS("Unable to determine libicui18n.so version: " << error));
        }
    }
    return version;
}


class DynamicIcuLoader
{
public:
    using ptr = std::shared_ptr<DynamicIcuLoader>;
    // Function pointer types
    typedef UCollator *(*ucol_open_t)(const char *, UErrorCode *);
    typedef void (*ucol_close_t)(UCollator *);
    typedef UCollationResult (*ucol_strcoll_t)(const UCollator *, const UChar *, int32_t, const UChar *, int32_t);
    using ucol_setStrength_t = typeof(&ucol_setStrength);

    static ptr icuLoader;
    static std::mutex icuLoaderMutex;

    static DynamicIcuLoader::ptr get_instance()
    {
        std::lock_guard lock{icuLoaderMutex};

        if (!icuLoader)
        {
            icuLoader = std::make_shared<DynamicIcuLoader>();
        }
        return icuLoader;
    }

    ~DynamicIcuLoader()
    {
        // Close the library if it was opened
        if (library_handle)
        {
            dlclose(library_handle);
            library_handle = nullptr;
        }
    }

    ucol_open_t ucol_open_fn;
    ucol_close_t ucol_close_fn;
    ucol_strcoll_t ucol_strcoll_fn;
    ucol_setStrength_t ucol_setStrength_fn;

    DynamicIcuLoader() : library_handle(nullptr),
                         ucol_open_fn(nullptr),
                         ucol_close_fn(nullptr),
                         ucol_strcoll_fn(nullptr)
    {
#ifndef DISABLE_DYNAMIC_ICU_LOADER
        load();
#else
        this->ucol_open_fn = &ucol_open;
        this->ucol_close_fn = &ucol_close;
        this->ucol_strcoll_fn = &ucol_strcoll;
        this->ucol_setStrength_fn = &ucol_setStrength;
#endif
    }

private:
    void *library_handle;
    // Function pointers
    static std::string VersionedName(const char*name,int version)
    {
        std::stringstream ss;
        ss << name << "_" << version;
        return ss.str();
    }
    void load()
    {
        try
        {
            // Open the library
            library_handle = dlopen("libicui18n.so", RTLD_LAZY);
            if (!library_handle)
            {
                throw std::runtime_error(SS("Error loading library: " << dlerror()));
            }


            int version = getICUVersion(library_handle);

            // Clear any existing errors
            dlerror();

            // Load ucol_open
            ucol_open_fn = reinterpret_cast<ucol_open_t>(dlsym(library_handle, VersionedName("ucol_open",version).c_str()));
            if (!ucol_open_fn)
            {

                throw std::runtime_error(SS("Error loading ucol_open: " << dlerror()));
            }

            // Load ucol_close
            ucol_close_fn = reinterpret_cast<ucol_close_t>(dlsym(library_handle, VersionedName("ucol_close",version).c_str()));
            if (!ucol_close_fn)
            {
                throw std::runtime_error(SS("Error loading ucol_close: " << dlerror()));
            }

            // Load ucol_strcoll
            ucol_strcoll_fn = reinterpret_cast<ucol_strcoll_t>(dlsym(library_handle, VersionedName("ucol_strcoll",version).c_str()));
            if (!ucol_strcoll_fn)
            {
                throw std::runtime_error(SS("Error loading ucol_strcoll: " << dlerror()));
            }
            this->ucol_setStrength_fn = reinterpret_cast<ucol_setStrength_t>(dlsym(library_handle, VersionedName("ucol_setStrength",version).c_str()));
            if (!ucol_setStrength_fn)
            {
                throw std::runtime_error(SS("Error loading ucol_setStrength: " << dlerror()));
            }
        }
        catch (const std::exception &e)
        {
            Lv2Log::warning(e.what());
            this->ucol_open_fn = fallback_ucol_open_func;
            this->ucol_close_fn = fallback_ucol_close_func;
            this->ucol_strcoll_fn = fallback_ucol_strcoll_func;
            this->ucol_setStrength_fn = fallback_ucol_setStrength_func;
        }
    }

    static void fallback_ucol_setStrength_func(UCollator *coll,
                 UCollationStrength strength)
    {
    
    }


    static UCollator *fallback_ucol_open_func(const char *locale, UErrorCode *ec)
    {
        *ec = UErrorCode::U_ZERO_ERROR;
        return (UCollator *)(void *)-1;
    }
    static void fallback_ucol_close_func(UCollator *)
    {
    }
    static UCollationResult fallback_ucol_strcoll_func(const UCollator *, const UChar *left, int32_t nLeft, const UChar *right, int32_t nRight)
    {
        auto c = std::min(nLeft, nRight);

        for (int32_t i = 0; i < c; ++i)
        {
            UChar cl = *left++;
            UChar cr = *right++;
            if (cl != cr)
            {
                if (cl >= 'A' && cl <= 'Z')
                {
                    cl += 'A' - 'a';
                }
                if (cr >= 'A' && cr <= 'Z')
                {
                    cr += 'A' - 'a';
                }
                if (cl != cr)
                {
                    if (cl < cr)
                    {
                        return UCollationResult::UCOL_LESS;
                    }
                    else
                    {
                        return UCollationResult::UCOL_GREATER;
                    }
                }
            }
        }
        return UCollationResult::UCOL_EQUAL;
    }
};

DynamicIcuLoader::ptr DynamicIcuLoader::icuLoader;
std::mutex DynamicIcuLoader::icuLoaderMutex;

std::string getCurrentLocale()
{
    std::string locale = setlocale(LC_ALL, nullptr);
    if (locale.empty() || locale == "C")
    {
        // If setlocale fails, try getting it from environment variables
        const char *lang = getenv("LC_ALL");
        if (lang)
        {
            locale = lang;
        }
        else
        {
            // If LANG is not set, fall back to a default
            lang = getenv("LC_COLLATE");
            if (lang)
            {
                locale = lang;
            }
            else
            {
                lang = getenv("LANG");
                if (lang)
                {
                    locale = lang;
                }
                else
                {
                    locale = "en_US";
                }
            }
        }
    }
    // Extract just the language and country code
    size_t dot_pos = locale.find('.');
    if (dot_pos != std::string::npos)
    {
        locale = locale.substr(0, dot_pos);
    }
    return locale;
}

Collator::~Collator()
{
}

class LocaleImpl;

class CollatorImpl : public Collator
{
public:
    CollatorImpl(std::shared_ptr<LocaleImpl> localeImpl, const char *localeStr);
    ~CollatorImpl();

    virtual int Compare(const std::string &left, const std::string &right);
    virtual int Compare(const std::u16string &left, const std::u16string &right);

private:
    UCollator *collator = nullptr;
    std::shared_ptr<LocaleImpl> localeImpl;

    DynamicIcuLoader::ptr icuLoader;
};

CollatorImpl::~CollatorImpl()
{
    if (collator)
    {
        icuLoader->ucol_close_fn(collator);
        collator = nullptr;
    }
    localeImpl = nullptr;
}

CollatorImpl::CollatorImpl(std::shared_ptr<LocaleImpl> localeImpl, const char *localeStr)
    : localeImpl(localeImpl)
{
    icuLoader = DynamicIcuLoader::get_instance();
    UErrorCode status = U_ZERO_ERROR;
    collator = icuLoader->ucol_open_fn(localeStr, &status);

    if (U_FAILURE(status))
    {
        throw std::runtime_error(SS("Failed to create collator: " << status));
    }
    icuLoader->ucol_setStrength_fn(collator,UCollationStrength::UCOL_PRIMARY);
}

int CollatorImpl::Compare(const std::u16string &left, const std::u16string&right) 
{
    return icuLoader->ucol_strcoll_fn(collator,
                                      reinterpret_cast<const UChar *>(left.c_str()), left.length(),
                                      reinterpret_cast<const UChar *>(right.c_str()), right.length());

}

int CollatorImpl::Compare(const std::string &left_, const std::string &right_)
{
    std::u16string left = Utf8ToUtf16(left_);
    std::u16string right = Utf8ToUtf16(right_);

    return icuLoader->ucol_strcoll_fn(collator,
                                      reinterpret_cast<const UChar *>(left.c_str()), left.length(),
                                      reinterpret_cast<const UChar *>(right.c_str()), right.length());
}

Locale::ptr g_instance;

class LocaleImpl : public Locale, public std::enable_shared_from_this<LocaleImpl>
{
public:
    LocaleImpl();
    LocaleImpl(const std::string&locale);
    virtual const std::string &CurrentLocale() const;
    virtual Collator::ptr GetCollator();

private:
    std::string currentLocale;
};

LocaleImpl::LocaleImpl()
{
    currentLocale = getCurrentLocale();
}
LocaleImpl::LocaleImpl(const std::string& locale)
{
    currentLocale = locale;
}


const std::string &LocaleImpl::CurrentLocale() const
{
    return currentLocale;
}

Collator::ptr LocaleImpl::GetCollator()
{
    auto pThis = shared_from_this();
    return std::shared_ptr<Collator>(new CollatorImpl(pThis, currentLocale.c_str()));
}

static std::mutex createMutex;

Locale::~Locale()
{
}

Locale::ptr Locale::g_instance;

Locale::ptr Locale::GetInstance()
{
    std::lock_guard lock{createMutex};

    if (g_instance)
    {
        return g_instance;
    }
    g_instance = std::make_shared<LocaleImpl>();
    return g_instance;
}

Locale::ptr Locale::GetTestInstance(const std::string&locale)
{
    return std::make_shared<LocaleImpl>(locale);
}