#include "DBusLog.hpp"
#include <memory>
#include "ss.hpp"
#include <systemd/sd-journal.h>
#include <fstream>
#include <stdexcept>
#include "ss.hpp"
#include <chrono>
#include "ofstream_synced.hpp"


namespace impl {
    std::mutex logMutex;
    DBusLogLevel dbusLogLevel = DBusLogLevel::Info;

}

static std::string getDateTime()
{
    auto now = std::chrono::system_clock::now();
    time_t in_time_t = std::chrono::system_clock::to_time_t(now);
    auto ms = std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()) % 1000;
    std::stringstream ss; 
    ss << std::put_time(std::localtime(&in_time_t),"%Y-%m-%d %H:%M:%S");
    ss << '.' << std::setfill('0') << std::setw(3) << ms.count();
    ss << " ";
    return ss.str();
}

class ConsoleDBusLogger : public IDBusLogger {

public:
    ConsoleDBusLogger(bool showTime = true)
    :showTime(showTime)
    {

    }
    void LogError(const std::string&message) {
        std::cout << getDateTime() << "error: " << message << std::endl;
    }
    void LogWarning(const std::string&message){
        std::cout << getDateTime() << "warning: " << message << std::endl;
    }
    void LogInfo(const std::string&message) {
        std::cout << getDateTime() << "info: " << message << std::endl;
    }
    void LogDebug(const std::string&message) {
        std::cout << getDateTime() << "debug: " << message << std::endl;
    }
    void LogTrace(const std::string&message) {
        std::cout << getDateTime() << " trace: " << message << std::endl;
    }
private:

    std::string getDateTime() {
        return  (showTime) ? getDateTime(): "";
    }
    bool showTime;
};
class FileDBusLogger : public IDBusLogger {

public:
    FileDBusLogger(const std::filesystem::path&path)
    {
        f.open(path);
        if (!f)
        {
            throw std::runtime_error(SS("Can't open file " << path));
        }
    }
    void LogError(const std::string&message) {
        f << "error: " << message << std::endl;
    }
    void LogWarning(const std::string&message){
        f << "warning: " << message << std::endl;
    }
    void LogInfo(const std::string&message) {
        f << "info: " << message << std::endl;
    }
    void LogDebug(const std::string&message) {
        f << "debug: " << message << std::endl;
    }
    void LogTrace(const std::string&message) {
        f << "trace: " << message << std::endl;
    }
private:
    pipedal::ofstream_synced f;
};

class SystemdDBusLogger : public IDBusLogger {

public:
    void LogError(const std::string&message) {
        sd_journal_print(LOG_ERR,"%s",message.c_str());

    }
    void LogWarning(const std::string&message){
        sd_journal_print(LOG_WARNING,"%s",message.c_str());
    }
    void LogInfo(const std::string&message) {
        sd_journal_print(LOG_INFO,"%s",message.c_str());

    }
    void LogDebug(const std::string&message) {
        sd_journal_print(LOG_DEBUG,"%s",message.c_str());
    }
    void LogTrace(const std::string&message) {
        // no trace level. just log as debug.
        sd_journal_print(LOG_DEBUG,"%s",message.c_str());
    }

};




static std::vector<std::unique_ptr<IDBusLogger>> loggers;

void SetDBusLogger(std::unique_ptr<IDBusLogger> && logger)
{
    loggers.clear();
    loggers.push_back(std::move(logger));
}


void SetDBusLogLevel(DBusLogLevel level)
{
    impl::dbusLogLevel = level;
}

void LogError(const std::string&path,const char*method,const std::string&message)
{
    if (impl::dbusLogLevel > DBusLogLevel::Error) return;
    std::lock_guard<std::mutex> lock{impl::logMutex};
    for (auto &logger: loggers)
    {
        logger->LogError(SS(path<< "::" << method << ": "<< message));
    }
}
void LogInfo(const std::string&path,const char*method,const std::string&message)
{
    if (impl::dbusLogLevel > DBusLogLevel::Info) return;
    std::lock_guard<std::mutex> lock{impl::logMutex};
    for (auto &logger: loggers)
    {
        logger->LogInfo(SS(path<< "::" << method<< ": " << message));
    }
}

void LogDebug(const std::string&path,const char*method,const std::string&message)
{
    if (impl::dbusLogLevel > DBusLogLevel::Debug) return;
    std::lock_guard<std::mutex> lock{impl::logMutex};
    
    for (auto &logger: loggers)
    {
        logger->LogDebug(SS(path<< "::" << method << ": "<< message));
    }
}

void LogDebug(const std::string&path,const char*method,const std::function<std::string ()> &text)
{
    if (impl::dbusLogLevel > DBusLogLevel::Debug) return;
    std::lock_guard<std::mutex> lock{impl::logMutex};
    
    for (auto &logger: loggers)
    {
        logger->LogDebug(SS(path<< "::" << method << ": " << text()));
    }
}

void LogTrace(const std::string&path,const char*method,const std::function<std::string ()> &text)
{
    if (impl::dbusLogLevel > DBusLogLevel::Trace) return;
    std::lock_guard<std::mutex> lock{impl::logMutex};
    for (auto &logger: loggers)
    {
        logger->LogTrace(SS(path<< "::" << method << ": " << text()));
    }
}
void LogWarning(const std::string&path,const char*method,const std::function<std::string ()> &text)
{
    if (impl::dbusLogLevel > DBusLogLevel::Warning) return;
    std::lock_guard<std::mutex> lock{impl::logMutex};
    std::string message = text();
    for (auto &logger: loggers)
    {
        logger->LogWarning(SS(path<< "::" << method << ": " << message));
    }
}
void LogInfo(const std::string&path,const char*method,const std::function<std::string ()> &text)
{
    if (impl::dbusLogLevel > DBusLogLevel::Info) return;
    std::lock_guard<std::mutex> lock{impl::logMutex};
    std::string message = text();;
    for (auto &logger: loggers)
    {
        logger->LogInfo(SS(path<< "::" << method << ": " << message));
    }
}
void LogError(const std::string&path,const char*method,const std::function<std::string ()> &text)
{
    if (impl::dbusLogLevel > DBusLogLevel::Error) return;
    std::lock_guard<std::mutex> lock{impl::logMutex};
    for (auto &logger: loggers)
    {
        logger->LogError(SS(path<< "::" << method << ": " << text()));
    }
}


void LogWarning(const std::string&path,const char*method,const std::string&message)
{
    if (impl::dbusLogLevel > DBusLogLevel::Warning) return;
    std::lock_guard<std::mutex> lock{impl::logMutex};
    for (auto &logger: loggers)
    {
        logger->LogWarning(SS(path<< "::" << method << ": " << message));
    }
}


void LogTrace(const std::string&path,const char*method,const std::string&message)
{
    if (impl::dbusLogLevel > DBusLogLevel::Trace) return;

    std::lock_guard<std::mutex> lock{impl::logMutex};
    for (auto &logger: loggers)
    {
        logger->LogTrace(SS(path<< "::" << method << ": " << message));
    }
}


void SetDBusConsoleLogger()
{
    loggers.clear();
    loggers.push_back(std::make_unique<ConsoleDBusLogger>());
}
void AddDBusConsoleLogger()
{
    loggers.push_back(std::make_unique<ConsoleDBusLogger>());
}
void SetDBusSystemdLogger()
{
    loggers.clear();
    loggers.push_back(std::make_unique<SystemdDBusLogger>());

}
void SetDBusFileLogger(const std::filesystem::path &path)
{
    loggers.clear();
    loggers.push_back(std::make_unique<FileDBusLogger>(path));
}

