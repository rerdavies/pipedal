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

#include "Ipv6Helpers.hpp"
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "ss.hpp"
#include <sys/types.h>
#include <ifaddrs.h>
#include <memory.h>
#include <exception>
#include <netdb.h>

using namespace pipedal;

std::string pipedal::GetNonLinkLocalAddress(const std::string fromAddress);

static bool IsIpv4MappedAddress(const struct in6_addr &inetAddr6)
{
    return IN6_IS_ADDR_V4MAPPED(&inetAddr6) != 0;
}
static uint32_t GetIpv4MappedAddress(const struct in6_addr &inetAddr6)
{
    if (!IsIpv4MappedAddress(inetAddr6))
    {
        return 0;
    }
    uint8_t *p = (uint8_t *)&inetAddr6;
    return htonl(*(uint32_t *)(p + 12));
}
static bool ipv6NetmaskCompare(const struct in6_addr &left, const struct in6_addr &right, const struct in6_addr &mask)
{
    uint8_t *pLeft = (uint8_t *)&left;
    uint8_t *pRight = (uint8_t *)&right;
    uint8_t *pMask = (uint8_t *)&mask;
    for (int i = 0; i < 16; ++i)
    {
        if ((pLeft[i] & pMask[i]) != (pRight[i] & pMask[i]))
        {
            return false;
        }
    }
    return true;
}

static bool IsIpv4OnLocalSubnet(uint32_t ipv4Addres)
{
    bool result = false;
    struct ifaddrs *ifap = nullptr;
    if (getifaddrs(&ifap) != 0)
        return false;

    for (ifaddrs *p = ifap; p != nullptr; p = p->ifa_next)
    {
        if (p->ifa_addr->sa_family == AF_INET && p->ifa_addr != nullptr && p->ifa_netmask != nullptr)
        { // TODO: Add support for AF_INET6
            uint32_t netmask = htonl(((sockaddr_in *)(p->ifa_netmask))->sin_addr.s_addr);
            uint32_t ifAddr = htonl(((sockaddr_in *)(p->ifa_addr))->sin_addr.s_addr);

            if ((netmask & ifAddr) == (netmask & ipv4Addres))
            {
                result = true;
                break;
            }
        }
    }
    freeifaddrs(ifap);
    return result;
}

bool pipedal::ParseHttpAddress(const std::string address,
                               std::string *pUser,
                               std::string *pServer,
                               int *pPort,
                               int defaultPort)
{
    // strip user.
    auto start = address.find_first_of('@');
    if (start != std::string::npos)
    {
        if (pUser)
        {
            *pUser = address.substr(0, start);
        }
        start = start + 1;
    }
    else
    {
        start = 0;
        if (pUser)
        {
            *pUser = "";
        }
    }
    // find the port address.
    int port = address.length();
    while (port > 0 && address[port - 1] != ':')
    {
        if (address[port - 1] != ']')
        {
            port = address.length();
            break;
        }
        --port;
    }
    int portNumber = defaultPort;
    if (port < address.length() && address[port] == ':')
    {
        const char *p = address.c_str() + port + 1;
        if (*p)
        {
            portNumber = 0;
            while (*p)
            {
                if (*p >= '0' && *p <= '9')
                {
                    portNumber = portNumber * 10 + *p - '0';
                }
                else
                {
                    return false;
                }
            }
        }
    }
    if (pPort)
    {
        *pPort = portNumber;
    }
    if (pServer)
    {
        *pServer = address.substr(start, port - start);
    }
    return true;
}
static std::string StripPortNumber(const std::string &fromAddress)
{
    std::string address = fromAddress;

    if (address.size() == 0)
        return fromAddress;

    char lastChar = address[address.size() - 1];
    size_t pos = address.find_last_of(':');

    // if ipv6, make sure we found an actual port address.
    size_t posBracket = address.find_last_of(']');
    if (posBracket != std::string::npos && pos != std::string::npos)
    {
        if (posBracket > pos)
        {
            pos = std::string::npos;
        }
    }
    if (pos != std::string::npos)
    {
        address = address.substr(0, pos);
    }
    return address;
}

bool pipedal::IsOnLocalSubnet(const std::string &fromAddress)
{
    std::string address = StripPortNumber(fromAddress);
    if (address.length() == 0)
        return false;

    bool result = false;
    if (address[0] != '[')
    {
        // ipv4
        struct in_addr inetAddr;
        memset(&inetAddr, 0, sizeof(inetAddr));

        if (inet_pton(AF_INET, address.c_str(), &inetAddr) == 1)
        {
            uint32_t remoteAddress = htonl(inetAddr.s_addr);

            result = IsIpv4OnLocalSubnet(remoteAddress);
        }
    }
    else
    {
        // ipv6
        if (address[0] != '[' || address[address.length() - 1] != ']')
            return false;
        address = address.substr(1, address.length() - 2);
        // strip scope if neccessary.
        auto pos = address.find_last_of('%');
        if (pos != std::string::npos)
        {
            address = address.substr(0, pos);
        }

        struct in6_addr inetAddr6;
        memset(&inetAddr6, 0, sizeof(inetAddr6));
        if (inet_pton(AF_INET6, address.c_str(), &inetAddr6) == 1)
        {
            int32_t remoteAddress = -1;
            // cases:
            //   [::FFFF:ipv4 address]
            //   [FE80:: ]   link local.
            //   [FEC0:: ]   site local.
            //   others?
            if (IsIpv4MappedAddress(inetAddr6))
            {
                uint32_t remoteAddress = GetIpv4MappedAddress(inetAddr6);
                return IsIpv4OnLocalSubnet(remoteAddress);
            }
            if (IN6_IS_ADDR_LINKLOCAL(&inetAddr6))
            {
                return true;
            }
            if (IN6_IS_ADDR_SITELOCAL(&inetAddr6))
            {
                return true;
            }
            return false;
#if JUNK
            // if the address is global, we have to reject it.
            // no point in going further.
            struct ifaddrs *ifap = nullptr;
            if (getifaddrs(&ifap) != 0)
                return false;

            const int SITE_LOCAL_SCOPE = 5;

            for (ifaddrs *p = ifap; p != nullptr; p = p->ifa_next)
            {
                if (p->ifa_addr->sa_family == AF_INET6 && p->ifa_addr != nullptr && p->ifa_netmask != nullptr)
                {

                    struct sockaddr_in6 *pAddr = (struct sockaddr_in6 *)(p->ifa_addr);
                    struct sockaddr_in6 *pNetMask = (struct sockaddr_in6 *)(p->ifa_netmask);
                    if (pAddr->sin6_scope_id <= SITE_LOCAL_SCOPE)
                    {
                        if (ipv6NetmaskCompare(inetAddr6, pAddr->sin6_addr, pNetMask->sin6_addr))
                        {
                            result = true;
                            break;
                        }
                    }
                }
            }
            freeifaddrs(ifap);
#endif
        }
    }
    return result;
}

static std::string GetInterfaceForIp4Address(uint32_t ipv4Address)
{
    struct ifaddrs *ifap = nullptr;
    if (getifaddrs(&ifap) != 0)
        return "";
    std::string result;
    for (ifaddrs *p = ifap; p != nullptr; p = p->ifa_next)
    {
        if (p->ifa_addr->sa_family == AF_INET && p->ifa_addr != nullptr && p->ifa_netmask != nullptr)
        { // TODO: Add support for AF_INET6
            uint32_t netmask = htonl(((sockaddr_in *)(p->ifa_netmask))->sin_addr.s_addr);
            uint32_t ifAddr = htonl(((sockaddr_in *)(p->ifa_addr))->sin_addr.s_addr);

            if ((netmask & ifAddr) == (netmask & ipv4Address))
            {
                result = p->ifa_name;
                break;
            }
        }
    }
    freeifaddrs(ifap);
    return result;
}

static std::string GetInterfaceForIp6Address(const in6_addr inetAddr6)
{
    struct ifaddrs *ifap = nullptr;
    if (getifaddrs(&ifap) != 0)
        return "";
    std::string result;
    for (ifaddrs *p = ifap; p != nullptr; p = p->ifa_next)
    {
        if (p->ifa_addr->sa_family == AF_INET6 && p->ifa_addr != nullptr && p->ifa_netmask != nullptr)
        { // TODO: Add support for AF_INET6
            struct sockaddr_in6 *pAddr = (struct sockaddr_in6 *)(p->ifa_addr);
            struct sockaddr_in6 *pNetMask = (struct sockaddr_in6 *)(p->ifa_netmask);

            if (ipv6NetmaskCompare(inetAddr6, pAddr->sin6_addr, pNetMask->sin6_addr))
            {
                result = p->ifa_name;
                break;
            }
        }
    }
    freeifaddrs(ifap);
    return result;
}

static std::string GetNonLinkLocalAddressForInterface(const std::string &name)
{
    struct ifaddrs *ifap = nullptr;
    if (getifaddrs(&ifap) != 0)
        return "";
    std::string result = "notlinklocal.error";
    for (ifaddrs *p = ifap; p != nullptr; p = p->ifa_next)
    {
        if (p->ifa_addr->sa_family == AF_INET6 && p->ifa_addr != nullptr && p->ifa_netmask != nullptr)
        { // TODO: Add support for AF_INET6
            struct sockaddr_in6 *pAddr = (struct sockaddr_in6 *)(p->ifa_addr);
            if (!IN6_IS_ADDR_LINKLOCAL(&(pAddr->sin6_addr)))
            {
                if (name == p->ifa_name)
                {
                    const int BUFSIZE = 128;
                    char host[BUFSIZE];
                    if (getnameinfo(p->ifa_addr, sizeof(struct sockaddr_in6), host, BUFSIZE, NULL, 0, NI_NUMERICHOST) == 0)
                    {
                        host[BUFSIZE - 1] = '\0';

                        // trim the interface spec if present
                        for (char *p = host; *p != 0; ++p)
                        {
                            if (*p == '%')
                            {
                                *p = '\0';
                                break;
                            }
                        }
                        result = SS('[' << host << ']');
                        break;
                    }
                }
            }
        }
    }
    freeifaddrs(ifap);
    return result;
}
static std::string GetNonLinkLocalAddressForIp4Interface(const std::string &name)
{
    struct ifaddrs *ifap = nullptr;
    if (getifaddrs(&ifap) != 0)
        return "";
    std::string result = "";
    for (ifaddrs *p = ifap; p != nullptr; p = p->ifa_next)
    {
        if (p->ifa_addr->sa_family == AF_INET && p->ifa_addr != nullptr && p->ifa_netmask != nullptr)
        { // TODO: Add support for AF_INET6
            struct sockaddr_in *pAddr = (struct sockaddr_in *)(p->ifa_addr);
            {
                if (name == p->ifa_name)
                {
                    const int BUFSIZE = 128;
                    char host[BUFSIZE];
                    if (getnameinfo(p->ifa_addr, sizeof(struct sockaddr_in), host, BUFSIZE, NULL, 0, NI_NUMERICHOST) == 0)
                    {
                        host[BUFSIZE - 1] = '\0';

                        result = host;
                        break;
                    }
                }
            }
        }
    }
    freeifaddrs(ifap);
    return result;
}

bool pipedal::IsLinkLocalAddress(const std::string fromAddress)
{
    std::string address = fromAddress;
    std::string result;
    if (address[0] != '[')
    {
        return false;
    }
    else
    {
        // ipv6
        if (address[0] != '[' || address[address.length() - 1] != ']')
            throw std::invalid_argument("Bad address.");
        address = address.substr(1, address.length() - 2);

        struct in6_addr inetAddr6;
        memset(&inetAddr6, 0, sizeof(inetAddr6));
        if (inet_pton(AF_INET6, address.c_str(), &inetAddr6) == 1)
        {
            if (IN6_IS_ADDR_LINKLOCAL(&inetAddr6))
            {
                return true;
            }
        }
    }
    return false;
}

std::string pipedal::GetNonLinkLocalAddress(const std::string fromAddress)
{
    std::string address = fromAddress;
    std::string result;
    if (address[0] != '[')
    {
        return address;
    }
    else
    {
        // ipv6
        if (address[0] != '[' || address[address.length() - 1] != ']')
            throw std::invalid_argument("Bad address.");
        address = address.substr(1, address.length() - 2);

        auto nPos = address.find('%') ;
        if (nPos != std::string::npos)
        {
            std::string ifName = address.substr(nPos+1);
            return GetNonLinkLocalAddressForInterface(ifName);

        }
        struct in6_addr inetAddr6;
        memset(&inetAddr6, 0, sizeof(inetAddr6));
        if (inet_pton(AF_INET6, address.c_str(), &inetAddr6) == 1)
        {
            int32_t remoteAddress = -1;
            // cases:
            //   [::FFFF:ipv4 address]
            //   [FE80:: ]   link local.
            //   [FEC0:: ]   site local.
            //   others?
            if (IN6_IS_ADDR_V4MAPPED(&inetAddr6))
            {
                int8_t *pAddr = (int8_t *)&inetAddr6;
                uint32_t remoteAddress = htonl(*(int32_t *)(pAddr + 12));
                std::string interfaceName = GetInterfaceForIp4Address(remoteAddress);
                result = GetNonLinkLocalAddressForIp4Interface(interfaceName);
            }
            else
            {


                std::string interfaceName = GetInterfaceForIp6Address(inetAddr6);
                result =  GetNonLinkLocalAddressForInterface(interfaceName);
            }
        }
    }
    if (result == "")
    {
        result = GetNonLinkLocalAddressForInterface("");
    }
    return result;
}

std::string pipedal::GetInterfaceIpv4Address(const std::string& interfaceName)
{
    return GetNonLinkLocalAddressForIp4Interface(interfaceName);
}
