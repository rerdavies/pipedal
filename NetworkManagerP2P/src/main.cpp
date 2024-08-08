
#include "DBusLog.hpp"
#include "ss.hpp"
#include "CommandLineParser.hpp"
#include "Install.hpp"
#include <iostream>
#include "Sudo.hpp"
#include <map>
#include <systemd/sd-daemon.h>

using namespace std;

#include "P2PManager.hpp"

using namespace std;

static void PrintHelp()
{
    cout << "nm-pipedal-p2p - P2P Manager for NetworkManager network stack." << endl
         << "Copyright (c) 2024 Robin Davies." << endl
         << endl
         << "Provides P2P connections on LINUX distributions that use NetworkManager." << endl
         << endl
         << "Usage: " << endl
         << endl
         << "   nm-pipedal-p2p [action]" << endl
         << "   where action is one of" << endl
         << endl
         << "   install      Install PiPedal Wi-Fi P2P service." << endl
         << "   uninstall    Uninstall the PiPedal Wi-Fi P2P Services." << endl
         << endl
         << "   If no action is provided, nm-pipedal-p2p runs the service." << endl
         << endl
         << "Options" << endl
         << endl
         << "    --loglevel trace|debug|info|warning|error" << endl
         << "           Set the log level" << endl
         << "    --help, -?" << endl
         << "           show this message." << endl
         << "     --systemd" << endl
         << "           Use systemd logging and notifications." << endl
         << endl;
}

static int RunService(bool underDebugger)
{
    {
        try
        {
            P2PManager::ptr p2pManager = P2PManager::Create();
            if (!p2pManager->Init())
            {
                // config files are not correct; message given; indicate to systemd that we don't want a restart.
                return EXIT_SUCCESS;
            }
            char c;
            p2pManager->Run();

            if (underDebugger)
            {
                char c;
                std::cin.get(c);
                p2pManager->Stop();
            }
            else
            {
                p2pManager->Wait();
            }
            bool reloadRequested = p2pManager->ReloadRequested();
            if (reloadRequested)
            {
                return EXIT_FAILURE; 
            }
            if (p2pManager->TerminatedNormally())
            {
                return EXIT_SUCCESS;
            } else {
                return EXIT_FAILURE;
            }
        }
        catch (const std::exception &e)
        {
            LogError("P2pManager", "main", e.what());
            sleep(1);
            return EXIT_FAILURE;
        }
    }

    return EXIT_SUCCESS;
}

static std::map<std::string, DBusLogLevel> logLevelMap{
    {"trace", DBusLogLevel::Trace},
    {"info", DBusLogLevel::Info},
    {"warning", DBusLogLevel::Warning},
    {"error", DBusLogLevel::Error},
    {"debug", DBusLogLevel::Debug},
};

void SetLogLevel(const std::string &s)
{
    if (logLevelMap.contains(s))
    {
        auto level = logLevelMap[s];
        SetDBusLogLevel(level);
    }
    else
    {
        throw CommandLineException(SS("Invalid log level: " << s));
    }
}
int main(int argc, char **argv)
{
    // const char *destinationName = "org.freedesktop.NetworkManager";
    // const char *objectPath = "/org/freedesktop/NetworkManager";
    bool help = false;
    std::string logLevel = "info";
    std::string logFilePath;
    bool underDebugger = false;
    bool systemd = false;
    bool systemdConsole = false;
    bool console = false;
    bool logFile = false;

    CommandLineParser commandLine;
    commandLine.AddOption("-?", &help);
    commandLine.AddOption("--help", &help);
    commandLine.AddOption("--loglevel", &logLevel);
    commandLine.AddOption("--debug", &underDebugger); // undocumented. wait for getc to shut down, instead of waiting for signals.
    commandLine.AddOption("--systemd", &systemd);
    commandLine.AddOption("--systemd-console", &systemdConsole); // undocumented. log to both systemd and console.
    commandLine.AddOption("--logfile", &logFilePath);
    std::string action;
    try
    {
        commandLine.Parse(argc, argv);
        if (commandLine.Arguments().size() > 1)
        {
            throw CommandLineException("Invalid arguments.");
        }
        if (help)
        {
            PrintHelp();
            return EXIT_SUCCESS;
        }
        if (commandLine.Arguments().size() != 0)
        {
            action = commandLine.Arguments()[0];
        }

        if (!logLevel.empty())
        {
            SetLogLevel(logLevel);
        }

        if (action.empty())
        {
            if (!logFilePath.empty())
            {
                SetDBusFileLogger(logFilePath);
            }
            else if (systemdConsole)
            {
                SetDBusSystemdLogger();
                AddDBusConsoleLogger();
            }
            else if (systemd)
            {
                SetDBusSystemdLogger();
            } else {
                SetDBusConsoleLogger();
            }
            if (systemd)
            {
                sd_notifyf(0, "READY=1\n"
                              "MAINPID=%u",
                           (int)getpid());
            }
            try
            {
                return RunService(underDebugger);
            }
            catch (const exception &e)
            {
                LogError("", "main", e.what());
                return EXIT_SUCCESS; // i.e. don't ask for a restart.
            }
        }
        throw CommandLineException(SS("Invalid action: " << action));
    }
    catch (const std::exception &e)
    {
        if (!logFilePath.empty())
        {
            SetDBusFileLogger(logFilePath);
        }
        if (systemd)
        {
            SetDBusSystemdLogger();
            LogError("", "main", e.what());
            return EXIT_FAILURE;
        }
        else
        {
            cout << SS("Error: " << e.what()) << std::endl;
            return EXIT_FAILURE;
        }
    }
}
