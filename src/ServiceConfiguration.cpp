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

#include "ServiceConfiguration.hpp"
#include <fstream>
#include <stdexcept>
#include <grp.h>
#include <unistd.h>
#include <sys/stat.h>
#include <tuple>

using namespace pipedal;

using namespace std;
using namespace config_serializer;


const char ServiceConfiguration::DEVICEID_FILE_NAME[] = "/etc/pipedal/config/service.conf";

#define SERIALIZER_ENTRY(MEMBER_NAME) \
    new ConfigSerializer<ServiceConfiguration,decltype(ServiceConfiguration::MEMBER_NAME)>(#MEMBER_NAME, &ServiceConfiguration::MEMBER_NAME, "")


static p2p::autoptr_vector<ConfigSerializerBase<ServiceConfiguration>>
deviceIdSerializers  
{
    SERIALIZER_ENTRY(uuid),
    SERIALIZER_ENTRY(deviceName),
    SERIALIZER_ENTRY(server_port)
};


ServiceConfiguration::ServiceConfiguration()
: base(deviceIdSerializers)
{

}

void ServiceConfiguration::Load()
{
    base::Load(DEVICEID_FILE_NAME,false);
}
void ServiceConfiguration::Save()
{
    base::Save(DEVICEID_FILE_NAME);
    {
        struct group *group_;
        group_ = getgrnam("pipedal_d");
        if (group_ == nullptr)
        {
            throw logic_error("Group not found: pipedal_d");
        }
        std::ignore = chown(DEVICEID_FILE_NAME,-1,group_->gr_gid);
        std::ignore = chmod(DEVICEID_FILE_NAME, S_IWUSR | S_IRUSR | S_IRGRP | S_IROTH);
    }
}