// Copyright (c) Robin E. R. Davies
// Copyright (c) Gabriel Hernandez
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

#include "JackServerSettings.hpp"
#include <fstream>
#include "PiPedalException.hpp"
#include <string>
#include "unistd.h"
#include <filesystem>
#include "Lv2Log.hpp"
#include "SysExec.hpp"
#include "PiPedalAlsa.hpp"

#define DRC_FILENAME "/etc/jackdrc"

using namespace std;
using namespace pipedal;

JackServerSettings::JackServerSettings()
{
}

static std::vector<std::string> SplitArgs(const char *szBuff)
{
    std::vector<std::string> result;

    while (*szBuff != '\0')
    {
        while (*szBuff == ' ' || *szBuff == '\t' || *szBuff == '\r' || *szBuff == '\n')
            ++szBuff;
        if (*szBuff == 0)
            break;

        std::stringstream s;
        while (*szBuff != ' ' && *szBuff != '\t' && *szBuff != '\0' && *szBuff != '\r' && *szBuff != '\n')
        {
            s.put(*szBuff++);
        }
        result.push_back(s.str());
    }

    return result;
}

static std::string GetJackStringArg(const std::vector<std::string> &args, const std::string &shortOption, const std::string &longOption)
{
    std::string strVal;
    for (size_t i = 1; i < args.size(); ++i)
    {
        auto pos = args[i].rfind(longOption, 0);
        if (pos != std::string::npos)
        {
            if (args[i].length() == longOption.length())
            {
                if (i == args.size() - 1)
                {
                    throw PiPedalException("Can't read Jack configuration.");
                }
                strVal = args[i + 1];
            }
            else
            {
                strVal = args[i].substr(longOption.length());
            }
            break;
        }
        pos = args[i].rfind(shortOption, 0);
        if (pos != std::string::npos)
        {
            if (args[i].length() == shortOption.length())
            {
                if (i == args.size() - 1)
                {
                    throw PiPedalException("Can't read Jack configuration.");
                }
                strVal = args[i + 1];
            }
            else
            {
                strVal = args[i].substr(shortOption.length());
            }
            break;
        }
    }
    return strVal;
}
static std::int32_t GetJackArg(const std::vector<std::string> &args, const std::string &shortOption, const std::string &longOption)
{
    std::string strVal = GetJackStringArg(args, shortOption, longOption);
    try
    {
        unsigned long value = std::stoul(strVal);
        return value;
    }
    catch (const std::exception &)
    {
        throw PiPedalException("Can't read Jack configuration.");
    }
}

static std::string GetAlsaDeviceName(const std::vector<AlsaDeviceInfo> &availableDevices, const std::string &id)
{
    if (id.empty())
        return "";
    std::string name;
    for (const auto &availableDevice : availableDevices)
    {
        if (availableDevice.id_ == id)
        {
            name = availableDevice.name_;
            break;
        }
    }
    if (name.empty())
    {
        auto pos = id.find(":");
        if (pos != std::string::npos)
        {
            name = id.substr(pos + 1);
        }
    }
    return name;
}
void JackServerSettings::FixUpDeviceNames()
{
    if (
        ((!this->alsaInputDevice_.empty()) && this->alsaInputDeviceName_.empty()) ||
        (!this->alsaOutputDevice_.empty()) && this->alsaOutputDeviceName_.empty())
    {

        PiPedalAlsaDevices& alsaDevices = PiPedalAlsaDevices::instance();
        std::vector<AlsaDeviceInfo> availableDevices = alsaDevices.GetAlsaDevices();

        this->alsaInputDeviceName_ = GetAlsaDeviceName(availableDevices, this->alsaInputDevice_);
        this->alsaOutputDeviceName_ = GetAlsaDeviceName(availableDevices, this->alsaOutputDevice_);
    }
}



JSON_MAP_BEGIN(JackServerSettings)
JSON_MAP_REFERENCE(JackServerSettings, valid)
JSON_MAP_REFERENCE(JackServerSettings, isOnboarding)
JSON_MAP_REFERENCE(JackServerSettings, rebootRequired)
JSON_MAP_REFERENCE(JackServerSettings, isJackAudio)
JSON_MAP_REFERENCE(JackServerSettings, alsaDevice) // legacy field
JSON_MAP_REFERENCE(JackServerSettings, alsaInputDevice)
JSON_MAP_REFERENCE(JackServerSettings, alsaOutputDevice)
JSON_MAP_REFERENCE(JackServerSettings, alsaInputDeviceName)
JSON_MAP_REFERENCE(JackServerSettings, alsaOutputDeviceName)
JSON_MAP_REFERENCE(JackServerSettings, sampleRate)
JSON_MAP_REFERENCE(JackServerSettings, bufferSize)
JSON_MAP_REFERENCE(JackServerSettings, numberOfBuffers)
JSON_MAP_END()
