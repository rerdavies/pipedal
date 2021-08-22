#include <iostream>
#include "CommandLineParser.hpp"
#include "PiPedalException.hpp"
#include <filesystem>
#include <stdlib.h>
#include "WriteTemplateFile.hpp"
#include <fstream>
#include "SetWifiConfig.hpp"
#include "SysExec.hpp"
#include <sys/wait.h>

using namespace std;
using namespace pipedal;

#define SERVICE_ACCOUNT_NAME "pipedal_d"
#define SERVICE_GROUP_NAME "pipedal_d"

#define SYSTEMCTL_BIN "/usr/bin/systemctl"
#define GROUPADD_BIN "/usr/sbin/groupadd"
#define USERADD_BIN  "/usr/sbin/useradd"
#define USERMOD_BIN "/usr/sbin/usermod"
#define CHGRP_BIN "/usr/bin/chgrp"
#define CHOWN_BIN "/usr/bin/chown"
#define CHMOD_BIN "/usr/bin/chmod"


#define SERVICE_PATH "/usr/lib/systemd/system"
#define NATIVE_SERVICE "pipedald"
#define SHUTDOWN_SERVICE "pipedalshutdownd"
// #define REACT_SERVICE "pipedal_react"


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
    if (SysExec(SYSTEMCTL_BIN " enable " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to enable the " NATIVE_SERVICE " service.";
    }
    if (SysExec(SYSTEMCTL_BIN " enable " SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to enable the " SHUTDOWN_SERVICE " service.";
    }
}
void DisableService()
{
    if (SysExec(SYSTEMCTL_BIN " disable " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to disable the " NATIVE_SERVICE " service.";
    }
    if (SysExec(SYSTEMCTL_BIN " disable " SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to disable the " SHUTDOWN_SERVICE " service.";
    }
}

void StopService()
{
    if (SysExec(SYSTEMCTL_BIN " stop " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to stop the " NATIVE_SERVICE " service.";
    }
    if (SysExec(SYSTEMCTL_BIN " stop " SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to stop the " SHUTDOWN_SERVICE " service.";
    }
}
void StartService()
{

    if (SysExec(SYSTEMCTL_BIN " start " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to start the " NATIVE_SERVICE " service.");
    }
    if (SysExec(SYSTEMCTL_BIN " start " SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to start the " SHUTDOWN_SERVICE " service.");
    }
}
void RestartService()
{

    StopService();
    StartService();
}

void Install(const std::filesystem::path &programPrefix, const std::string endpointAddress)
{
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

    if (SysExec(GROUPADD_BIN " -f " SERVICE_GROUP_NAME) != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to create service group.");
    }
    if (SysExec(USERADD_BIN " " SERVICE_ACCOUNT_NAME " -g " SERVICE_GROUP_NAME " -M -N -r") != EXIT_SUCCESS)
    {
        //  throw PiPedalException("Failed to create service account.");
    }
    SysExec(USERMOD_BIN " -a -G jack " SERVICE_ACCOUNT_NAME);
    SysExec(USERMOD_BIN " -a -G audio " SERVICE_ACCOUNT_NAME);

    // create and configure /var directory.

    std::filesystem::path varDirectory("/var/pipedal");
    std::filesystem::create_directory(varDirectory);

    {
        std::stringstream s;
        s << CHGRP_BIN " " SERVICE_GROUP_NAME " " << varDirectory.c_str();
        SysExec(s.str().c_str());
    }
    {
        std::stringstream s;
        s << CHOWN_BIN << " " << SERVICE_ACCOUNT_NAME << " " << varDirectory.c_str();
        SysExec(s.str().c_str());
    }

    {
        std::stringstream s;
        s << CHMOD_BIN << " 775 " << varDirectory.c_str();
        SysExec(s.str().c_str());
    }
    {
        std::stringstream s;
        s << CHMOD_BIN << " g+s " << varDirectory.c_str(); // child files/directories inherit ownership.
        SysExec(s.str().c_str());
    }

    // authbind port.

    if (authBindRequired)
    {
        std::filesystem::path portAuthFile = std::filesystem::path("/etc/authbind/byport") / strPort;

        {
            // create it.
            std::ofstream f(portAuthFile);
        }
        {
            // own it.
            std::stringstream s;
            s << CHOWN_BIN << " " SERVICE_ACCOUNT_NAME " " << portAuthFile.c_str();
            SysExec(s.str().c_str());
        }
        {
            // group own it.
            std::stringstream s;
            s << CHGRP_BIN << " " SERVICE_GROUP_NAME " " << portAuthFile.c_str();
            SysExec(s.str().c_str());
        }
        {
            std::stringstream s;
            s << CHMOD_BIN << " 770 " << portAuthFile.c_str();
            SysExec(s.str().c_str());
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

    SysExec(SYSTEMCTL_BIN " daemon-reload");

    cout << "Starting service" << endl;
    RestartService();
    EnableService();

    cout << "Complete" << endl;
}

int SudoExec(char**argv) {
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


int main(int argc, char **argv)
{
    CommandLineParser parser;
    bool install = false;
    bool uninstall = false;
    bool help = false;
    bool stop = false, start = false;
    bool enable = false, disable = false, restart = false;
    bool enable_ap = false, disable_ap = false;
    bool nopkexec = false;
    std::string prefixOption;
    std::string portOption;

    parser.AddOption("-nopkexec", &nopkexec); // strictly a debugging aid.
    parser.AddOption("-install", &install);
    parser.AddOption("-uninstall", &uninstall);
    parser.AddOption("-stop", &stop);
    parser.AddOption("-start", &start);
    parser.AddOption("-enable", &enable);
    parser.AddOption("-disable", &disable);
    parser.AddOption("-restart", &restart);
    parser.AddOption("-h", &help);
    parser.AddOption("--help", &help);
    parser.AddOption("-prefix", &prefixOption);
    parser.AddOption("-port", &portOption);
    parser.AddOption("-enable_ap", &enable_ap);
    parser.AddOption("-disable_ap", &disable_ap);
    try
    {
        parser.Parse(argc, (const char **)argv);

        int actionCount = install + uninstall + stop + start + enable + disable + enable_ap + disable_ap;
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
                     << "Incorrect number of arguments supplied for -enable_ap option." << endl;
                return EXIT_FAILURE;
            }
        }
        else
        {
            if (parser.Arguments().size() != 0)
            {
                cout << "Error: Unexpected argument : " << parser.Arguments()[0] << endl;
                return EXIT_FAILURE;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::string s = e.what();
        cout << "Error: " << s << endl;
        exit(EXIT_FAILURE);
    }

    if (help)
    {
        cout << "pipedalconfig - Post-install configuration for pipdeal" << endl
             << "Copyright (c) 2021 Robin Davies. All rights reserved." << endl
             << endl
             << "Syntax:" << endl
             << "    pipedalconfig [Options...]" << endl
             << "Options: " << endl
             << "    -h --help     Display this message." << endl
             << "    -install      Install services and service accounts." << endl
             << "    -prefix       Either /usr/local or /usr as appropriate for the install" << endl
             << "                  (only valid with the -install option). Usually determined " << endl
             << "                  automatically." << endl
             << "    -uninstall    Remove installed services and service accounts." << endl
             << "    -enable       Start the pipedal service at boot time." << endl
             << "    -disable      Do not start the pipedal service at boot time." << endl
             << "    -stop         Stop the pipedal services." << endl
             << "    -start        Start the pipedal services." << endl
             << "    -restart      Restart the pipedal services." << endl
             << "    -port         Port for the web server (only with -install option)." << endl
             << "                  Either a port (e.g. '81'), or a full endpoint (e.g.: " << endl
             << "                 '127.0.0.1:8080'). If no address is specified, the " << endl
             << "                  web server will listen on 0.0.0.0 (any)." << endl
             << "   -enable_ap <country_code> <ssid> <wep_password> <channel>" << endl
             << "                  Enable the Wi-Fi access point. <country_code> is the 2-letter " << endl
             << "                  ISO-3166 country code for the country in which you are currently" << endl
             << "                  located. The country code determines which channels may be legally" << endl
             << "                  used in (and which features need to be enabled) in order to use a " << endl
             << "                  Wi-Fi channel in your legislative regime." << endl
             << "   -disable_ap    Disabled the Wi-Fi access point." << endl
             << endl
             << "Description:" << endl
             << "    pipedalconfig performs post-install configuration of pipedal." << endl
             << endl;
        exit(EXIT_SUCCESS);
    }
    if (portOption.size() != 0 && !install)
    {
        cout << "Error: -port option can only be specified with the -install option." << endl;
        exit(EXIT_FAILURE);
    }

    auto uid = getuid();
    if (uid != 0 && !nopkexec)
    {
        // re-execute with PKEXEC in order to prompt form SUDO credentials.
        std::vector<char*> args;
        std::string pkexec = "/usr/bin/pkexec"; // staged because "ISO C++ forbids converting a string constant to std::vector<char*>::value_type"(!)
        args.push_back((char*)(pkexec.c_str()));

        std::filesystem::path path = std::filesystem::absolute(argv[0]);
        std::string sPath = path;
        args.push_back((char*)path.c_str());
        for (int arg = 1; arg < argc; ++arg)
        {
            args.push_back((char*)argv[arg]);
        }

        std::string prefixArg; //  lifetime management for the prefix arguments
        std::string prefixOptionArg;

        if (prefixOption.length() == 0)
        {
            std::filesystem::path prefix = std::filesystem::path(argv[0]).parent_path().parent_path();
            prefixOptionArg = "-prefix";
            args.push_back((char*)(prefixOptionArg.c_str()));
            prefixArg = prefix;
            args.push_back((char*)prefixArg.c_str());
        }
        args.push_back(nullptr);

        char**rawArgv = &args[0];
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
            RestartService();
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