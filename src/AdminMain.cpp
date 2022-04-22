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

// shim file to allow shutdown to be called from a service.
// (Gets suid root permissions during install, since otherwise
// non-interactive services are not allowed to execute)

#include "pch.h"
#include <sys/stat.h>
#include <grp.h>
#include "ss.hpp"
#include "CommandLineParser.hpp"
#include "CpuGovernor.hpp"
#include <iostream>
#include <cstdint>
#include <iostream>
#include <memory>
#include <thread>
#include <stdlib.h>
#include "JackServerSettings.hpp"
#include <cstdlib>
#include "Lv2Log.hpp"
#include "Lv2SystemdLogger.hpp"
#include "WifiConfigSettings.hpp"
#include "SetWifiConfig.hpp"
#include "SysExec.hpp"
#include <filesystem>
#include <thread>
#include <mutex>
#include <condition_variable>
#include <chrono>
#include "UnixSocket.hpp"
#include <signal.h>

using namespace std;
using namespace pipedal;

const char ADMIN_SOCKET_NAME[] = "/run/pipedal/pipedal_admin";

class GovernorMonitorThread
{
public:
    ~GovernorMonitorThread()
    {
        Stop();
    }
    void Start(std::string governor)
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (!pThread)
        {
            this->governor = governor;
            cancelled = false;
            wake = false;
            pThread = std::make_unique<std::thread>([this]() { this->ServiceProc(); });
        }
        else
        {
            this->governor = governor;
            wake = true;

            lock.unlock();
            cv.notify_one();
        }
    }
    void Stop()
    {
        std::unique_lock<std::mutex> lock(mutex);
        if (pThread)
        {
            wake = true;
            cancelled = true;
            lock.unlock();

            cv.notify_one();
            pThread->join();
            pThread.reset();
        }
    }
    void SetGovernor(const std::string &governor)
    {
        {
            std::unique_lock<std::mutex> lock(mutex);
            wake = true;
            this->governor = governor;
        }
        cv.notify_one();
    }

private:
    bool wake = false;
    bool cancelled = false;
    std::string savedGovernor;
    void ServiceProc()
    {
        savedGovernor = pipedal::GetCpuGovernor();
        pipedal::SetCpuGovernor(this->governor);
        while (true)
        {
            bool cancelled;
            std::string governor;
            {
                std::unique_lock<std::mutex> lock(mutex);
                auto timeToWaitFor = std::chrono::system_clock::now() + 5000ms;
                cv.wait_until(
                    lock,
                    timeToWaitFor,
                    [this]() { return wake; });
                wake = false;
                governor = this->governor;
                cancelled = this->cancelled;
            }
            if (cancelled)
            {
                break;
            }
            std::string activeGovernor = pipedal::GetCpuGovernor();
            if (activeGovernor != governor)
            {
                // somebody set it so they must have wanted it.
                // save the value so that we can restore it when done.
                savedGovernor = activeGovernor;

                // but insist on using ours!!
                pipedal::SetCpuGovernor(governor);
            }
        }
        pipedal::SetCpuGovernor(savedGovernor);
    }
    std::unique_ptr<std::thread> pThread;
    std::string governor;
    std::mutex mutex;
    bool canceled;
    std::condition_variable cv;
};

GovernorMonitorThread governorMonitorThread;

void StopGovernorMonitorThread()
{
    governorMonitorThread.Stop();
}
void StartGovernorMonitorThread(const std::string &governor)
{
    governorMonitorThread.Start(governor);
}

static bool startsWith(const std::string &s, const char *text)
{
    if (s.length() < strlen(text))
        return false;
    const char *sp = s.c_str();
    while (*text)
    {
        if (*text++ != *sp++)
            return false;
    }
    return true;
}
static std::vector<std::string> tokenize(std::string value)
{
    std::vector<std::string> result;
    std::stringstream s(value);
    std::string item;
    while (std::getline(s, item, ' '))
    {
        if (item.length() != 0)
            result.push_back(item);
    }
    return result;
}

static void delayedRestartProc()
{
    sleep(1); // give a chance for websocket messages to propagate.
    Lv2Log::error("Delayed Restart");
    std::string pipedalConfigPath = std::filesystem::path(getSelfExePath()).parent_path() / "pipedalconfig";

    std::stringstream s;
    s << pipedalConfigPath.c_str() << " --restart --excludeShutdownService";
    if (::system(s.str().c_str()) != EXIT_SUCCESS)
    {
        Lv2Log::error("Delayed Restart failed.");
    }
}

bool setJackConfiguration(JackServerSettings serverSettings)
{
    bool success = true;

    serverSettings.Write();

    silentSysExec("/usr/bin/systemctl unmask jack");
    silentSysExec("/usr/bin/systemctl enable jack");

    std::thread delayedRestartThread(delayedRestartProc);
    delayedRestartThread.detach();
    return true;
}

class AdminServer
{
private:
    std::stringstream message;

    size_t writePosition = 0;
    std::string response;

    char buffer[4096];

private:
    bool HandleCommand()
    {
        std::string sender;
        size_t length = socket.ReceiveFrom(buffer, sizeof(buffer), &sender);
        if (length > 0 && buffer[length - 1] == '\n')
            --length;
        buffer[length] = 0;

        std::string text{buffer};
        int result = -1;
        std::string command;
        std::string args;
        auto pos = text.find_first_of(' ');
        if (pos == std::string::npos)
        {
            command = text;
        }
        else
        {
            command = text.substr(0, pos);
            args = text.substr(pos + 1);
        }
        if (command == "__quit")
        {
            return false;
        }
        try
        {
            if (command == "UnmonitorGovernor")
            {
                StopGovernorMonitorThread();
                result = 0;
            }
            else if (command == "MonitorGovernor")
            {
                std::stringstream ss(args);
                std::string governor;
                try
                {
                    json_reader reader(ss);
                    reader.read(&governor);
                }
                catch (const std::exception &e)
                {
                    throw PiPedalArgumentException("Invalid arguments.");
                }
                StartGovernorMonitorThread(governor);
                result = 0;
            }
            else if (command == "GovernorSettings")
            {
                std::stringstream ss(args);
                std::string governor;
                try
                {
                    json_reader reader(ss);
                    reader.read(&governor);
                }
                catch (const std::exception &e)
                {
                    throw PiPedalArgumentException("Invalid arguments.");
                }
                governorMonitorThread.SetGovernor(governor);
                result = 0;
            }
            else if (command == "WifiConfigSettings")
            {
                std::stringstream ss(args);
                WifiConfigSettings settings;
                try
                {
                    json_reader reader(ss);
                    reader.read(&settings);
                }
                catch (const std::exception &e)
                {
                    throw PiPedalArgumentException("Invalid arguments.");
                }
                pipedal::SetWifiConfig(settings);
                result = 0;
            }
            else if (command == "WifiDirectConfigSettings")
            {
                std::stringstream ss(args);
                WifiDirectConfigSettings settings;
                try
                {
                    json_reader reader(ss);
                    reader.read(&settings);
                }
                catch (const std::exception &e)
                {
                    throw PiPedalArgumentException("Invalid arguments.");
                }
                pipedal::SetWifiDirectConfig(settings);
                result = 0;
            }
            else if (command == "shutdown")
            {
                result = sysExec("/usr/sbin/shutdown -P now");
            }
            else if (command == "restart")
            {
                result = sysExec("/usr/sbin/shutdown -r now");
            }
            else if (command == "setJackConfiguration")
            {

                std::stringstream input(args);
                JackServerSettings serverSettings;
                json_reader reader(input);

                reader.read(&serverSettings);

                if (input.fail())
                {
                    result = -1;
                }
                else
                {
                    result = setJackConfiguration(serverSettings) ? 0 : -1;
                }
            }
            else
            {
                throw std::invalid_argument(SS("Unknown command:" << command));
            }
        }
        catch (const std::exception &e)
        {

            std::string reply = SS("-2 " << e.what() << "\n");

            try
            {
                socket.SendTo(reply.c_str(), reply.length(), sender);
            }
            catch (const std::exception /*ignored*/)
            {
            }
            return true;
        }

        std::string reply;
        reply = SS(result << "\n");
        try
        {
            socket.SendTo(reply.c_str(), reply.length(), sender);
        }
        catch (const std::exception /*ignored*/)
        {
        }

        return true;
    }

private:
    UnixSocket socket;
    UnixSocket cancelSocket;

    std::unique_ptr<std::thread> serviceThread;
    void ServiceProc()
    {
        try
        {
            while (true)
            {
                if (!HandleCommand())
                {
                    break;
                }
            }
        }
        catch (const std::exception &)
        {
        }
    }

public:
    void Start()
    {
        socket.Bind(ADMIN_SOCKET_NAME, "pipedal_d");
        cancelSocket.Connect(ADMIN_SOCKET_NAME, "pipedal_d");

        serviceThread = std::make_unique<std::thread>([this] {
            this->ServiceProc();
        });
    }

    void Stop()
    {

        if (serviceThread)
        {
            std::string msg{"__quit\n"};
            try
            {
                cancelSocket.Send(msg.c_str(), msg.length());
            }
            catch (const std::exception &ignored)
            {
            };

            serviceThread->join();
            serviceThread = nullptr;
        }
    }
};

static bool signaled = false;

static void sigHandler(int signal)
{
    signaled = true;
}

struct sigaction saInt, oldActionInt;
struct sigaction saTerm, oldActionTerm;

static void setSignalHandlers()
{
    memset(&saInt, 0, sizeof(saInt));
    saInt.sa_handler = sigHandler;
    sigaction(SIGINT, &saInt, &oldActionInt);

    memset(&saTerm, 0, sizeof(saTerm));
    saTerm.sa_handler = sigHandler;
    sigaction(SIGTERM, &saTerm, &oldActionTerm);
}

int main(int argc, char **argv)
{
    Lv2Log::set_logger(MakeLv2SystemdLogger());
    CommandLineParser parser;

    uint16_t port = 0;

    parser.AddOption("-port", &port); // ignored legacy option.

    try
    {
        parser.Parse(argc, (const char **)argv);
        if (parser.Arguments().size() != 0)
        {
            throw PiPedalException("Invalid argument.");
        }
    }
    catch (const std::exception &e)
    {
        cout << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }

    mkdir("/run/pipedal", S_IRWXU | S_IRWXG);

    struct group *group_;
    group_ = getgrnam("pipedal_d");
    if (group_ == nullptr)
    {
        throw UnixSocketException("Group not found.");
    }

    chown("/run/pipedal", -1, group_->gr_gid);

    setSignalHandlers();

    AdminServer server;
    server.Start();

    while (!signaled)
    {
        usleep(300000);
    }
    server.Stop();

    governorMonitorThread.Stop();
    return EXIT_SUCCESS;
}
