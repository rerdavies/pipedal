// Copyright (c) 2022-2024 Robin Davies
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
#include "PiPedalConfiguration.hpp"
#include "json.hpp"
#include "ss.hpp"
#include "ServiceConfiguration.hpp"

using namespace pipedal;
namespace fs  = std::filesystem;

void PiPedalConfiguration::Load(const std::filesystem::path &path, const std::filesystem::path &webRoot)
{
    docRoot_ = path;
    webRoot_ = webRoot;

    // load installer defaults.
    {
        std::filesystem::path configPath = path / "config.json";
        std::ifstream f(configPath);
        if (!f.is_open())
        {
            std::stringstream s;
            s << "Unable to open " << configPath;
            throw PiPedalStateException(s.str());
        }
        json_reader reader(f);
        reader.read(this);
    }
    // web port comes from service.conf
    ServiceConfiguration serviceConfiguration;

    serviceConfiguration.Load(fs::path(this->local_storage_path_) / "config" / "service.conf");
    this->socketServerAddress_ = SS("0.0.0.0:" << serviceConfiguration.server_port);

    // load any user-made settings
    {
        std::filesystem::path userPath = std::filesystem::path(this->local_storage_path_) / "config" / "config.json";
        if (std::filesystem::exists(userPath))
        {
            std::ifstream f(userPath);
            json_reader reader(f);
            reader.read(this);
        }
    }
}

uint16_t PiPedalConfiguration::GetSocketServerPort() const
{
    try
    {
        size_t pos = this->socketServerAddress_.find_last_of(':');
        if (pos == std::string::npos)
        {
            throw std::exception();
        }
        std::string strPort = socketServerAddress_.substr(pos + 1);
        unsigned long port = std::stoul(strPort);
        if (port == 0)
        {
            throw std::exception();
        }
        if (port > std::numeric_limits<uint16_t>::max())
        {
            throw std::exception();
        }
        return (uint16_t)port;
    }
    catch (const std::exception &)
    {
        std::stringstream s;
        s << "Invalid port number in '" << this->socketServerAddress_ << "'.";
        throw PiPedalArgumentException(s.str());
    }
}

JSON_MAP_BEGIN(PiPedalConfiguration)
JSON_MAP_REFERENCE(PiPedalConfiguration, lv2_path)
JSON_MAP_REFERENCE(PiPedalConfiguration, local_storage_path)
JSON_MAP_REFERENCE(PiPedalConfiguration, mlock)
JSON_MAP_REFERENCE(PiPedalConfiguration, reactServerAddresses)
JSON_MAP_REFERENCE(PiPedalConfiguration, socketServerAddress)
JSON_MAP_REFERENCE(PiPedalConfiguration, threads)
JSON_MAP_REFERENCE(PiPedalConfiguration, logLevel)
JSON_MAP_REFERENCE(PiPedalConfiguration, logHttpRequests)
JSON_MAP_REFERENCE(PiPedalConfiguration, maxUploadSize)
JSON_MAP_REFERENCE(PiPedalConfiguration, accessPointGateway)
JSON_MAP_REFERENCE(PiPedalConfiguration, accessPointServerAddress)
JSON_MAP_REFERENCE(PiPedalConfiguration, isVst3Enabled)
JSON_MAP_REFERENCE(PiPedalConfiguration, end)
JSON_MAP_END()
