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

#include "SysExec.hpp"
#include <sys/wait.h>
#include <filesystem>
#include <stdexcept>
#include <thread>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <sys/wait.h>
#include <linux/limits.h> // for PATH_MAX
#include <vector>
#include <string>
#include "ss.hpp"
#include <stdlib.h>
#include <fcntl.h>

using namespace pipedal;
using namespace std;

#pragma GCC diagnostic ignored "-Wunused-result"

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
    throw std::runtime_error(s.str());
}

int pipedal::silentSysExec(const char *szCommand)
{
    std::stringstream s;
    s << szCommand << " 2>&1";

    FILE *output = popen(s.str().c_str(), "r");
    char buffer[512];
    if (output)
    {
        while (!feof(output))
        {
            fgets(buffer, sizeof(buffer), output);
        }
        return pclose(output);
    }
    return -1;
}
int pipedal::sysExec(const char *szCommand)
{
    char *args = strdup(szCommand);
    int argc;
    std::vector<char *> argv;

    char *p = args;
    while (*p)
    {
        argv.push_back(p);

        while (*p && *p != ' ')
        {
            ++p;
        }
        if (*p)
        {
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
        free((void *)args);
        throw std::runtime_error(SS("Path does not exist: " << execPath << "."));
    }
    argv[0] = (char *)(execPath.c_str());

    char **rawArgv = &argv[0];
    int pbPid;
    int returnValue = 0;

    if ((pbPid = fork()) == 0)
    {
        execv(argv[0], rawArgv);
        exit(-1);
    }
    else
    {
        free((void *)args);
        waitpid(pbPid, &returnValue, 0);
        int exitStatus = WEXITSTATUS(returnValue);
        return exitStatus;
    }
}


std::string pipedal::getSelfExePath() 
{
    char result[PATH_MAX+1];
    ssize_t count = readlink("/proc/self/exe", result, PATH_MAX);
    if (count < 0) throw std::runtime_error("Can't find EXE path.");
    result[count] = 0;
    return result;
}

ProcessId pipedal::sysExecAsync(const std::string&command)
{
    char *args = strdup(command.c_str());
    int argc;
    std::vector<char *> argv;

    char *p = args;
    while (*p)
    {
        argv.push_back(p);

        while (*p && *p != ' ')
        {
            ++p;
        }
        if (*p)
        {
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
        free((void *)args);
        throw std::runtime_error(SS("Path does not exist: " << execPath << "."));
    }
    argv[0] = (char *)(execPath.c_str());

    char **rawArgv = &argv[0];
    int pbPid;

    if ((pbPid = fork()) == 0)
    {
        // redirect output to /dev/null
        int null_fd = open("/dev/null",O_WRONLY);
        if (null_fd == -1)
        {
            std::cerr << "Failed to open /dev/null" << std::endl;
            exit(EXIT_FAILURE);
        }
        dup2(null_fd,STDOUT_FILENO);
        dup2(null_fd,STDERR_FILENO);
        close(null_fd);
        return execv(argv[0], rawArgv);
    }
    else
    {
        free((void *)args);
        return pbPid;
    }

}

void pipedal::sysExecTerminate(ProcessId pid_,int termTimeoutMs,int killTimeoutMs )
{
    pid_t pid = (pid_t)pid_;
    if (pid == -1) return;
    if (kill(pid,SIGTERM) == -1) {
        throw std::runtime_error("Failed to send SIGTERM");
    }
    auto start = std::chrono::steady_clock::now();
    int status;
    do {
        pid_t result;
        result = waitpid(pid,&status,WNOHANG);
        if (result > 0)
        {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now()-start).count() < termTimeoutMs
        );
    start = std::chrono::steady_clock::now();
    if (kill(pid,SIGKILL) == -1) //  some odd async effect?
    {
        return; 
    }
    do {
        pid_t result;
        result = waitpid(pid,&status,WNOHANG);
        if (result > 0)
        {
            return;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
    } while (
        std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now()-start).count() < killTimeoutMs
        );

    throw std::runtime_error("Failed to terminate process.");

}
int pipedal::sysExecWait(ProcessId pid_)
{
    pid_t pid = (pid_t)pid_;
    int returnValue;
    waitpid(pid, &returnValue, 0);
    int exitStatus = WEXITSTATUS(returnValue);
    return exitStatus;

}
