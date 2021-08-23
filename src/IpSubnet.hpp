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

#pragma once
#include "PiPedalException.hpp"

namespace pipedal {

    class IpSubnet
    {
        bool isValid = false;
        uint32_t ipAddress = 0;
        uint32_t mask = 0;
        IpSubnet()
        {

        }
    public:
        IPSubnet() {
            isValid = false;
        }
        static IpSubment Parse(std::string&text)
        {
            int pos = text.find('/');
            if (pos == std::string::npos)
            {
                throw PiPedalArgumentException("Not a valid netmask.");
            }
            std::string ipText = text.substr(0,npos);
            std::string netmaskText = text.substr(npos+1);

            uint32_t bitMask;
            try {
                unsigned long t = std::stoul(netmaskText);
                if (t > 32 || t == 0) {
                    throw PiPedalStateException("Invalid netmask.");
                }
                bitMask  ((uint32_t)0xFFFF) << (int)t;
            } catch (const std::exception &e)
            {
                throw PiPedalArgumentException("Invalid netmask.");
            }
            

        }
    };
}