// Copyright (c) 2024Robin Davies
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

#include "AdminInstallUpdate.hpp"
#include <fstream>
#include <stdexcept>
#include "ss.hpp"
#include <vector>
#include <unistd.h>
#include <fcntl.h>
#include "Lv2Log.hpp"
#include <string.h>
#include <sys/prctl.h>
#include <sys/wait.h>
#include <signal.h>

using namespace pipedal;
namespace fs = std::filesystem;

static fs::path ROOT_INSTALL_DIRECTORY = "/var/pipedal/updates";
static fs::path INSTALLER_LOG_FILE_PATH = ROOT_INSTALL_DIRECTORY / "install.log";

static fs::path UPDATE_DIRECTORY = ROOT_INSTALL_DIRECTORY / "downloads";

static void removeSignalHandler(int signal)
{
    struct sigaction sa;
    sa.sa_handler = SIG_DFL;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(signal, &sa, NULL);
}

static int exec(const std::string &command)
{
    std::string buffer = command;


    std::vector<char *> argv;
    char *p = const_cast<char *>(buffer.c_str());
    while (*p)
    {
        char *start = p;
        while (*p != ' ' && *p != '\0')
        {
            ++p;
        }
        argv.push_back(start);
        while (*p == ' ')
        {
            *p++ = '\0';
        }
    }
    argv.push_back(nullptr);

    pid_t pid;

    if ((pid = fork()) == 0)
    {
        execv(argv[0], (char *const *)argv.data());
        write(1,"!\n",2);
        exit(EXIT_FAILURE);
    }
    else
    {
        
        if (pid == -1)
        {
            write(1,"*",1);
            perror("execv");
            return EXIT_FAILURE;
        }
        int returnValue;
        waitpid(pid, &returnValue, 0);


        int exitStatus = WEXITSTATUS(returnValue);
        return exitStatus;
    }
}

void updateLog(const std::string &message)
{
    write(1,message.c_str(),message.length());
    write(1,"\n",1);
}
void pipedal::AdminInstallUpdate(const std::filesystem::path path)
{

    try
    {
        fs:create_directories(ROOT_INSTALL_DIRECTORY);
        Lv2Log::info(SS("Installing " << path));

        if (!path.string().starts_with(UPDATE_DIRECTORY.string()))
        {
            throw std::runtime_error("Update file path is incorrect.");
        }
        if (!fs::exists(path))
        {
            throw std::runtime_error("File does not exist.");
        }

        std::stringstream ssCmd;
        ssCmd << "/usr/bin/apt-get -q -y install " << path.string();

        std::string cmd = ssCmd.str();

        // direct standard outputs to a log file.
        close(0);
        close(1);
        close(2);

        int fd = open(INSTALLER_LOG_FILE_PATH.c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
        dup2(fd, 1); // to stdout
        dup2(fd, 2); // to stderr
        close(fd);
        fd = -1;

        // disconnect std input from parent's stdin.
        int fdNull = open("/dev/null", O_RDONLY, 0644);
        dup2(fdNull, 0);
        close(fdNull);

        updateLog("**Stopping pipedald");

        exec("/usr/bin/systemctl stop pipedald");

        updateLog("** Installing update");
        int retcode = exec(cmd);


        // in case we didn't actually update for some reason.
        updateLog("** Starting pipedald");
        exec("/usr/bin/systemctl start pipedald");
   }
    catch (const std::exception &e)
    {
        updateLog(e.what());
    }
}