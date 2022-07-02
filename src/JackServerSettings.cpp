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

#include "JackServerSettings.hpp"
#include <fstream>
#include "PiPedalException.hpp"
#include <string>
#include "unistd.h"
#include <filesystem>
#include "Lv2Log.hpp"
#include "SysExec.hpp"

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

static std::string GetJackStringArg(const std::vector<std::string> &args, const std::string&shortOption, const std::string&longOption)
{
    std::string strVal;
    for (size_t i = 1; i < args.size(); ++i)
    {
        auto pos = args[i].rfind(longOption,0);
        if (pos != std::string::npos) {
            if (args[i].length() == longOption.length())
            {
                if (i == args.size()-1) {
                    throw PiPedalException("Can't read Jack configuration.");                    
                }
                strVal = args[i+1];
            } else {
                strVal = args[i].substr(longOption.length());
            }
            break;
        }
        pos = args[i].rfind(shortOption,0);
        if (pos != std::string::npos) {
            if (args[i].length() == shortOption.length())
            {
                if (i == args.size()-1) {
                    throw PiPedalException("Can't read Jack configuration.");                    
                }
                strVal = args[i+1];
            } else {
                strVal = args[i].substr(shortOption.length());
            }
            break;
        }
    }
    return strVal;
}
static std::int32_t GetJackArg(const std::vector<std::string> &args, const std::string&shortOption, const std::string&longOption)
{
    std::string strVal = GetJackStringArg(args,shortOption,longOption);
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


void JackServerSettings::ReadJackConfiguration()
{
    this->valid_ = false;

    std::string lastLine;
    {
        ifstream input(DRC_FILENAME);

        if (!input.is_open())
        {
            return;
        }

        while (true)
        {
            std::string line;
            std::getline(input,line);
            if (line.length() != 0) {
                lastLine = line;
            }
            if (input.eof()) {
                break;
            }
        }
        try
        {
            std::vector<std::string> argv = SplitArgs(lastLine.c_str());
            for (auto i = argv.begin(); i != argv.end(); ++i)
            {
                if ((*i) == "-dalsa") {
                    argv.erase(i);
                    break;
                }
            }
            this->bufferSize_ = GetJackArg(argv, "-p", "--period");
            this->numberOfBuffers_ = (uint32_t)GetJackArg(argv, "-n", "--nperiods");
            this->sampleRate_ = (uint32_t)GetJackArg(argv, "-r", "--rate");
            this->alsaDevice_ = GetJackStringArg(argv,"-d", "--device");
            this->valid_ = true;
        }
        catch (std::exception &)
        {
            //Lv2Log::error("Can't parse " DRC_FILENAME);
        }
    }
}

void JackServerSettings::Write()
{
    #if JACK_HOST
    this->valid_ = false;

    std::vector<std::string> precedingLines;
    std::vector<std::string> argv;
    std::string lastLine;

    if (std::filesystem::exists(DRC_FILENAME)) {
        ifstream input(DRC_FILENAME);

        if (!input.is_open())
        {
            return;
        }

        while (true)
        {
            if (input.eof())
            {
                break;
            }
            std::getline(input,lastLine);
            precedingLines.push_back(lastLine);
            if (input.eof())
            {
                break;
            }
        }
        // erase blank lines at the end.
        while (precedingLines.size() != 0 && precedingLines[precedingLines.size()-1] == "")
        {
            precedingLines.erase(precedingLines.begin()+precedingLines.size()-1);
        }
        // erase the last line, which should contain the command invocation.
        if (precedingLines.size() != 0)
        {
            precedingLines.erase(precedingLines.begin()+precedingLines.size()-1);
        }
     }
    // write to the output.
    try
    {
        ofstream output(DRC_FILENAME);
        if (!output.is_open())
        {
            throw PiPedalException("Can't write " DRC_FILENAME);
        }
        if (precedingLines.size() == 0)
        {
            // jack1 incantation for promiscuous servers.
            output << "#!/bin/sh" <<endl;
            output << "export JACK_PROMISCUOUS_SERVER=audio" << endl;
            output << "export JACK_NO_AUDIO_RESERVATION=1" << endl;
            output << "umask 0" << endl;
        }
        for (auto line : precedingLines)
        {
            output << line << endl;
        }
        // the style used by qjackctl. :-/
        // Lower to -P70 in order to allow the USB soft-irq to run at higher priority than JACK (it runs at 80).
        output << "/usr/bin/jackd "
            << "-R -P70 --silent"
            << " -dalsa -d" << this->alsaDevice_ 
            << " -r" << this->sampleRate_ 
            << " -p" << this->bufferSize_ 
            << " -n" << this->numberOfBuffers_ << " -Xseq" 
            << endl;
        if (silentSysExec("/usr/bin/chmod 755 " DRC_FILENAME) != 0)
        {
            Lv2Log::error("Failed to set permissions on /etc/jackdrc");
        }
    }
    catch (const std::exception &e)
    {
    }
    #else 
        throw PiPedalStateException("JACK_HOST not enabled at compile time.");
    #endif
}

JSON_MAP_BEGIN(JackServerSettings)
JSON_MAP_REFERENCE(JackServerSettings, valid)
JSON_MAP_REFERENCE(JackServerSettings, rebootRequired)
JSON_MAP_REFERENCE(JackServerSettings, alsaDevice)
JSON_MAP_REFERENCE(JackServerSettings, sampleRate)
JSON_MAP_REFERENCE(JackServerSettings, bufferSize)
JSON_MAP_REFERENCE(JackServerSettings, numberOfBuffers)
JSON_MAP_END()
