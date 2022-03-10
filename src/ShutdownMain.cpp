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
#include "CommandLineParser.hpp"
#include "CpuGovernor.hpp"
#include <iostream>
#include <cstdint>
#include <boost/asio.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <iostream>
#include <boost/bind.hpp>
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

using namespace std;
using namespace pipedal;
using namespace boost;
using namespace boost::asio;
using ip::tcp;

const size_t MAX_LENGTH = 128;

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
            pThread = std::make_unique<std::thread>([this]()
                                                    { this->ServiceProc(); });
        } else {
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
                    [this]()
                    { return wake; });
                wake = false;
                governor = this->governor;
                cancelled = this->cancelled;
            }
            if (cancelled) {
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
void StartGovernorMonitorThread(const std::string&governor)
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

class Reader : public std::enable_shared_from_this<Reader>
{
private:
    tcp::socket socket;
    int readAvailable = MAX_LENGTH;
    char data[MAX_LENGTH];
    char *writePointer;
    ssize_t writeLength;
    std::stringstream message;

    size_t writePosition = 0;
    std::string response;

public:
    Reader(asio::io_service &ios)
        : socket(ios)
    {
    }
    static std::shared_ptr<Reader> Create(asio::io_service &ios)
    {
        return std::make_shared<Reader>(ios);
    }

    tcp::socket &Socket() { return socket; }

    void Start()
    {
        ReadSome();
    }

private:
    void ReadSome()
    {
        socket.async_read_some(
            boost::asio::buffer(data, readAvailable),
            boost::bind(&Reader::HandleRead,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }
    void WriteSome()
    {
        if (writePosition == response.size())
        {
            socket.close();
            return;
        }
        socket.async_write_some(
            boost::asio::buffer(response.c_str() + writePosition, response.length() - writePosition),
            boost::bind(&Reader::HandleWrite,
                        shared_from_this(),
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred));
    }

private:
    void HandleWrite(const boost::system::error_code &err, size_t bytes_transferred)
    {
        writePosition += bytes_transferred;
        WriteSome();
    }
    void HandleRead(const boost::system::error_code &err, size_t bytes_transferred)
    {
        if (!err)
        {
            for (size_t i = 0; i < bytes_transferred; ++i)
            {
                char c = data[i];
                if (c == '\n')
                {
                    std::string command = message.str();
                    cout << command << endl;
                    HandleCommand(command);
                    socket.close();
                    return;
                }
                else
                {
                    message.put(data[i]);
                }
            }
            ReadSome();
        }
        else
        {
            socket.close();
        }
    }
    void HandleCommand(const std::string &text)
    {
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
            else if (command == "WifiConfigSettings ")
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
            std::stringstream t;
            t << "-2 " << e.what() << "\n";
            this->response = t.str();
            this->writePosition = 0;
            WriteSome();
            return;
        }

        std::stringstream t;
        t << result << "\n";
        this->response = t.str();
        this->writePosition = 0;
        WriteSome();
    }
};

class Server
{
private:
    tcp::acceptor acceptor_;
    boost::asio::io_service &io_service;

    void HandleAccept(std::shared_ptr<Reader> connection, const boost::system::error_code &err)
    {
        if (!err)
        {
            connection->Start();
        }
        StartAccept();
    }

    void StartAccept()
    {
        // socket
        std::shared_ptr<Reader> reader = Reader::Create(io_service);

        // asynchronous accept operation and wait for a new connection.
        acceptor_.async_accept(reader->Socket(),
                               boost::bind(&Server::HandleAccept, this, reader,
                                           boost::asio::placeholders::error));
    }

public:
    // constructor for accepting connection from client
    Server(boost::asio::io_service &io_service, const tcp::endpoint &endpoint)
        : acceptor_(io_service, endpoint),
          io_service(io_service)

    {
        StartAccept();
    }
    ~Server()
    {
    }
};

void runServer(uint16_t port)
{
    asio::ip::tcp::endpoint endpoint(ip::make_address_v4("127.0.0.1"), port);
    asio::io_service ios;

    try
    {
        Server server(ios, endpoint);
        ios.run();
        cout << "Processing terminated." << endl;
    }
    catch (const std::exception &e)
    {
        cout << "Error: " << e.what() << endl;
    }
}

int main(int argc, char **argv)
{
    Lv2Log::set_logger(MakeLv2SystemdLogger());
    CommandLineParser parser;

    uint16_t port = 0;

    parser.AddOption("-port", &port);

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
    if (port == 0)
    {
        std::cerr << "Error: port not specified." << std::endl;
        return EXIT_FAILURE;
    }
    runServer(port);
    governorMonitorThread.Stop();
    return EXIT_SUCCESS;
}
