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
#define SHUTDOWN_SERVICE "pipedalshutdownd"
#define JACK_SERVICE "jack"

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
    if (sysExec(SYSTEMCTL_BIN " enable " SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to enable the " SHUTDOWN_SERVICE " service.";
    }
}
void DisableService()
{
    if (sysExec(SYSTEMCTL_BIN " disable " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to disable the " NATIVE_SERVICE " service.";
    }
    if (sysExec(SYSTEMCTL_BIN " disable " SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to disable the " SHUTDOWN_SERVICE " service.";
    }
}

void StopService(bool excludeShutdownService = false)
{
    if (sysExec(SYSTEMCTL_BIN " stop " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to stop the " NATIVE_SERVICE " service.";
    }
    if (!excludeShutdownService)
    {
        if (sysExec(SYSTEMCTL_BIN " stop " SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
        {
            cout << "Error: Failed to stop the " SHUTDOWN_SERVICE " service.";
        }
    }
    if (sysExec(SYSTEMCTL_BIN " stop " JACK_SERVICE ".service") != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to stop the " JACK_SERVICE " service.");
    }
}

void StartService(bool excludeShutdownService = false)
{

    silentSysExec("/usr/bin/pulseaudio --kill"); // interferes with Jack audio service startup.
    if (!excludeShutdownService)
    {
        if (sysExec(SYSTEMCTL_BIN " start " SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
        {
            throw PiPedalException("Failed to start the " SHUTDOWN_SERVICE " service.");
        }
    }
    if (sysExec(SYSTEMCTL_BIN " start " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to start the " NATIVE_SERVICE " service.");
    }
    JackServerSettings serverSettings;
    serverSettings.ReadJackConfiguration();
    if (serverSettings.IsValid())
    {
        if (sysExec(SYSTEMCTL_BIN " start " JACK_SERVICE ".service") != EXIT_SUCCESS)
        {
            throw PiPedalException("Failed to start the " JACK_SERVICE " service.");
        }
    }
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

static void RemoveLine(const std::string&path, const std::string lineToRemove)
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
                    throw PiPedalException((std::stringstream() << "Can't open " << path).str());
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
                    throw PiPedalException("Can't write to file.");
                }
                for (auto&line: lines)
                {
                    f << line << endl;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        cout  << "Failed to update " << path << ". " << e.what();
    }
}

const std::string PAM_LINE = "JACK_PROMISCUOUS_SERVER      DEFAULT=" JACK_SERVICE_GROUP_NAME;
void UninstallPamEnv()
{
    RemoveLine(
        "/etc/security/pam_env.conf",
        PAM_LINE
    );
}
void InstallPamEnv()
{
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
                    throw PiPedalException((std::stringstream() << "Can't open " << path).str());
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
                    throw PiPedalException("Can't write to file.");
                }
                for (auto&line: lines)
                {
                    f << line << endl;
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        cout  << "Failed to update " << path << ". " << e.what();
    }
}

void InstallLimits()
{
    if (sysExec(GROUPADD_BIN " -f " AUDIO_SERVICE_GROUP_NAME) != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to create audio service group.");
    }

    std::filesystem::path limitsConfig = "/usr/security/pipedal.conf";

    if (!std::filesystem::exists(limitsConfig))
    {
        ofstream output(limitsConfig);
        output << "# Realtime priority group used by pipedal/jack daemon" << endl;
        output << "@audio   -  rtprio     95" << endl;
        output << "@audio   -  memlock    unlimited" << endl;
    }
}

void UninstallLimits()
{
    std::filesystem::path limitsConfig = "/usr/security/pipedal.conf";

    if (std::filesystem::exists(limitsConfig))
    {
        std::filesystem::remove(limitsConfig);
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
    if (sysExec(GROUPADD_BIN " -f " JACK_SERVICE_GROUP_NAME) != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to create jack service group.");
    }
    if (!userExists(JACK_SERVICE_ACCOUNT_NAME))
    {
        if (sysExec(USERADD_BIN " " JACK_SERVICE_ACCOUNT_NAME " -g " JACK_SERVICE_GROUP_NAME " -M -N -r") != EXIT_SUCCESS)
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

    WriteTemplateFile(map, std::filesystem::path("/etc/pipedal/templateJack.service"), GetServiceFileName(JACK_SERVICE));

    MaybeStartJackService();
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
    StopService();
    DisableService();
    silentSysExec(SYSTEMCTL_BIN " stop jack");
    silentSysExec(SYSTEMCTL_BIN " disable jack");
    std::filesystem::remove("/usr/bin/systemd/system/" SHUTDOWN_SERVICE ".service");
    std::filesystem::remove("/usr/bin/systemd/system/" NATIVE_SERVICE ".service");
    std::filesystem::remove("/usr/bin/systemd/system/" JACK_SERVICE ".service");
    UninstallPamEnv();
    UninstallLimits();
    sysExec(SYSTEMCTL_BIN " daemon-reload");

}


void Install(const std::filesystem::path &programPrefix, const std::string endpointAddress)
{
    if (sysExec(GROUPADD_BIN " -f " AUDIO_SERVICE_GROUP_NAME) != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to create audio service group.");
    }

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
        if (sysExec(USERADD_BIN " " SERVICE_ACCOUNT_NAME " -g " SERVICE_GROUP_NAME " -M -N -r") != EXIT_SUCCESS)
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
            if (!f.is_open()) {
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

    cout << "Creating Systemd file." << endl;

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
            << (programPrefix / "bin/pipedald").string()
            << " /etc/pipedal/config /etc/pipedal/react -port " << endpointAddress << " -systemd -shutdownPort " << shutdownPort;

        map["COMMAND"] = s.str();
    }
    WriteTemplateFile(map, std::filesystem::path("/etc/pipedal/template.service"), GetServiceFileName(NATIVE_SERVICE));

    map["DESCRIPTION"] = "PiPedal Shutdown Service";
    {
        std::stringstream s;

        s
            << (programPrefix / "bin/pipedalshutdownd").string()
            << " -port " << shutdownPort;

        map["COMMAND"] = s.str();
    }
    WriteTemplateFile(map, std::filesystem::path("/etc/pipedal/templateShutdown.service"), GetServiceFileName(SHUTDOWN_SERVICE));

    sysExec(SYSTEMCTL_BIN " daemon-reload");

    cout << "Starting services" << endl;
    RestartService(false);
    EnableService();

    cout << "Complete" << endl;
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
    parser.AddOption("--enable_ap", &enable_ap);
    parser.AddOption("--disable_ap", &disable_ap);
    parser.AddOption("--excludeShutdownService", &excludeShutdownService); // private (unstable) option used by shutdown service.
    try
    {
        parser.Parse(argc, (const char **)argv);

        int actionCount = install + uninstall + stop + start + enable + disable + enable_ap + disable_ap + restart;
        if (actionCount > 1)
        {
            throw PiPedalException("Please provide only one action.");
        }
        if (actionCount == 0)
        {
            help = true;
        }
        if (enable_ap)
        {
            if (parser.Arguments().size() != 4)
            {
                cout << "Error: "
                     << "Incorrect number of arguments supplied for --enable_ap option." << endl;
                helpError = true;
            }
        }
        else
        {
            if (parser.Arguments().size() != 0)
            {
                cout << "Error: Unexpected argument : " << parser.Arguments()[0] << endl;
                helpError = true;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::string s = e.what();
        cout << "Error: " << s << endl;
        helpError = true;
    }

    if (help || helpError)
    {
        if (helpError)
            cout << endl;

        cout << "pipedalconfig - Post-install configuration for pipdeal" << endl
             << "Copyright (c) 2022 Robin Davies. All rights reserved." << endl
             << endl
             << "Syntax:" << endl
             << "    pipedalconfig [Options...]" << endl
             << "Options: " << endl
             << "    -h --help     Display this message." << endl
             << endl
             << "    --install     Install services and service accounts." << endl
             << endl
             << "    --prefix      Either /usr/local or /usr as appropriate for the install" << endl
             << "                  (only valid with the -install option). Usually determined " << endl
             << "                  automatically." << endl
             << endl
             << "    --uninstall   Remove installed services and service accounts." << endl
             << endl
             << "    --enable      Start the pipedal service at boot time." << endl
             << endl
             << "    --disable     Do not start the pipedal service at boot time." << endl
             << endl
             << "    --stop        Stop the pipedal services." << endl
             << endl
             << "    --start       Start the pipedal services." << endl
             << endl
             << "    --restart     Restart the pipedal services." << endl
             << endl
             << "    --port        Port for the web server (only with -install option)." << endl
             << "                  Either a port (e.g. '81'), or a full endpoint (e.g.: " << endl
             << "                 '127.0.0.1:8080'). If no address is specified, the " << endl
             << "                  web server will listen on 0.0.0.0 (any)." << endl
             << endl
             << "   --enable_ap <country_code> <ssid> <wep_password> <channel>" << endl
             << "                  Enable the Wi-Fi access point. <country_code> is the 2-letter " << endl
             << "                  ISO-3166 country code for the country in which you are currently" << endl
             << "                  located. The country code determines which channels may be legally" << endl
             << "                  used in (and which features need to be enabled) in order to use a " << endl
             << "                  Wi-Fi channel in your legislative regime." << endl
             << endl
             << "   --disable_ap    Disabled the Wi-Fi access point." << endl
             << endl
             << "Description:" << endl
             << "    pipedalconfig performs post-install configuration of pipedal." << endl
             << endl;
        exit(helpError ? EXIT_FAILURE : EXIT_SUCCESS);
    }
    if (portOption.size() != 0 && !install)
    {
        cout << "Error: -port option can only be specified with the -install option." << endl;
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
        else if (enable_ap)
        {
            auto argv = parser.Arguments();
            WifiConfigSettings settings;
            settings.valid_ = true;
            settings.enable_ = true;
            settings.countryCode_ = argv[0];
            settings.hotspotName_ = argv[1];
            settings.password_ = argv[2];
            settings.channel_ = argv[3];
            settings.hasPassword_ = settings.password_.length() != 0;

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
        cout << "Error: " << e.what() << endl;
        exit(EXIT_FAILURE);
    }
}