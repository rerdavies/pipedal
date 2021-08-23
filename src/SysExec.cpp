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
#include "SysExec.hpp"
#include <sys/wait.h>
#include <filesystem>
#include "PiPedalException.hpp"
#include <string.h>
#include <unistd.h>

using namespace pipedal;
using namespace std;


// find on path, but ONLY /usr/bin and /usr/sbin

static std::filesystem::path findOnSystemPath(const std::string &command)
{
    if (command.length() != 0 && command[0] == '/')
    {
        return command;
    }
    std::string path = "/usr/bin:/usr/sbin";
    std::vector<std::string> paths;
    size_t t = 0;
    while (t < path.length())
    {
        size_t pos = path.find(':', t);
        if (pos == string::npos)
        {
            pos = path.length();
        }
        std::string thisPath = path.substr(t, pos - t);
        std::filesystem::path path = std::filesystem::path(thisPath) / command;
        if (std::filesystem::exists(path))
        {
            return path;
        }
        t = pos + 1;
    }
    std::stringstream s;
    s << "'" << command << "' is not installed.";
    throw PiPedalException(s.str());
}



int pipedal::SysExec(const char*szCommand) {
    char*args = strdup(szCommand);
    int argc;
    std::vector<char*> argv;

    char *p = args;
    while (*p) {
        argv.push_back(p);

        while (*p && *p != ' ') {
            ++p;
        }
        if (*p) {
            *p++ = '\0';
            while (*p && *p == ' ') 
            {
                ++p;
            }
        }
    }
    argv.push_back(nullptr);

    std::filesystem::path execPath = argv[0];
    if (execPath.is_relative())
    {
        execPath = findOnSystemPath(execPath);
    }
    if (!std::filesystem::exists(execPath))
    {
        throw PiPedalException( SS("Path does not exist: " << execPath << "."));
    }
    argv[0] = (char*)(execPath.c_str());
    
    
    char**rawArgv = &argv[0];
    int pbPid;
    int returnValue = 0;

    if ((pbPid = fork()) == 0)
    {
        execv(argv[0], rawArgv);
        exit(-1);
    }
    else
    {
        free((void*)args);
        waitpid(pbPid, &returnValue, 0);
        int exitStatus = WEXITSTATUS(returnValue);
        return exitStatus;
    }
}