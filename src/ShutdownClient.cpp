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

#include "pch.h"
#include "ShutdownClient.hpp"
#include "PiPedalException.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Lv2Log.hpp"
#include <sstream>
#include <sys/types.h>
#include <ifaddrs.h>


using namespace pipedal;



extern uint16_t g_ShutdownPort; // set in main.cpp


bool ShutdownClient::CanUseShutdownClient()
{
    return g_ShutdownPort != 0;
}




bool ShutdownClient::RequestShutdown(bool restart)
{
    const char*message = restart? "restart\n": "shutdown\n";
    return WriteMessage(message);
}

bool ShutdownClient::WriteMessage(const char*message) {

    uint16_t shutdownServerPort = g_ShutdownPort;
    if (g_ShutdownPort == 0)
    {
        throw PiPedalException("No shutdown server available.");
    }

    struct sockaddr_in serv_addr; 

    memset(&serv_addr,0,sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(shutdownServerPort);
    if (inet_pton(AF_INET,"127.0.0.1",&serv_addr.sin_addr) <= 0)
    {
        Lv2Log::error("Failed to parse socket address.");
        return false;
    }
    int sock = socket(AF_INET,SOCK_STREAM,0);
    if (sock < 0) 
    {
        Lv2Log::error("Can't create socket.");
        return false;
    }

    if (connect(sock,(struct sockaddr*)&serv_addr,sizeof(serv_addr)) < 0)
    {
        std::stringstream s;
        s << "Connect to 127.0.0.1:" << shutdownServerPort << " failed.";
        Lv2Log::error(s.str());
        close(sock);
        return false;
    }
    int length = strlen(message);
    const char *p = message;

    bool success = true;
    while (length != 0) {
        auto nWritten = write(sock,p,length);
        if (nWritten < 0)
        {
            success = false;
            Lv2Log::error("Shutdown server write failed.");
            close(sock);
            return false;
        }
        p += nWritten;
        length -= nWritten;
    }
    shutdown(sock, SHUT_WR);
    char responseBuffer[1024];
    ssize_t available = sizeof(responseBuffer);
    char *pWrite = responseBuffer;
    char*pEndOfLine = responseBuffer;

    bool eolFound = false;
    while (!eolFound)
    {
        ssize_t nRead = read(sock,pWrite,available);
        if (nRead == 0) {
            *pWrite = 0;
            break;
        }
        if (nRead <= 0) {
            Lv2Log::error("Failed to read shutdown server response.");
            close(sock);
            return false;
            
        }
        pWrite += nRead;
        available -= nRead;
        if (available == 0)
        {
            Lv2Log::error("Bad response from shutdown server.");
            close(sock);
            return false;
        }

        while (pEndOfLine != pWrite)
        {
            if (*pEndOfLine++ == '\n')
            {
                eolFound = true;
                pEndOfLine[-1] = '\0';
                break;
            }
        }
    }
    int response = atoi(responseBuffer);
    close(sock);
    if (response == -2) // throw exception with message
    {
        const char *p = responseBuffer;
        while (*p != ' ' && *p != '\n' && *p != '\0')
        {
            ++p;
        }
        if (*p != 0) ++p;
        throw PiPedalStateException(p);
    }
    return response == 0;
}

bool ShutdownClient::SetJackServerConfiguration(const JackServerSettings & jackServerSettings)
{
    std::stringstream s;
    s << "setJackConfiguration " 
      << jackServerSettings.GetSampleRate() 
      << " " << jackServerSettings.GetBufferSize()
      << " " << jackServerSettings.GetNumberOfBuffers()
      << "\n";
    
    return WriteMessage(s.str().c_str());
}

bool ShutdownClient::IsOnLocalSubnet(const std::string&fromAddress)
{
    std::string address = fromAddress;
    if (address.size() == 0) return false;
    char lastChar = address[address.size()-1];
    if (address[0] != '[' || lastChar != ']') // i.e. not an ipv6 address;
    {
        size_t pos = address.find_last_of(':');
        if (pos != std::string::npos)
        {
            address = address.substr(0,pos);
        }
    }
    if (address[0] == '[') return false;

    char buf[512];

    struct in_addr inetAddr;

    memset(&inetAddr,0,sizeof(inetAddr));

    if (inet_pton(AF_INET,address.c_str(),&inetAddr) == 0) {
        return false;
    }
    uint32_t remoteAddress = htonl(inetAddr.s_addr);

    struct ifaddrs *ifap = nullptr;
    if (getifaddrs(&ifap) != 0) return false;

    bool result = false;
    for (ifaddrs*p = ifap; p != nullptr; p = p->ifa_next) {
        if (p->ifa_addr->sa_family == AF_INET && p->ifa_addr != nullptr && p->ifa_netmask != nullptr) {  // TODO: Add support for AF_INET6
            uint32_t netmask = htonl(((sockaddr_in*)(p->ifa_netmask))->sin_addr.s_addr);
            uint32_t ifAddr = htonl(((sockaddr_in*)(p->ifa_addr))->sin_addr.s_addr);

            if ((netmask & ifAddr) == (netmask & remoteAddress))
            {
                result = true;
                break;
            }
        }
    }
    freeifaddrs(ifap);
    return result;
}

void ShutdownClient::SetWifiConfig(const WifiConfigSettings & settings)
{
    if (!CanUseShutdownClient())
    {
        throw PiPedalException("Can't use ShutdownClient when running interactively.");
    }
    std::stringstream cmd;
    cmd << "WifiConfigSettings ";
    json_writer writer(cmd,true);
    writer.write(settings);
    cmd << '\n';
    bool result = WriteMessage(cmd.str().c_str());
    if (!result) { // unexpected. Should throw exception on failure.
        throw PiPedalException("Operation failed.");
    }
}
