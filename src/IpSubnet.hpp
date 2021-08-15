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