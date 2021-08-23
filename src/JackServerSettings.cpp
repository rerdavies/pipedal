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

#include "Lv2Log.hpp"
#include "JackServerSettings.hpp"
#include <fstream>
#include "PiPedalException.hpp"

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

static uint64_t GetJackArg(const std::vector<std::string> &args, const char *shortOption, const char *longOption)
{
    int pos = -1;
    for (size_t i = 1; i < args.size(); ++i)
    {
        if (args[i] == shortOption || args[i] == longOption)
        {
            pos = i;
            break;
        }
    }
    if (pos == -1 || pos == args.size() - 1)
    {
        throw PiPedalException("Can't read Jack configuration.");
    }
    try
    {
        unsigned long value = std::stoul(args[pos + 1]);
        return value;
    }
    catch (const std::exception &)
    {
        throw PiPedalException("Can't read Jack configuration.");
    }
}
static void SetJackArg(std::vector<std::string> &args, const char *shortOption, const char *longOption,uint64_t value)
{
    int pos = -1;
    for (size_t i = 1; i < args.size(); ++i)
    {
        if (args[i] == shortOption || args[i] == longOption)
        {
            pos = i;
            break;
        }
    }
    if (pos == -1 || pos == args.size() - 1)
    {
        throw PiPedalException("Can't read Jack configuration.");
    }
    stringstream s;
    s << value;
    args[pos+1] = s.str();
    return;
}



void JackServerSettings::ReadJackConfiguration()
{
    this->valid_ = false;

    char firstLine[1024];

    {
        ifstream input(DRC_FILENAME);

        if (!input.is_open())
        {
            Lv2Log::error("Can't read " DRC_FILENAME);
            return;
        }

        while (true)
        {
            if (input.eof())
            {
                Lv2Log::error("Premature end of file in " DRC_FILENAME);
                return;
            }
            input.getline(firstLine, sizeof(firstLine));
            if (firstLine[0] != '#')
                break;
        }
        try {
            std::vector < std::string> argv = SplitArgs(firstLine);
            this->bufferSize_ = GetJackArg(argv, "-p", "-period");
            this->numberOfBuffers_ = (uint32_t)GetJackArg(argv, "-n", "-nperiods");
            this->sampleRate_ = (uint32_t)GetJackArg(argv, "-r", "-rate");
            valid_ = true;
        } catch (std::exception &)
        {
            Lv2Log::error("Can't parse " DRC_FILENAME);
        }
    }
}

void JackServerSettings::Write()
{
    this->valid_ = false;

    std::vector<std::string> precedingLines;
    std::vector < std::string> argv;

    {
        char firstLine[1024];
        ifstream input(DRC_FILENAME);

        if (!input.is_open())
        {
            Lv2Log::error("Can't read " DRC_FILENAME);
            return;
        }

        while (true)
        {
            if (input.eof())
            {
                Lv2Log::error("Premature end of file in " DRC_FILENAME);
                return;
            }
            input.getline(firstLine, sizeof(firstLine));
            if (firstLine[0] != '#')
                break;
            precedingLines.push_back(firstLine);
        }

        // set new values for arguments.
        argv = SplitArgs(firstLine);
        try {
            SetJackArg(argv, "-p", "-period", this->bufferSize_);
            SetJackArg(argv, "-n", "-nperiods", this->numberOfBuffers_);
            SetJackArg(argv, "-r", "-rate",this->sampleRate_);
        } catch (std::exception &)
        {
            Lv2Log::error("Can't parse " DRC_FILENAME);
            return;
        }
    }
    // write to the output.
    try {
        ofstream output(DRC_FILENAME);
        if (!output.is_open())
        {
            throw PiPedalException("Can't write " DRC_FILENAME);
        }
        for (auto line: precedingLines)
        {
            output << line << endl;
        }
        for (size_t i = 0; i < argv.size(); ++i)
        {
            if (i != 0) {
                output << " ";
            }
            output << argv[i];
        }
        output << endl;
    } catch (const std::exception &e) {
        stringstream s;
        s << "jack - " << e.what();
        Lv2Log::error(s.str());
    }
}

JSON_MAP_BEGIN(JackServerSettings)
    JSON_MAP_REFERENCE(JackServerSettings,valid)
    JSON_MAP_REFERENCE(JackServerSettings,rebootRequired)
    JSON_MAP_REFERENCE(JackServerSettings,sampleRate)
    JSON_MAP_REFERENCE(JackServerSettings,bufferSize)
    JSON_MAP_REFERENCE(JackServerSettings,numberOfBuffers)
JSON_MAP_END()

