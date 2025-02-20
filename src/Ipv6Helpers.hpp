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

#pragma once

#include <string>
#include <utility>
#include <boost/asio.hpp>
#include <optional>

namespace pipedal {
    enum class NetworkInterfaceType {
        Loopback,
        Ethernet,
        WiFi,
        WiFiDirect,
        Other,
    };

    NetworkInterfaceType GetNetworkInterfaceType(const char*interfaceName);
    inline NetworkInterfaceType GetNetworkInterfaceType(const std::string&interfaceName)
    {
        return GetNetworkInterfaceType(interfaceName.c_str());
    }

    std::string GetInterfaceIpv4Address(const std::string& interfaceName);


    std::optional<std::string> GetWlanInterfaceName();
    std::optional<std::string> GetWlanIpv4Address();

    std::vector<std::string> GetEthernetIpv4Addresses();



    // bool IsLinkLocalAddress(const std::string &fromAddress);

    std::string GetNonLinkLocalAddress(const std::string& fromAddress);

    bool IsOnLocalSubnet(const std::string&fromAddress);


    bool RemapLinkLocalUrl(
        const boost::asio::ip::address &address,
        const std::string&url,std::string*outputUrl);

    bool ParseHttpAddress(const std::string address,
        std::string *pUser,
        std::string *pServer, 
        int *pPort, 
        int defaultPort);


}