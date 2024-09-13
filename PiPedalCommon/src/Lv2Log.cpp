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

#include <string>
#include <vector>
#include "Lv2Log.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

using namespace pipedal;

#include <mutex>




static auto timeZero = std::chrono::system_clock::now();
bool Lv2Log::show_time_ = true;

std::string timeTag()
{
    if (!Lv2Log::show_time()) return "";

    using namespace std::chrono;

    auto now = std::chrono::system_clock::now();
    std::time_t now_c = std::chrono::system_clock::to_time_t(now);

   auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;


    std::stringstream s;

    using namespace std;
    s << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S") << "." << std::setw(3) << std::setfill('0') << milliseconds.count() << " ";
    return s.str();
    
}
class StdErrLogger : public Lv2Logger
{
    std::mutex m;

    
    public : virtual ~StdErrLogger() {}

    virtual void onError(const char* message)
    {
        std::lock_guard lock(m);
        std::cerr << timeTag() << "ERR: " << message << std::endl;
    }
    virtual void onWarning(const char* message)
    {
        std::lock_guard lock(m);
        std::cerr  << timeTag() << "WRN: " << message << std::endl;
    }

    virtual void onDebug(const char* message)
    {
        std::lock_guard lock(m);
        std::cerr  << timeTag() << "DBG: " << message << std::endl;
    }
    virtual void onInfo(const char* message)
    {
        std::lock_guard lock(m);
        std::cerr  << timeTag()  << "INF: " << message << std::endl;
    }
};

Lv2Logger *Lv2Log::logger_ = new StdErrLogger();

LogLevel Lv2Log::log_level_ = LogLevel::Debug;

void Lv2Log::v_error(const char *format, va_list arglist)
{
    char buffer[512];
    vsnprintf(buffer,sizeof(buffer),format,arglist);
    logger_->onError(buffer);
}
void Lv2Log::v_warning(const char *format, va_list arglist)
{
    char buffer[512];
    vsnprintf(buffer,sizeof(buffer),format,arglist);
    logger_->onWarning(buffer);
}
void Lv2Log::v_debug(const char *format, va_list arglist)
{
    char buffer[512];
    vsnprintf(buffer,sizeof(buffer),format,arglist);
    logger_->onDebug(buffer);
}
void Lv2Log::v_info(const char *format, va_list arglist)
{
    char buffer[512];
    vsnprintf(buffer,sizeof(buffer),format,arglist);
    logger_->onInfo(buffer);

}


