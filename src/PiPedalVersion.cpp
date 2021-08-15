#include "pch.h"
#include "config.hpp"
#include "PiPedalVersion.hpp"

#ifdef _WIN32
#include "windows.h"
#include "sstream"
#else
#include <sys/utsname.h>
#endif

using namespace pipedal;

JSON_MAP_BEGIN(PiPedalVersion)
    JSON_MAP_REFERENCE(PiPedalVersion,server)
    JSON_MAP_REFERENCE(PiPedalVersion,serverVersion)
    JSON_MAP_REFERENCE(PiPedalVersion,operatingSystem)
    JSON_MAP_REFERENCE(PiPedalVersion,osVersion)
    JSON_MAP_REFERENCE(PiPedalVersion,debug)
JSON_MAP_END()

PiPedalVersion::PiPedalVersion()
{
    server_ = "PiPedal Server";
    // defined on build command line.
    serverVersion_ = PROJECT_VER;
#ifdef _WIN32 
   OSVERSIONINFOEXA info;
    ZeroMemory(&info, sizeof(OSVERSIONINFOEA));
    info.dwOSVersionInfoSize = sizeof(OSVERSIONINFOEA);

    GetVersionExA(&info);

    ::stringstream s;
    s << info.dwMajorVersion << '.' << info.dwMinorVersion << '.' << info.dwBuildNumber;
    #ifdef _WIN32
    operatingSystem_ = "Windows 32-bit";
    #elif _WIN64
    operatingSystem_ = "Windows 64-bit";
    #endif
    osVersion_ = s.str();
#else
    struct utsname uts;
    uname(&uts);
    operatingSystem_ = uts.sysname;

    osVersion_ = std::string(uts.release) + "/"+ std::string(uts.machine);
#endif

#ifdef DEBUG
    debug_ = true;
#else
    debug_ = false;
#endif
}
