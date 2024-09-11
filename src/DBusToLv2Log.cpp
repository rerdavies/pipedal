// Copyright (c) 2024 Robin Davies
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

#include "DBusToLv2Log.hpp"
#include "DBusLog.hpp"
#include "Lv2Log.hpp"

using namespace pipedal;

class DBus2ToLv2Logger : public IDBusLogger
{

public:
    virtual void LogError(const std::string &message) override
    {
        Lv2Log::error(message);
    }
    virtual void LogWarning(const std::string &message) override
    {
        Lv2Log::warning(message);
    }
    virtual void LogInfo(const std::string &message) override
    {
        Lv2Log::info(message);
    }
    virtual void LogDebug(const std::string &message) override
    {
        Lv2Log::debug(message);
    }
    virtual void LogTrace(const std::string &message) override
    {
        Lv2Log::debug(message);
    }
};

void pipedal::DbusLogToLv2Log()
{
    SetDBusLogger(std::make_unique<DBus2ToLv2Logger>());
}
