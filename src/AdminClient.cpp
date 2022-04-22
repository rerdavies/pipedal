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

#include "pch.h"
#include "AdminClient.hpp"
#include "PiPedalException.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "Lv2Log.hpp"
#include <sstream>
#include <sys/types.h>
#include <ifaddrs.h>
#include "Ipv6Helpers.hpp"

using namespace pipedal;

const char ADMIN_SOCKET_NAME[] = "/run/pipedal/pipedal_admin";

AdminClient::AdminClient()
{
}

AdminClient::~AdminClient()
{
}

bool AdminClient::CanUseShutdownClient()
{
    return true;
}

bool AdminClient::RequestShutdown(bool restart)
{
    const char *message = restart ? "restart\n" : "shutdown\n";
    return WriteMessage(message);
}

bool AdminClient::WriteMessage(const char *message)
{
    std::lock_guard lock{mutex};

    if (!socket.IsOpen())
    {
        try
        {
            socket.Connect(ADMIN_SOCKET_NAME, "pipedal_d");
        }
        catch (const std::exception &e)
        {
            Lv2Log::error(SS("Failed to connect to PiPedal Admin service."));
            return false;
        }
    }

    socket.Send(message, strlen(message));

    char responseBuffer[1024];

    socket.Receive(responseBuffer, sizeof(responseBuffer));

    int response = atoi(responseBuffer);
    if (response == -2) // throw exception with message
    {
        const char *p = responseBuffer;
        while (*p != ' ' && *p != '\n' && *p != '\0')
        {
            ++p;
        }
        if (*p != 0)
            ++p;
        throw PiPedalStateException(p);
    }
    return response == 0;
}

bool AdminClient::SetJackServerConfiguration(const JackServerSettings &jackServerSettings)
{
    std::stringstream s;
    s << "setJackConfiguration ";

    json_writer writer(s, true);
    writer.write(jackServerSettings);
    s << std::endl;
    return WriteMessage(s.str().c_str());
}

void AdminClient::SetWifiConfig(const WifiConfigSettings &settings)
{
    if (!CanUseShutdownClient())
    {
        throw PiPedalException("Can't perform this operation when debugging.");
    }
    std::stringstream cmd;
    cmd << "WifiConfigSettings ";
    json_writer writer(cmd, true);
    writer.write(settings);
    cmd << '\n';
    bool result = WriteMessage(cmd.str().c_str());
    if (!result)
    { // unexpected. Should throw exception on failure.
        throw PiPedalException("Operation failed.");
    }
}
void AdminClient::SetWifiDirectConfig(const WifiDirectConfigSettings &settings)
{
    if (!CanUseShutdownClient())
    {
        throw PiPedalException("Can't perform this operation when debugging.");
    }
    std::stringstream cmd;
    cmd << "WifiDirectConfigSettings ";
    json_writer writer(cmd, true);
    writer.write(settings);
    cmd << '\n';
    bool result = WriteMessage(cmd.str().c_str());
    if (!result)
    { // unexpected. Should throw exception on failure.
        throw PiPedalException("Operation failed.");
    }
}

void AdminClient::SetGovernorSettings(const std::string &settings)
{
    if (!CanUseShutdownClient())
    {
        throw PiPedalException("Can't use AdminClient when running interactively.");
    }
    std::stringstream cmd;
    cmd << "GovernorSettings ";
    json_writer writer(cmd, true);
    writer.write(settings);
    cmd << '\n';
    bool result = WriteMessage(cmd.str().c_str());
    if (!result)
    { // unexpected. Should throw exception on failure.
        throw PiPedalException("Operation failed.");
    }
}

void AdminClient::MonitorGovernor(const std::string &governor)
{
    if (!CanUseShutdownClient())
    {
        return;
    }
    std::stringstream cmd;
    cmd << "MonitorGovernor ";
    json_writer writer(cmd, true);
    writer.write(governor);
    cmd << '\n';
    bool result = WriteMessage(cmd.str().c_str());
    if (!result)
    { // unexpected. Should throw exception on failure.
        throw PiPedalException("Operation failed.");
    }
}
void AdminClient::UnmonitorGovernor()
{
    if (!CanUseShutdownClient())
    {
        return;
    }
    std::stringstream cmd;
    cmd << "UnmonitorGovernor";
    cmd << '\n';
    bool result = WriteMessage(cmd.str().c_str());
    if (!result)
    { // unexpected. Should throw exception on failure.
        throw PiPedalException("Operation failed.");
    }
}
