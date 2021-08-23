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

#pragma once
#include <stdarg.h>
#include <chrono>

namespace pipedal
{

    class Lv2Logger
    {
    public:
        virtual ~Lv2Logger() = default;

        virtual void onError(const char *message) = 0;
        virtual void onWarning(const char *message) = 0;
        virtual void onDebug(const char *message) = 0;
        virtual void onInfo(const char *message) = 0;
    };

    enum class LogLevel
    {
        None = 0,
        Error = 1,
        Warning = 2,
        Info = 3,
        Debug = 4
    };

    class Lv2Log
    {
    private:
        static Lv2Logger *logger_;
        static LogLevel log_level_;

        static void v_error(const char *format, va_list arglist);
        static void v_warning(const char *format, va_list arglist);
        static void v_debug(const char *format, va_list arglist);
        static void v_info(const char *format, va_list arglist);

    public:
        static void set_logger(Lv2Logger *logger)
        {
            Lv2Log::logger_ = logger;
        }
        static void log_level(LogLevel level)
        {
            Lv2Log::log_level_ = level;
        }
        static LogLevel log_level()
        {
            return Lv2Log::log_level_;
        }

        // static void error(const char* message) {
        //     if (Lv2Log::log_level_ >= LogLevel::Error)
        //     {
        //         logger_->onError(message);
        //     }
        // }
        // static void warning(const char* message) {
        //     if (Lv2Log::log_level_ >= LogLevel::Warning)
        //     {
        //         logger_->onWarning(message);
        //     }
        // }
        // static void debug(const char* message) {
        //     if (Lv2Log::log_level_ >= LogLevel::Debug)
        //     {
        //         logger_->onDebug(message);
        //     }
        // }
        // static void info(const char* message) {
        //     if (Lv2Log::log_level_ >= LogLevel::Info)
        //     {
        //         logger_->onInfo(message);
        //     }
        // }

        static void info(const char *format, ...)
        {
            if (Lv2Log::log_level_ >= LogLevel::Info)
            {
                va_list arglist;
                va_start(arglist, format);
                v_info(format, arglist);
                va_end(arglist);
            }
        }
        static void info(const std::string &str)
        {
            info("%s", str.c_str());
        }

        static void debug(const char *format, ...)
        {
            if (Lv2Log::log_level_ >= LogLevel::Debug)
            {
                va_list arglist;
                va_start(arglist, format);
                v_debug(format, arglist);
                va_end(arglist);
            }
        }
        static void debug(const std::string &str)
        {
            debug("%s", str.c_str());
        }

        static void warning(const char *format, ...)
        {
            if (Lv2Log::log_level_ >= LogLevel::Warning)
            {
                va_list arglist;
                va_start(arglist, format);
                v_warning(format, arglist);
                va_end(arglist);
            }
        }
        static void warning(const std::string &str)
        {
            warning("%s", str.c_str());
        }

        static void error(const char *format, ...)
        {
            if (Lv2Log::log_level_ >= LogLevel::Warning)
            {
                va_list arglist;
                va_start(arglist, format);
                v_error(format, arglist);
                va_end(arglist);
            }
        }
        static void error(const std::string &str)
        {
            error("%s", str.c_str());
        }
    };

} // namespace pipedal