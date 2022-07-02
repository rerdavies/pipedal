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

#include <iostream>
#include <stdlib.h>
#include <unistd.h>
#include "CommandLineParser.hpp"
#include "PiPedalException.hpp"
#include <filesystem>
#include <stdlib.h>
#include "WriteTemplateFile.hpp"
#include <fstream>
#include "SetWifiConfig.hpp"
#include "SysExec.hpp"
#include <sys/wait.h>
#include <pwd.h>
#include "JackServerSettings.hpp"
#include "P2pConfigFiles.hpp"
#include "PrettyPrinter.hpp"
#include "DeviceIdFile.hpp"
#include <uuid/uuid.h>
#include <random>
#include "AudioConfig.hpp"


#if JACK_HOST
#define INSTALL_JACK_SERVICE 1
#else
#define INSTALL_JACK_SERVICE 0
#endif

#define UNINSTALL_JACK_SERVICE 1


#define SS(x) (((std::stringstream &)(std::stringstream() << x)).str())

#define TEMPLATE_PATH(filename) ("/etc/pipedal/config/templates/" filename ".service")

using namespace std;
using namespace pipedal;

#define SERVICE_ACCOUNT_NAME "pipedal_d"
#define SERVICE_GROUP_NAME "pipedal_d"
#define JACK_SERVICE_ACCOUNT_NAME "jack"
#define AUDIO_SERVICE_GROUP_NAME "audio"
#define JACK_SERVICE_GROUP_NAME AUDIO_SERVICE_GROUP_NAME

#define SYSTEMCTL_BIN "/usr/bin/systemctl"
#define GROUPADD_BIN "/usr/sbin/groupadd"
#define USERADD_BIN "/usr/sbin/useradd"
#define USERMOD_BIN "/usr/sbin/usermod"
#define CHGRP_BIN "/usr/bin/chgrp"
#define CHOWN_BIN "/usr/bin/chown"
#define CHMOD_BIN "/usr/bin/chmod"

#define SERVICE_PATH "/usr/lib/systemd/system"
#define NATIVE_SERVICE "pipedald"
#define ADMIN_SERVICE "pipedaladmind"
#define PIPEDAL_P2PD_SERVICE "pipedal_p2pd"
#define JACK_SERVICE "jack"

#define REMOVE_OLD_SERVICE 1 // Grandfathering: whether to remove the old shutdown service (now pipedaladmind)
#define OLD_SHUTDOWN_SERVICE "pipedalshutdownd"

std::filesystem::path GetServiceFileName(const std::string &serviceName)
{
    return std::filesystem::path(SERVICE_PATH) / (serviceName + ".service");
}

std::filesystem::path findOnPath(const std::string &command)
{
    std::string path = getenv("PATH");
    std::vector<std::string> paths;
    size_t t = 0;
    while (t < path.length())
    {
        size_t pos = path.find(':', t);
        if (pos == string::npos)
        {
            pos = path.length();
        }
        std::string thisPath = path.substr(t, pos - t);
        std::filesystem::path path = std::filesystem::path(thisPath) / command;
        if (std::filesystem::exists(path))
        {
            return path;
        }
        t = pos + 1;
    }
    std::stringstream s;
    s << "'" << command << "' is not installed.";
    throw PiPedalException(s.str());
}

void EnableService()
{
    if (sysExec(SYSTEMCTL_BIN " enable " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to enable the " NATIVE_SERVICE " service.";
    }
    if (sysExec(SYSTEMCTL_BIN " enable " ADMIN_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to enable the " ADMIN_SERVICE " service.";
    }
}
void DisableService()
{
    #if INSTALL_JACK_SERVICE || UNINSTALL_JACK_SERVICE
    if (sysExec(SYSTEMCTL_BIN " disable " JACK_SERVICE ".service") != EXIT_SUCCESS)
    {
        #if INSTALL_JACK_SERVICE
        cout << "Error: Failed to disable the " JACK_SERVICE " service.";
        #endif
    }
    #endif
    if (sysExec(SYSTEMCTL_BIN " disable " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to disable the " NATIVE_SERVICE " service.";
    }
    if (sysExec(SYSTEMCTL_BIN " disable " ADMIN_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to disable the " ADMIN_SERVICE " service.";
    }
#if REMOVE_OLD_SERVICE
    if (sysExec(SYSTEMCTL_BIN " disable " OLD_SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to disable the " OLD_SHUTDOWN_SERVICE " service.";
    }
#endif
}

void StopService(bool excludeShutdownService = false)
{
    if (sysExec(SYSTEMCTL_BIN " stop " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to stop the " NATIVE_SERVICE " service.";
    }
    if (!excludeShutdownService)
    {
        if (sysExec(SYSTEMCTL_BIN " stop " ADMIN_SERVICE ".service") != EXIT_SUCCESS)
        {
            cout << "Error: Failed to stop the " ADMIN_SERVICE " service.";
        }
#if REMOVE_OLD_SERVICE
        if (sysExec(SYSTEMCTL_BIN " stop " OLD_SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
        {
            cout << "Error: Failed to stop the " OLD_SHUTDOWN_SERVICE " service.";
        }
#endif
    }
#if INSTALL_JACK_SERVICE || UNINSTALL_JACK_SERVICE
    if (sysExec(SYSTEMCTL_BIN " stop " JACK_SERVICE ".service") != EXIT_SUCCESS)
    {
        #if INSTALL_JACK_SERVICE
        throw PiPedalException("Failed to stop the " JACK_SERVICE " service.");
        #endif
    }
#endif
}

void StartService(bool excludeShutdownService = false)
{

    silentSysExec("/usr/bin/pulseaudio --kill"); // interferes with Jack audio service startup.
    if (!excludeShutdownService)
    {
        if (sysExec(SYSTEMCTL_BIN " start " ADMIN_SERVICE ".service") != EXIT_SUCCESS)
        {
            throw PiPedalException("Failed to start the " ADMIN_SERVICE " service.");
        }
    }
    if (sysExec(SYSTEMCTL_BIN " start " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to start the " NATIVE_SERVICE " service.");
    }
#if INSTALL_JACK_SERVICE
    JackServerSettings serverSettings;
    serverSettings.ReadJackConfiguration();
    if (serverSettings.IsValid())
    {
        if (sysExec(SYSTEMCTL_BIN " start " JACK_SERVICE ".service") != EXIT_SUCCESS)
        {
            throw PiPedalException("Failed to start the " JACK_SERVICE " service.");
        }
    }
#endif
}
void RestartService(bool excludeShutdownService)
{

    StopService(excludeShutdownService);
    StartService(excludeShutdownService);
}

static bool userExists(const char *userName)
{
    struct passwd pwd;
    struct passwd *result;
    char *buf;
    size_t bufsize;
    int s;

    bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufsize == -1)   /* Value was indeterminate */
        bufsize = 16384; /* Should be more than enough */

    buf = (char *)malloc(bufsize);
    if (buf == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    s = getpwnam_r(userName, &pwd, buf, bufsize, &result);
    if (result == NULL)
    {
        return false;
    }
    return true;
}

static void RemoveLine(const std::string &path, const std::string lineToRemove)
{
    std::vector<std::string> lines;
    try
    {
        if (std::filesystem::exists(path))
        {
            {
                ifstream f(path);
                if (!f.is_open())
                {
                    throw PiPedalException(SS("Can't open " << path));
                }
                while (true)
                {
                    std::string line;
                    if (f.eof())
                        break;
                    std::getline(f, line);
                    if (line != lineToRemove)
                    {
                        lines.push_back(line);
                    }
                }
            }

            {
                std::ofstream f(path);
                if (!f.is_open())
                {
                    throw PiPedalException(SS("Can't write to " << path));
                }
                for (auto &line : lines)
                {
                    f << line << "\n";
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        cout << "Failed to update " << path << ". " << e.what();
    }
}

const std::string PAM_LINE = "JACK_PROMISCUOUS_SERVER      DEFAULT=" JACK_SERVICE_GROUP_NAME;
void UninstallPamEnv()
{
#if UNINSTALL_JACK_SERVICE
    RemoveLine(
        "/etc/security/pam_env.conf",
        PAM_LINE);
#endif
}
void InstallPamEnv()
{
#if INSTALL_JACK_SERVICE
    std::string newLine = PAM_LINE;
    std::vector<std::string> lines;
    std::filesystem::path path = "/etc/security/pam_env.conf";
    try
    {
        if (std::filesystem::exists(path))
        {
            {
                ifstream f(path);
                if (!f.is_open())
                {
                    throw PiPedalException(SS("Can't open " << path));
                }
                while (true)
                {
                    std::string line;
                    if (f.eof())
                        break;
                    std::getline(f, line);
                    lines.push_back(line);
                }
            }

            for (auto &line : lines)
            {
                if (line == newLine)
                {
                    return;
                }
            }
            lines.push_back(newLine);

            {
                std::ofstream f(path);
                if (!f.is_open())
                {
                    throw PiPedalException(SS("Can't write to " << path));
                }
                for (auto &line : lines)
                {
                    f << line << "\n";
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        cout << "Failed to update " << path << ". " << e.what();
    }
#endif
}

void InstallLimits()
{
    if (sysExec(GROUPADD_BIN " -f " AUDIO_SERVICE_GROUP_NAME) != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to create audio service group.");
    }

    std::filesystem::path limitsConfig = "/etc/security/limits.d/audio.conf";

    if (!std::filesystem::exists(limitsConfig))
    {
        ofstream output(limitsConfig);
        output << "# Realtime priority group used by pipedal audio (and also by jack)"
               << "\n";
        output << "@audio   -  rtprio     95"
               << "\n";
        output << "@audio   -  memlock    unlimited"
               << "\n";
    }
}


void MaybeStartJackService()
{
    std::filesystem::path drcFile = "/etc/jackdrc";

    if (std::filesystem::exists(drcFile) && std::filesystem::file_size(drcFile) != 0)
    {
        sysExec(SYSTEMCTL_BIN " start jack");
    }
    else
    {
        silentSysExec(SYSTEMCTL_BIN " mask jack");
    }
}
void InstallJackService()
{
    InstallLimits();
    InstallPamEnv();

#if INSTALL_JACK_SERVICE
    if (sysExec(GROUPADD_BIN " -f " JACK_SERVICE_GROUP_NAME) != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to create jack service group.");
    }
    if (!userExists(JACK_SERVICE_ACCOUNT_NAME))
    {
        if (sysExec(USERADD_BIN " " JACK_SERVICE_ACCOUNT_NAME " -g " JACK_SERVICE_GROUP_NAME " -m -N -r") != EXIT_SUCCESS)
        {
            //  throw PiPedalException("Failed to create service account.");
        }
        // lock account for login.
        silentSysExec("passwd -l " JACK_SERVICE_ACCOUNT_NAME);
    }
    // Add to audio groups.
    // sysExec(USERMOD_BIN " -a -G " JACK_SERVICE_GROUP_NAME " " JACK_SERVICE_ACCOUNT_NAME);
    sysExec(USERMOD_BIN " -a -G" AUDIO_SERVICE_GROUP_NAME " " JACK_SERVICE_ACCOUNT_NAME);

// If ONBOARDING, add to bluetooth group.
#ifdef ONBOARDING
    sysExec(USERMOD_BIN " -a -G bluetooth " JACK_SERVICE_ACCOUNT_NAME);
#endif

    // deploy the systemd service file
    std::map<std::string, std::string> map; // nothing to customize.

    WriteTemplateFile(map, GetServiceFileName(JACK_SERVICE));

    MaybeStartJackService();
#endif

}

int SudoExec(char **argv)
{
    int pbPid;
    int returnValue = 0;

    if ((pbPid = fork()) == 0)
    {
        execv(argv[0], argv);
        exit(-1);
    }
    else
    {
        waitpid(pbPid, &returnValue, 0);
        int exitStatus = WEXITSTATUS(returnValue);
        return exitStatus;
    }
}

void Uninstall()
{
    try
    {

        OnWifiUninstall();

        StopService();
        DisableService();
        silentSysExec(SYSTEMCTL_BIN " stop jack");
        silentSysExec(SYSTEMCTL_BIN " disable jack");
        OnWifiUninstall();

        try
        {
            std::filesystem::remove("/usr/bin/systemd/system/" OLD_SHUTDOWN_SERVICE ".service");
        }
        catch (...)
        {
        }

        try
        {
            std::filesystem::remove("/usr/bin/systemd/system/" ADMIN_SERVICE ".service");
        }
        catch (...)
        {
        }
        try
        {
            std::filesystem::remove("/usr/bin/systemd/system/" PIPEDAL_P2PD_SERVICE ".service");
        }
        catch (...)
        {
        }

        try
        {
            std::filesystem::remove("/usr/bin/systemd/system/" NATIVE_SERVICE ".service");
        }
        catch (...)
        {
        }

        try
        {
            std::filesystem::remove("/usr/bin/systemd/system/" JACK_SERVICE ".service");
        }
        catch (...)
        {
        }
        UninstallPamEnv();
        // UninstallLimits();
        sysExec(SYSTEMCTL_BIN " daemon-reload");
    }
    catch (const std::exception &e)
    {
        // We should NEVER get here. But the consequences of failure are high (causes permanent apt installs/uninstall problems), so be safe. 
        cout << "ERROR: Unexpected error while uninstalling pipedal. (" << e.what() << ")" << endl;
    }
    catch (...)
    {
        // We should NEVER get here. But the consequences of failure are high (causes permanent apt installs/uninstall problems), so be safe. 
        cout << "ERROR: Unexpected error while uninstalling pipedal." << endl;
    }
}

std::random_device randdev;

std::string MakeUuid()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char strUid[36 + 1];
    uuid_unparse(uuid, strUid);
    return strUid;
}

static std::string RandomChars(int nchars)
{
    std::stringstream s;

    std::uniform_int_distribution distr(0, 26 + 26 + 10 - 1);

    for (int i = 0; i < nchars; ++i)
    {
        int v = distr(randdev);
        char c;
        if (v < 10)
        {
            c = (char)('0' + v);
        }
        else
        {
            v -= 10;
            if (v < 26)
            {
                c = (char)('a' + v);
            }
            else
            {
                v -= 26;
                c = (char)('A' + v);
            }
        }
        s << c;
    }
    return s.str();
}

static void PrepareDeviceidFile()
{
    if (!std::filesystem::exists(DeviceIdFile::DEVICEID_FILE_NAME))
    {
        DeviceIdFile deviceIdFile;

        deviceIdFile.deviceName = "PiPedal-" + RandomChars(2);
        deviceIdFile.uuid = MakeUuid();
        deviceIdFile.Save();
    }
}

void Install(const std::filesystem::path &programPrefix, const std::string endpointAddress)
{
    if (sysExec(GROUPADD_BIN " -f " AUDIO_SERVICE_GROUP_NAME) != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to create audio service group.");
    }

    PrepareDeviceidFile();
    InstallJackService();
    auto endpos = endpointAddress.find_last_of(':');
    if (endpos == string::npos)
    {
        throw PiPedalException("Invalid endpoint address: " + endpointAddress);
    }
    uint16_t port;
    auto strPort = endpointAddress.substr(endpos + 1);
    try
    {
        auto lport = std::stoul(strPort);
        if (lport == 0 || lport >= std::numeric_limits<uint16_t>::max())
        {
            throw PiPedalException("out of range.");
        }
        port = (uint16_t)lport;
        std::stringstream s;
        s << port;
        strPort = s.str(); // normalized.
    }
    catch (const std::exception &)
    {
        std::stringstream s;
        s << "Invalid port number: " << strPort;
        throw PiPedalException(s.str());
    }

    bool authBindRequired = port < 512;

    // Create and configure service account.

    if (sysExec(GROUPADD_BIN " -f " SERVICE_GROUP_NAME) != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to create service group.");
    }

    if (!userExists(SERVICE_ACCOUNT_NAME))
    {
        if (sysExec(USERADD_BIN " " SERVICE_ACCOUNT_NAME " -g " SERVICE_GROUP_NAME " -m --home /var/pipedal/home -N -r") != EXIT_SUCCESS)
        {
            //  throw PiPedalException("Failed to create service account.");
        }
        // lock account for login.
        silentSysExec("passwd -l " SERVICE_ACCOUNT_NAME);
    }

    // Add to audio groups.
    sysExec(USERMOD_BIN " -a -G  " AUDIO_SERVICE_GROUP_NAME " " SERVICE_ACCOUNT_NAME);

    // create and configure /var directory.

    std::filesystem::path varDirectory("/var/pipedal");
    std::filesystem::create_directory(varDirectory);

    {
        std::stringstream s;
        s << CHGRP_BIN " " SERVICE_GROUP_NAME " " << varDirectory.c_str();
        sysExec(s.str().c_str());
    }
    {
        std::stringstream s;
        s << CHOWN_BIN << " " << SERVICE_ACCOUNT_NAME << " " << varDirectory.c_str();
        sysExec(s.str().c_str());
    }

    {
        std::stringstream s;
        s << CHMOD_BIN << " 775 " << varDirectory.c_str();
        sysExec(s.str().c_str());
    }
    {
        std::stringstream s;
        s << CHMOD_BIN << " g+s " << varDirectory.c_str(); // child files/directories inherit ownership.
        sysExec(s.str().c_str());
    }

    // authbind port.

    if (authBindRequired)
    {
        std::filesystem::create_directories("/etc/authbind/byport");
        std::filesystem::path portAuthFile = std::filesystem::path("/etc/authbind/byport") / strPort;

        {
            // create it.
            std::ofstream f(portAuthFile);
            if (!f.is_open())
            {
                throw PiPedalException("Failed to create " + portAuthFile.string());
            }
        }
        {
            // own it.
            std::stringstream s;
            s << CHOWN_BIN << " " SERVICE_ACCOUNT_NAME " " << portAuthFile.c_str();
            sysExec(s.str().c_str());
        }
        {
            // group own it.
            std::stringstream s;
            s << CHGRP_BIN << " " SERVICE_GROUP_NAME " " << portAuthFile.c_str();
            sysExec(s.str().c_str());
        }
        {
            std::stringstream s;
            s << CHMOD_BIN << " 770 " << portAuthFile.c_str();
            sysExec(s.str().c_str());
        }
    }

    cout << "Creating Systemd file."
         << "\n";

    std::map<string, string> map;

    int shutdownPort = 3147;
    map["DESCRIPTION"] = "PiPedal Web Service";
    {
        std::stringstream s;

        if (authBindRequired)
        {
            s << findOnPath("authbind").string() << " --deep ";
        }
        s
            << (programPrefix / "bin" / NATIVE_SERVICE).string()
            << " /etc/pipedal/config /etc/pipedal/react -port " << endpointAddress << " -systemd";

        map["COMMAND"] = s.str();
    }
    WriteTemplateFile(map, GetServiceFileName(NATIVE_SERVICE));

    map["DESCRIPTION"] = "PiPedal Admin Service";
    {
        std::stringstream s;

        s
            << (programPrefix / "bin" / ADMIN_SERVICE).string()
            << " -port " << shutdownPort;

        map["COMMAND"] = s.str();
    }
    WriteTemplateFile(map, GetServiceFileName(ADMIN_SERVICE));

    // /usr/bin/pipedal_p2pd --config-file /etc/pipedal/config/template_p2pd.conf

    std::string pipedal_p2pd_cmd = SS(
        (programPrefix / "bin" / "pipedal_p2pd").string()
        << " --config-file " << PIPEDAL_P2PD_CONF_PATH);
    map["COMMAND"] = pipedal_p2pd_cmd;

    WriteTemplateFile(map, GetServiceFileName(PIPEDAL_P2PD_SERVICE));

    sysExec(SYSTEMCTL_BIN " daemon-reload");

    cout << "Starting services"
         << "\n";
    RestartService(false);
    EnableService();

    cout << "Complete"
         << "\n";
}

static void PrintHelp()
{
    PrettyPrinter pp;
    pp << "pipedalconfig - Command-line post-install configuration for PiPedal"
       << "\n"
       << "Copyright (c) 2022 Robin Davies. All rights reserved."
       << "\n"
       << "\n"
       << "See https://rerdavies.github.io/pipedal/Documentation.html for "
       << "online documentation."
       << "\n"
       << "\n"
       << "Syntax:"
       << "\n";
    pp << Indent(4)
       << "pipedalconfig [Options...]"
       << "\n"
       << "\n";
    pp.Indent(0)
        << "Options: "
        << "\n\n";
    pp.Indent(20);

    pp
        << HangingIndent() << "    -h --help\t"
        << "Display this message."
        << "\n"
        << "\n"

        << HangingIndent() << "    --install [--port <port#>]\t"
        << "Install or re-install services and service accounts."
        << "\n"
        << "\n"
        << "The --port option controls which port the web server uses."
        << "\n\n"

        << HangingIndent() << "    --uninstall\t"
        << "Remove installed services."
        << "\n"
        << "\n"

        << HangingIndent() << "    --enable\t"
        << "Start the pipedal service at boot time."
        << "\n"
        << "\n"

        << HangingIndent() << "    --disable\tDo not start the pipedal service at boot time."
        << "\n"
        << "\n"
        << HangingIndent() << "    --start\tStart the pipedal services."
        << "\n"
        << "\n"
        << HangingIndent() << "    --stop\tStop the pipedal services."
        << "\n"
        << "\n"
        << HangingIndent() << "    --restart\tRestart the pipedal services."
        << "\n"
        << "\n"

        << HangingIndent() << "    --enable-p2p [<country_code> <ssid> [[<pin>] <channel>] ]\t"
        << "Enable the P2P (Wi-Fi Direct) hotspot."
        << "\n\n"
        << "With no additional arguments, the P2P channel is enabled with most-recent settings."
        << "\n\n"
        << "<country_code> is the 2-letter ISO-3166 country code for "
           "the country you are in. see below for further notes."
        << "\n\n"
        << "<ssid> is the name you see when connecting. "
        << "\n\n"
        << "<pin> is an exactly-eight-digit pin number that you must  "
        << "enter when connecting to the hotspot. If you don't  "
        << "provide a pin, pipedalconfig will generate and "
        << "display a random pin for you. The pin is a "
        << "so-called \"label\" pin, which is the same every "
        << "time you are asked to enter it (unlike a keypad pin "
        << "which changes every time you need to enter it."
        << "\n\n"
        << "Consider attaching a label to the bottom of your device "
        << "so you can can remember the pin if you wan't to connect a new "
        << "device to PiPedal. (It's also available on the Settings page of PiPedal, if you have access to PiPedal UI on another device.)"
        << "\n\n"
        << "For best performance, the channel number should be 1, 6, or 11 (the Wifi Direct \"social\" channels). "
        << "Channel number defaults to 1."
        << "\n\n"

        << HangingIndent() << "   --disable-p2p\tDisabled Wi-Fi Direct access."
        << "\n\n"

        << HangingIndent() << "    --enable-legacy-ap\t <country_code> <ssid> <wep_password> <channel>\tEnable a legacy Wi-Fi access point."
        << "\n\n"
        << "Enable a legacy Wi-Fi access point. \n\n"
        << "country_code is the 2-letter ISO-3166 country code for "
        << "the country you are in. see below for further notes."
        << "\n\n"
        << "See below for an explanation of when you might want to use a legacy Wi-Fi access point instead of Wifi-Direct access."
        << "an explanation of when you might want to use a legacy Access Point instead of "
        << "a P2P (Wi-Fi Direct) connection. Generally, you should prefer a P2p connection "
        << "to an ordinary Hotspot connection."
        << "\n\n"

        << HangingIndent() << "    --disable-legacy-ap\tDisabled the legacy Wi-Fi access point."
        << "\n\n"

        << Indent(0) << "Country codes:"
        << "\n\n"
        << Indent(4)
        << "Country codes are used to determine Wi-Fi-regulatory regime - the "
        << "channels you are legally allowed to use, and the features which must "
        << "be enabled or disabled on the channels you can use. "
        << "\n\n"
        << "Without a country code, Wi-Fi must be restricted to channels 1 through 11 "
        << "with reduced amplitude and feature sets."
        << "\n\n"
        << "For the most part, Wi-Fi country codes are taken from the list of ISO 3661 "
        << "2-letter country codes; although there are a handful of exceptions for  small "
        << "countries and islands. See the Alpha-2 code column of "
        << "\n\n"
        << Indent(8) << "https://en.wikipedia.org/wiki/List_of_ISO_3166_country_codes."
        << "\n\n"

        << Indent(0) << "Legacy Wi-Fi Access Points:"
        << "\n\n"
        << Indent(4)
        << "Some older devices may not be able to connect to PiPedal using Wi-Fi Direct connections. "
           "Old Apple devices, for example, do not support Wi-Fi Direct. In theory, Wi-Fi Direct should "
           "allow legacy Wi-Fi devices to connect to a Wi-Fi Direct access points as if it were an "
           "ordinary Access Point. If this turns out "
           "not to be the case, you can configure PiPedal to provide a legacy Wi-Fi access point instead. "
           "\n\n"
           "Unlike Wi-Fi Direct connections, legacy Access Points will prevent both the connecting device "
           "and the PiPedal host machine from connecting to the Internet over a simultanous Wi-Fi connection Access "
           "Point. On a connecting Android device, you won't be able to use the data connection either when a legacy "
           "Wi-Fi connection is active."
           "\n\n"
           "Wi-Fi Direct connections are "
           "therefore preferrable under almost all circumstances.\n\n";
}

int main(int argc, char **argv)
{
    CommandLineParser parser;
    bool install = false;
    bool uninstall = false;
    bool help = false;
    bool helpError = false;
    bool stop = false, start = false;
    bool enable = false, disable = false, restart = false;
    bool enable_ap = false, disable_ap = false;
    bool enable_p2p = false, disable_p2p = false;
    bool nopkexec = false;
    bool excludeShutdownService = false;
    std::string prefixOption;
    std::string portOption;

    parser.AddOption("--nopkexec", &nopkexec); // strictly a debugging aid.
    parser.AddOption("--install", &install);
    parser.AddOption("--uninstall", &uninstall);
    parser.AddOption("--stop", &stop);
    parser.AddOption("--start", &start);
    parser.AddOption("--enable", &enable);
    parser.AddOption("--disable", &disable);
    parser.AddOption("--restart", &restart);
    parser.AddOption("-h", &help);
    parser.AddOption("--help", &help);
    parser.AddOption("--prefix", &prefixOption);
    parser.AddOption("--port", &portOption);
    parser.AddOption("--enable-ap", &enable_ap);
    parser.AddOption("--disable-ap", &disable_ap);
    parser.AddOption("--enable-p2p", &enable_p2p);
    parser.AddOption("--disable-p2p", &disable_p2p);

    parser.AddOption("--excludeShutdownService", &excludeShutdownService); // private (unstable) option used by shutdown service.
    try
    {
        parser.Parse(argc, (const char **)argv);

        int actionCount = install + uninstall + stop + start + enable + disable + enable_ap + disable_ap + restart + enable_p2p + disable_p2p;
        if (actionCount > 1)
        {
            throw PiPedalException("Please provide only one action.");
        }
        if (actionCount == 0)
        {
            help = true;
        }
        if ((!enable_p2p) && (!enable_ap))
        {
            if (parser.Arguments().size() != 0)
            {
                cout << "Error: Unexpected argument : " << parser.Arguments()[0] << "\n";
                helpError = true;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::string s = e.what();
        cout << "Error: " << s << "\n";
        helpError = true;
    }
    if (helpError)
    {
        cout << "\n";
        return EXIT_FAILURE; // don't scroll the error off the screen.
    }

    if (help)
    {
        PrintHelp();
        return EXIT_SUCCESS;
    }
    if (portOption.size() != 0 && !install)
    {
        cout << "Error: -port option can only be specified with the -install option."
             << "\n";
        exit(EXIT_FAILURE);
    }

    auto uid = getuid();
    if (uid != 0 && !nopkexec)
    {
        // re-execute with PKEXEC in order to prompt for SUDO credentials once only.
        std::vector<char *> args;
        std::string pkexec = "/usr/bin/pkexec"; // staged because "ISO C++ forbids converting a string constant to std::vector<char*>::value_type"(!)
        args.push_back((char *)(pkexec.c_str()));

        std::string sPath = getSelfExePath();
        args.push_back(const_cast<char *>(sPath.c_str()));
        for (int arg = 1; arg < argc; ++arg)
        {
            args.push_back(const_cast<char *>(argv[arg]));
        }

        args.push_back(nullptr);

        char **rawArgv = &args[0];
        return SudoExec(rawArgv);
    }

    try
    {
        if (install)
        {
            std::filesystem::path prefix;
            if (prefixOption.length() != 0)
            {
                prefix = std::filesystem::path(prefixOption);
            }
            else
            {
                prefix = std::filesystem::path(argv[0]).parent_path().parent_path();

                std::filesystem::path pipedalPath = prefix / "bin/pipedald";
                if (!std::filesystem::exists(pipedalPath))
                {
                    std::stringstream s;
                    s << "Can't find pipedald executable at " << pipedalPath << ". Try again using the -prefix option.";
                    throw PiPedalException(s.str());
                }
            }

            if (portOption == "")
            {
                portOption = "80";
            }
            if (portOption.find(':') == string::npos)
            {
                portOption = "0.0.0.0:" + portOption;
            }
            Install(prefix, portOption);
        }
        else if (uninstall)
        {
            Uninstall();
        }
        else if (stop)
        {
            StopService();
        }
        else if (start)
        {
            StartService();
        }
        else if (restart)
        {
            RestartService(excludeShutdownService);
        }
        else if (enable)
        {
            EnableService();
        }
        else if (disable)
        {
            DisableService();
        }
        else if (enable_p2p)
        {
            auto argv = parser.Arguments();
            WifiDirectConfigSettings settings;
            settings.ParseArguments(argv);
            settings.valid_ = true;
            settings.enable_ = true;
            SetWifiDirectConfig(settings);
            RestartService(true); // also have to retart web service so that it gets the correct device name.
        }
        else if (disable_p2p)
        {
            WifiDirectConfigSettings settings;
            settings.valid_ = true;
            settings.enable_ = false;
            SetWifiDirectConfig(settings);
            RestartService(true);
        }
        else if (enable_ap)
        {
            auto argv = parser.Arguments();
            WifiConfigSettings settings;
            settings.ParseArguments(argv);

            SetWifiConfig(settings);
        }
        else if (disable_ap)
        {
            WifiConfigSettings settings;
            settings.valid_ = true;
            settings.enable_ = false;
            SetWifiConfig(settings);
        }
    }
    catch (const std::exception &e)
    {
        cout << "Error: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
}