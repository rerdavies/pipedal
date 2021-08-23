// Copyright (c) 2021 Robin Davies
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
#include <iostream>
#include <cstdint>
#include <boost/asio.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <iostream>
#include <boost/bind.hpp>
#include <memory>
#include <stdlib.h>
#include "JackServerSettings.hpp"
#include <cstdlib>
#include "Lv2Log.hpp"
#include "Lv2SystemdLogger.hpp"
#include "WifiConfigSettings.hpp"
#include "SetWifiConfig.hpp"
#include "SysExec.hpp"


using namespace std;
using namespace pipedal;
using namespace boost;
using namespace boost::asio;
using ip::tcp;

const size_t MAX_LENGTH = 128;

static bool startsWith(const std::string&s, const char*text)
{
    if (s.length() < strlen(text)) return false;
    const char*sp = s.c_str();
    while (*text)
    {
        if (*text++ != *sp++) return false;
    }
    return true;
}
static std::vector<std::string> tokenize(std::string value)
{
    std::vector<std::string> result;
    std::stringstream s(value);
    std::string item;
    while (std::getline(s,item,' '))
    {
        if (item.length() != 0)
            result.push_back(item);
    }
    return result;
}

static void CaptureAccessPoint(const std::string gatewayAddress)
{
    
}
static void ReleaseAccessPoint(const std::string gatewayAddress)
{
}


bool setJackConfiguration(uint32_t sampleRate,uint32_t bufferSize,uint32_t numberOfBuffers)
{
    bool success = true;
    JackServerSettings serverSettings(sampleRate,bufferSize,numberOfBuffers);

    try {
        serverSettings.Write();
    } catch (const std::exception &e) {
        std::stringstream s;
         s << "Failed to write jackdrc settings. " << e.what();
        Lv2Log::error(s.str());
        success = false;
    }
    ::system("pulseaudio --kill");
    
    if (::system("systemctl restart jack") != 0)
    {
        Lv2Log::error("Failed to restart jack server.");
        success = false;
    }
    return false;


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
    Reader(io_service &ios)
        : socket(ios)
    {
    }
    static std::shared_ptr<Reader> Create(io_service &ios)
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
            boost::asio::buffer(response.c_str()+ writePosition, response.length()-writePosition),
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
                    cout << "Received command: " << command << endl;
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
    void HandleCommand(const std::string &s)
    {
        int result = -1;
        try {
            if (startsWith(s,"WifiConfigSettings "))
            {
                std::string json = s.substr(strlen("WifiConfigSettings "));
                std::stringstream ss(json);
                WifiConfigSettings settings;
                try {
                    json_reader reader(ss);
                    reader.read(&settings);
                } catch (const std::exception &e)
                {
                    throw PiPedalArgumentException("Invalid arguments.");
                }
                pipedal::SetWifiConfig(settings);
                result = 0;

            } else if (startsWith(s,"release_ap "))
            {
                std::vector<std::string> argv = tokenize(s);
                if (argv.size() == 2) {
                    ReleaseAccessPoint(argv[1]);
                    result = 0;
                } else {
                    throw PiPedalArgumentException("Invalid arguments.");
                }

            } else if (startsWith(s,"capture_ap "))
            {
                std::vector<std::string> argv = tokenize(s);
                if (argv.size() == 2) {
                    CaptureAccessPoint(argv[1]);
                    result = 0;
                } else {
                    throw PiPedalArgumentException("Invalid arguments.");
                }
                

            } else if (s == "shutdown")
            {
                result = SysExec("/usr/sbin/shutdown -P now");
            }
            else if (s == "restart")
            {
                result = SysExec("/usr/sbin/shutdown -r now");
            } else if (startsWith(s,"setJackConfiguration "))
            {
                auto remainder = s.substr(strlen("setJackConfiguration "));

                std::stringstream input(remainder);
                uint32_t sampleRate;
                uint32_t bufferSize;
                uint32_t numberOfBuffers;
                input >> sampleRate >> bufferSize >> numberOfBuffers;
                if (input.fail())
                {
                    result = -1;
                } else {
                    result = setJackConfiguration(sampleRate,bufferSize,numberOfBuffers) ? 0: -1;
                }

            }
        } catch (const std::exception &e)
        {
            std::stringstream t;
            t << "-2 " << e.what();
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
        std::shared_ptr<Reader> reader = Reader::Create(acceptor_.get_io_service());

        // asynchronous accept operation and wait for a new connection.
        acceptor_.async_accept(reader->Socket(),
                               boost::bind(&Server::HandleAccept, this, reader,
                                           boost::asio::placeholders::error));
    }

public:
    //constructor for accepting connection from client
    Server(boost::asio::io_service &io_service, const tcp::endpoint &endpoint)
        : acceptor_(io_service, endpoint)
    {
        StartAccept();
    }
    ~Server() {

    }
};

void runServer(uint16_t port)
{
    asio::ip::tcp::endpoint endpoint(ip::make_address_v4("127.0.0.1"), port);
    asio::io_service ios;

    try {
        Server server(ios,endpoint);
        ios.run();
        cout << "Processing terminated." << endl;
    } catch (const std::exception & e)
    {
        cout << "Error: " << e.what() << endl;
    }
}

int main(int argc, char **argv)
{

    Lv2Log::set_logger(MakeLv2SystemdLogger());
    CommandLineParser parser;

    uint16_t port = 0;
    std::string captureAccessPoint;
    std::string releaseAccessPoint;

    parser.AddOption("-port", &port);
    parser.AddOption("-captureAP",&captureAccessPoint);
    parser.AddOption("-releaseAP",&releaseAccessPoint);

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
    if (captureAccessPoint.length() != 0)
    {
        try{
            CaptureAccessPoint(captureAccessPoint);

        } catch (const std::exception &e)
        {
            std::cerr  << "Error: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "Access point captured." << std::endl;
    } else if (releaseAccessPoint.length() != 0)
    {
        try{
            ReleaseAccessPoint(releaseAccessPoint);
        } catch (const std::exception &e)
        {
            std::cerr  << "Error: " << e.what() << std::endl;
            return EXIT_FAILURE;
        }
        std::cout << "Access point released." << std::endl;

    } else {
        if (port == 0) {
            std::cerr << "Error: port not specified." << std::endl;
            return EXIT_FAILURE;
        }
        runServer(port);
    }
    return EXIT_SUCCESS;
}
