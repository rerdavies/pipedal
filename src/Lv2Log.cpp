#include "pch.h"
#include "Lv2Log.hpp"
#include <iostream>
#include <chrono>
#include <iomanip>

using namespace pipedal;

#include <mutex>




static auto timeZero = std::chrono::system_clock::now();

std::string timeTag()
{
    using namespace std::chrono;

    auto t = std::chrono::system_clock::now()- timeZero;


    auto hours_ = duration_cast<hours>(t).count() % 24;
    auto minutes_ = duration_cast<minutes>(t).count() % 60;
    auto seconds_ = duration_cast<seconds>(t).count() % 60;
    auto milliseconds_ = duration_cast<milliseconds>(t).count() % 1000;

    std::stringstream s;

    using namespace std;
    s << setfill('0') << setw(2) << hours_ << ':' << setw(2) << minutes_ << ':' << setw(2) << seconds_ << "." << setw(3) << milliseconds_;
    return s.str();
    
}
class StdErrLogger : public Lv2Logger
{
    std::mutex m;

    
    public : virtual ~StdErrLogger() {}

    virtual void onError(const char* message)
    {
        std::lock_guard lock(m);
        std::cerr << timeTag() << " ERR: " << message << std::endl;
    }
    virtual void onWarning(const char* message)
    {
        std::lock_guard lock(m);
        std::cerr  << timeTag() << " WRN: " << message << std::endl;
    }

    virtual void onDebug(const char* message)
    {
        std::lock_guard lock(m);
        std::cerr  << timeTag() << " DBG: " << message << std::endl;
    }
    virtual void onInfo(const char* message)
    {
        std::lock_guard lock(m);
        std::cerr  << timeTag()  << " INF: " << message << std::endl;
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


