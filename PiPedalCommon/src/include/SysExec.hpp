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

namespace pipedal
{
    // exec a command, returning the actual exit code (unlike execXX() or system() )
    int sysExec(const char *szCommand);

    // execute a command, suppressing output.
    int silentSysExec(const char *szCommand);

    struct SysExecOutput
    {
        int exitCode;
        std::string output;
    };

    SysExecOutput sysExecForOutput(const std::string& command, const std::string&args, bool discardStderr = false);

    using ProcessId = int64_t; // platform-agnostic wrapper for pid_t;
    // Returns a pid or -1 on errror.
    ProcessId sysExecAsync(const std::string &command);

    void sysExecTerminate(ProcessId pid_, int termTimeoutMs = 1000, int killTimeoutMs = 500); // returns the process's exit status.
    int sysExecWait(ProcessId pid);

    // re-execute the current program with current argument, but with sudo.
    int SudoExec(int argc, char **argv);


    std::string getSelfExePath();
}