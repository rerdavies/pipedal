/*
 * MIT License
 * 
 * Copyright (c) 2022 Robin E. R. Davies
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "UnixSocket.hpp"
#include <atomic>
#include <filesystem>
#include <grp.h>
#include <stdexcept>
#include "gsl/util"

#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/un.h>
#include <unistd.h>
#include <fcntl.h>
#include "ss.hpp"

using namespace std;
namespace pipedal
{
    namespace detail
    {
        class UnixSocketData
        {
        public:
            UnixSocketData()
            {
                memset(&localAddress, 0, sizeof(localAddress));
                memset(&remoteAddress, 0, sizeof(remoteAddress));
                localAddress.sun_family = AF_UNIX;
                remoteAddress.sun_family = AF_UNIX;
            }
            int socket = -1;
            struct sockaddr_un localAddress;
            struct sockaddr_un remoteAddress;
        };
    }
}
using namespace pipedal;
using namespace pipedal::detail;
using namespace gsl;


bool UnixSocket::IsOpen() const { return data->socket != -1; }

UnixSocketException::UnixSocketException(int errno_)
    : what_(strerror(errno_)), errno_(errno_)
{
}

UnixSocketException::UnixSocketException(const std::string what_, int errno_)
    : what_(what_ + " " + strerror(errno_) + ")"), errno_(errno_)
{
}
UnixSocketException::UnixSocketException(const std::string what_)
    : what_(what_), errno_(0)
{
}

void UnixSocket::Bind(const std::string &socketName, const std::string &permissionGroup)
{
    if (socketName.length() >= sizeof(data->localAddress.sun_path) - 1)
    {
        throw UnixSocketException("Socket name too long.", E2BIG);
    }
    int s = socket(PF_UNIX, SOCK_DGRAM, 0);
    auto socket_cleanup = finally([&]() {
        if (s != -1)
        {
            close(s);
            s = -1;
        }
    });
    strcpy(data->localAddress.sun_path, socketName.c_str());

    for (int retry = 0; /**/; ++retry)
    {
        int err = bind(s, (struct sockaddr *)&data->localAddress, sizeof(data->localAddress));

        if (err < 0)
        {
            if (retry != 2)
            {
                unlink(data->localAddress.sun_path);
                continue;
            }
            throw UnixSocketException("Can't bind socket " + socketName, errno);
        }
        else
        {
            break;
        }
    }

    if (permissionGroup != "")
    {
        gid_t gid;
        struct group *group_;
        group_ = getgrnam(permissionGroup.c_str());
        if (group_ == nullptr)
        {
            throw UnixSocketException("Group not found.");
        }
        gid = group_->gr_gid;

        chown(data->localAddress.sun_path, -1, gid);
        chmod(data->localAddress.sun_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    }

    data->socket = s;
    s = -1;
}

UnixSocket::UnixSocket()
{
    data = new UnixSocketData();
}

void UnixSocket::Close()
{
    if (data->socket != -1)
    {
        close(data->socket);
        unlink(data->localAddress.sun_path);
    }
}
UnixSocket::~UnixSocket()
{
    Close();
    delete data;
}

static std::atomic<uint64_t> instanceId;

void UnixSocket::Connect(const std::string &remoteSocketName, const std::string &permissionGroup)
{
    
    const std::string localSocketName =
        SS("/tmp/ppadmin-" << getpid() << "-" << (instanceId++));

    if (localSocketName.length() >= sizeof(data->localAddress.sun_path) - 1)
    {
        throw UnixSocketException("Local socket path is too long. ", E2BIG);
    }
    strcpy(data->localAddress.sun_path, localSocketName.c_str());

    int s = socket(PF_UNIX, SOCK_DGRAM, 0);
    auto socket_cleanup = finally([&]() {
        if (s != -1)
        {
            close(s);
            s = -1;
            unlink(data->localAddress.sun_path);
        }
    });

    int e = bind(s, (sockaddr *)&data->localAddress, sizeof(data->localAddress));

    if (permissionGroup.length() != 0)
    {
        struct group *g = getgrnam(permissionGroup.c_str());
        if (g == nullptr)
        {
            throw UnixSocketException("Group not found");
        }
        gid_t gid = g->gr_gid;

        chown(data->localAddress.sun_path, -1, gid);
        chmod(data->localAddress.sun_path, S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP);
    }

    strcpy(data->remoteAddress.sun_path, remoteSocketName.c_str());
    e = connect(s, (sockaddr *)&data->remoteAddress, sizeof(data->remoteAddress));
    if (e < 0)
    {
        throw UnixSocketException("Can't connect to socket " + remoteSocketName, errno);
    }
    data->socket = s;
    s = -1;
}

size_t UnixSocket::Send(const void *buffer, size_t length)
{
    ssize_t size = send(data->socket, buffer, length, 0);
    if (size < 0)
    {
        throw UnixSocketException(errno);
    }
    return (size_t)size;
}

size_t UnixSocket::Receive(void *buffer, size_t length)
{
    ssize_t received = recv(data->socket, buffer, length, 0);
    if (received < 0)
    {
        throw UnixSocketException(errno);
    }
    return (size_t)received;
}

size_t UnixSocket::ReceiveFrom(void *buffer, size_t length, std::string *pFrom)
{
    sockaddr_un sockAddr;
    socklen_t sockAddrLen = sizeof(sockAddr);

    ssize_t received = recvfrom(data->socket, buffer, length, 0, (sockaddr *)&sockAddr, &sockAddrLen);
    if (received < 0)
    {
        throw UnixSocketException(errno);
    }
    *pFrom = std::string(sockAddr.sun_path);
    return (size_t)received;
}

size_t UnixSocket::SendTo(const void *buffer, size_t length, const std::string &toAddress)
{
    sockaddr_un sockAddr;
    if (toAddress.length() >= sizeof(sockAddr.sun_path) - 1)
    {
        throw UnixSocketException("Address too long.", E2BIG);
    }
    strcpy(sockAddr.sun_path, toAddress.c_str());
    sockAddr.sun_family = AF_UNIX;

    ssize_t size = sendto(data->socket, buffer, length, 0, (sockaddr *)&sockAddr, sizeof(sockaddr_un));
    if (size < 0)
    {
        throw UnixSocketException(errno);
    }
    return (size_t)size;
}
