#include "pch.h"
#include "Lv2SystemdLogger.hpp"
#include <systemd/sd-journal.h>

using namespace pipedal;

class Lv2SystemdLogger: public Lv2Logger {
public:
    virtual ~Lv2SystemdLogger() { }
    virtual void onError(const char* message) 
    {
        sd_journal_print(LOG_ERR,"%s",message);
    }
    virtual void onWarning(const char* message) 
    {
        sd_journal_print(LOG_WARNING,"%s",message);

    }
    virtual void onDebug(const char* message) 
    {
        sd_journal_print(LOG_DEBUG,"%s",message);

    }
    virtual void onInfo(const char* message) 
    {
        sd_journal_print(LOG_INFO,"%s",message);
    }
    
};


Lv2Logger *pipedal::MakeLv2SystemdLogger()
{
    return new Lv2SystemdLogger();
}
