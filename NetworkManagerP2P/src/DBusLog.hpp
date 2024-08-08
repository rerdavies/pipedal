#pragma once
#include <string>
#include <iostream>
#include <functional>
#include <mutex>
#include <filesystem>

enum class DBusLogLevel {
    Trace,
    Debug,
    Info,
    Warning,
    Error,
    None
};

namespace impl {
    extern std::mutex logMutex;
    extern DBusLogLevel dbusLogLevel;
}
extern void SetDBusLogLevel(DBusLogLevel level);
inline DBusLogLevel GetDBusLogLevel() {
    return impl::dbusLogLevel;
}
inline bool IsDBusLoggingEnabled(DBusLogLevel level) {
    return level >= impl::dbusLogLevel;
}

class IDBusLogger {
public:
    IDBusLogger() {}
    virtual ~IDBusLogger() { }
    virtual void LogError(const std::string&message) = 0;
    virtual void LogWarning(const std::string&message) = 0;
    virtual void LogInfo(const std::string&message) = 0;
    virtual void LogDebug(const std::string&message) = 0;
    virtual void LogTrace(const std::string&message) = 0;

};

void SetDBusConsoleLogger();
void AddDBusConsoleLogger();
void SetDBusSystemdLogger();
void SetDBusFileLogger(const std::filesystem::path &path);

extern  void LogError(const std::string&path,const char*method,const std::string&message);
extern  void LogInfo(const std::string&path,const char*method,const std::string&message);
extern  void LogDebug(const std::string&path,const char*method,const std::string&message);
extern  void LogWarning(const std::string&path,const char*method,const std::string&message);
extern  void LogTrace(const std::string&path,const char*method,const std::string&message);

extern  void LogTrace(const std::string&path,const char*method,const std::function<std::string ()> &text);
extern  void LogDebug(const std::string&path,const char*method,const std::function<std::string ()> &text);
extern  void LogInfo(const std::string&path,const char*method,const std::function<std::string ()> &text);
extern  void LogWarning(const std::string&path,const char*method,const std::function<std::string ()> &text);
extern  void LogError(const std::string&path,const char*method,const std::function<std::string ()> &text);



class DBusLog {
protected: 
    DBusLog(const std::string &path) 
    :path(path)
    {
    }
protected:
    void LogError(const char*tag,const std::string&message)
    {
        ::LogError(path,tag,message);
    }
    void LogWarning(const char *tag, const std::string&message)
    {
        ::LogWarning(path,tag,message);
    }
    void LogDebug(const char*tag, const std::string &text)
    {
        ::LogDebug(path,tag,text);
    }
    void LogDebug(const char*tag, const std::function<std::string ()> &text)
    {
        if (impl::dbusLogLevel > DBusLogLevel::Debug) return;
        ::LogDebug(path,tag,text());
    }
    void LogTrace(const char*tag, const std::string &text)
    {
        ::LogTrace(path,tag,text);
    }
    void LogTrace(const char*tag, const std::function<std::string ()> &text)
    {
        if (impl::dbusLogLevel > DBusLogLevel::Trace) return;
        ::LogTrace(path,tag,text());
    }
private:
    std::string path;
};



