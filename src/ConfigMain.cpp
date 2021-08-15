#include <iostream>
#include "CommandLineParser.hpp"
#include "PiPedalException.hpp"
#include <filesystem>
#include <stdlib.h>
#include "WriteTemplateFile.hpp"
#include <fstream>

using namespace std;
using namespace pipedal;

#define SERVICE_ACCOUNT_NAME "pipedal_d"
#define SERVICE_GROUP_NAME "pipedal_d"

#define SERVICE_PATH "/usr/lib/systemd/system"
#define NATIVE_SERVICE "pipedald"
#define SHUTDOWN_SERVICE "pipedalshutdownd"
// #define REACT_SERVICE "pipedal_react"

bool exec(std::stringstream &s)
{
    return system(s.str().c_str()) == EXIT_SUCCESS;
}

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
    if (system("systemctl enable " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to enable the " NATIVE_SERVICE " service.";
    }
    if (system("systemctl enable " SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to enable the " SHUTDOWN_SERVICE " service.";
    }
}
void DisableService()
{
    if (system("systemctl disable " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to disable the " NATIVE_SERVICE " service.";
    }
    if (system("systemctl disable " SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to disable the " SHUTDOWN_SERVICE " service.";
    }
}

void StopService()
{
    if (system("systemctl stop " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to stop the " NATIVE_SERVICE " service.";
    }
    if (system("systemctl stop " SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to stop the " SHUTDOWN_SERVICE " service.";
    }
}
void StartService()
{

    if (system("systemctl start " NATIVE_SERVICE ".service") != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to start the " NATIVE_SERVICE " service.");
    }
    if (system("systemctl start " SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
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

    // Create and configur service account.

    if (system("groupadd -f " SERVICE_GROUP_NAME) != EXIT_SUCCESS)
    {
        throw PiPedalException("Failed to create service group.");
    }
    if (system("useradd " SERVICE_ACCOUNT_NAME " -g " SERVICE_GROUP_NAME " -M -N -r") != EXIT_SUCCESS)
    {
        //  throw PiPedalException("Failed to create service account.");
    }
    system("usermod -a -G jack " SERVICE_ACCOUNT_NAME);

    // create and configure /var directory.

    std::filesystem::path varDirectory("/var/pipedal");
    std::filesystem::create_directory(varDirectory);

    {
        std::stringstream s;
        s << "chgrp " SERVICE_GROUP_NAME " " << varDirectory;
        system(s.str().c_str());
    }
    {
        std::stringstream s;
        s << "chown " SERVICE_ACCOUNT_NAME << " " << varDirectory;
        system(s.str().c_str());
    }

    {
        std::stringstream s;
        s << "chmod  775 " << varDirectory;
        system(s.str().c_str());
    }
    {
        std::stringstream s;
        s << "chmod g+s " << varDirectory; // child files/directories inherit ownership.
        system(s.str().c_str());
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
            s << "chown " SERVICE_ACCOUNT_NAME " " << portAuthFile;
            exec(s);
        }
        {
            // group own it.
            std::stringstream s;
            s << "chgrp " SERVICE_GROUP_NAME " " << portAuthFile;
            exec(s);
        }
        {
            std::stringstream s;
            s << "chmod 770 " << portAuthFile;
            exec(s);
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



    system("systemctl daemon-reload");

    cout << "Starting service" << endl;
    RestartService();
    EnableService();

    cout << "Complete" << endl;
}

int main(int argc, char **argv)
{
    CommandLineParser parser;
    bool install = false;
    bool uninstall = false;
    bool help = false;
    bool stop = false, start = false;
    bool enable = false, disable = false, restart = false;
    std::string prefixOption;
    std::string portOption;

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
    try
    {
        parser.Parse(argc, (const char **)argv);

        int actionCount = install + uninstall + stop + start + enable + disable;
        if (actionCount > 1)
        {
            throw PiPedalException("Please provide only one action.");
        }
        if (actionCount == 0)
        {
            help = true;
        }
    }
    catch (const std::exception &e)
    {
        std::string s = e.what();
        cout << "Error: " << s << endl;
        exit(EXIT_FAILURE);
    }

    if (help || parser.Arguments().size() != 0)
    {
        cout << "pipedalconfig - Post-install configuration for pipdeal" << endl
             << "Copyright (c) 2021 Robin Davies. All rights reserved." << endl
             << endl
             << "Syntax:" << endl
             << "    pipedalconfig [Options...]" << endl
             << "Options: " << endl
             << "    -install      Install services and service accounts." << endl
             << "    -uninstall    Remove installed services and service accounts." << endl
             << "    -enable       Start the pipedal at boot time." << endl
             << "    -disable      Do not start the pipedal at boot time." << endl
             << "    -h --help     Display this message." << endl
             << "    -stop         Stop the pipedal services." << endl
             << "    -start        Start the pipedal services." << endl
             << "    -restart      Restart the pipedal services." << endl
             << "    -prefix       Either /usr/local or /usr as appropriate for the install" << endl
             << "                  (only valid with the -install option)." << endl
             << "    -port         Port for the web server (only with -install option)." << endl
             << "                  Either a port (e.g. '81'), or a full endpoint (e.g.: " << endl
             << "                 '127.0.0.1:8080'). If no address is specified, the " << endl
             << "                  web server will listen on 0.0.0.0 (any)." << endl
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

    try
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
        if (install)
        {
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
    }
    catch (const std::exception &e)
    {
        cout << "Error: " << e.what() << endl;
        exit(EXIT_FAILURE);
    }
}