/*
 * MIT License
 * 
 * Copyright (c) 2022 Robin E. R. Davies
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "DeviceIdFile.hpp"
#include <fstream>
#include <stdexcept>
#include <grp.h>
#include <unistd.h>
#include <sys/stat.h>
#include <tuple>

using namespace pipedal;

using namespace std;


const char DeviceIdFile::DEVICEID_FILE_NAME[] = "/etc/pipedal/config/device_uuid";

void DeviceIdFile::Load()
{
    ifstream f;

    f.open(DEVICEID_FILE_NAME);
    if (!f.is_open())
    {
        throw invalid_argument("Can't open file " + std::string(DEVICEID_FILE_NAME));
    }

    std::getline(f, uuid);
    std::getline(f, deviceName);
}
void DeviceIdFile::Save()
{
    {
        std::string path = DEVICEID_FILE_NAME;
        ofstream f;
        f.open(path, ios_base::trunc);
        if (!f.is_open())
        {
            throw invalid_argument("Can't write to file " + path);
        }

        struct group *group_;
        group_ = getgrnam("pipedal_d");
        if (group_ == nullptr)
        {
            throw logic_error("Group not found: pipedal_d");
        }
        std::ignore = chown(path.c_str(),-1,group_->gr_gid);
        std::ignore = chmod(path.c_str(), S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);


        f << uuid << endl;
        f << deviceName << endl;
    }
}