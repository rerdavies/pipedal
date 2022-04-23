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


#include "StdErrorCapture.hpp"
#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <filesystem>
#include <sstream>
#include <fstream>
#include <sys/stat.h>

#ifdef __WIN32__

#define dup _dup
#define dup2 _dup2

#endif


using namespace pipedal;

constexpr int STDOUT_HANDLE = 2;
StdErrorCapture::StdErrorCapture()
{
    char filename[] = "/tmp/pipedal.XXXXXX";
    int redirectHandle = mkstemp(filename);
    this->tempFileName = filename;

    this->oldStdout = dup(STDOUT_HANDLE);
    dup2(redirectHandle,STDOUT_HANDLE);
    close(redirectHandle);


}
StdErrorCapture::~StdErrorCapture()
{
    EndCapture();
    if (tempFileName.length() != 0)
    {
        remove(tempFileName.c_str());
    }
}

void StdErrorCapture::EndCapture()
{

    if (this->oldStdout != -1)
    {
        dup2(this->oldStdout,STDOUT_HANDLE);
        close(this->oldStdout);
        this->oldStdout = -1;
        std::ignore = chmod(this->tempFileName.c_str(),0644);
    }
}

std::vector<std::string> StdErrorCapture::GetOutputLines() {
    EndCapture();

    std::vector<std::string> result;

    std::ifstream f(this->tempFileName);

    if (f.is_open())
    {
        char buffer[1024];

        std::string line;
        while (true)
        {
            if (f.eof())
            {
                break;
            }
            std::getline(f,line);
            if (f.fail()) break;
            if (line.length() != 0)
            {
                result.push_back(line);
            }
        }
    }
    return result;
}
