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
#include "json.hpp"
#include <fstream>
#include <filesystem>
#include "PiPedalException.hpp"
#include <string>
#include <limits>
#include <filesystem>
#include "Lv2Log.hpp"
#include <boost/asio/ip/network_v4.hpp>

namespace pipedal {


class PiPedalConfiguration
{
private:
    std::string lv2_path_ = "/usr/lib/lv2:/usr/local/lib/lv2";
    std::filesystem::path docRoot_;
    std::filesystem::path webRoot_;
    std::string local_storage_path_;
    bool mlock_ = true;
    std::vector<std::string> reactServerAddresses_ = {"*:5000"};
    std::string socketServerAddress_ = "0.0.0.0:8080";
    uint32_t threads_ = 5;
    bool logHttpRequests_ = false;
    int logLevel_ = 0;
    uint64_t maxUploadSize_ = 1024*1024;
    std::string accessPointGateway_;
    std::string accessPointServerAddress_;

public:

    std::filesystem::path GetConfigFilePath() const {
        return docRoot_ / "config.jason";
    }
    const std::filesystem::path& GetWebRoot() const {
        return webRoot_;
    }
    void Load(const std::filesystem::path& path, const std::filesystem::path&webRoot) {
        std::filesystem::path configPath =  path / "config.json";
        if (!std::filesystem::exists(configPath))
        {
            throw PiPedalException("File not found.");
        }
        std::ifstream f(configPath);
        if (!f.is_open())
        {
            std::stringstream s;
            s << "Unable to open " << configPath;
            throw PiPedalStateException(s.str());
        }
        json_reader reader(f);
        reader.read(this);
        docRoot_ = path;

        webRoot_ = webRoot;
        
    }
    const std::filesystem::path &GetDocRoot() const { return docRoot_; }
    const std::string &GetLv2Path() const { return lv2_path_; }
    const std::string &GetLocalStoragePath() const { return local_storage_path_; }
    const std::vector<std::string> &GetReactServerAddresses() const { return reactServerAddresses_;}
    bool GetMLock() const { return mlock_; }
    uint64_t GetMaxUploadSize() const { return maxUploadSize_; }

    LogLevel GetLogLevel() const { return (LogLevel)this->logLevel_; }

    bool LogHttpRequests() const { return this->logHttpRequests_; }

    void SetSocketServerEndpoint(const std::string &endpoint)
    {
        this->socketServerAddress_ = endpoint;
    }

    bool GetAccessPointGateway(boost::asio::ip::network_v4 *pGatewayNetwork) const {
        if (this->accessPointGateway_.length() == 0) return false;
        *pGatewayNetwork = boost::asio::ip::make_network_v4(this->accessPointGateway_.c_str());
        return true;
    }
    std::string GetAccessPointServerAddress() const { return this->accessPointServerAddress_; }
    std::string GetSocketServerAddress() const { 
        size_t pos = this->socketServerAddress_.find_last_of(':');
        if (pos == std::string::npos)
        {
            return socketServerAddress_;
        }
        return socketServerAddress_.substr(0,pos);
    }
    
    uint16_t GetSocketServerPort() const {
        try {
            size_t pos = this->socketServerAddress_.find_last_of(':');
            if (pos == std::string::npos)
            {
                throw std::exception();
            }
            std::string strPort = socketServerAddress_.substr(pos+1);
            unsigned long port =  std::stoul(strPort);
            if (port == 0) 
            {
                throw std::exception();
            }
            if (port > std::numeric_limits<uint16_t>::max())
            {
                throw std::exception();
            }
            return (uint16_t) port;
        } catch (const std::exception &)
        {
            std::stringstream s;
            s << "Invalid port number in '" << this->socketServerAddress_ << "'.";
            throw PiPedalArgumentException(s.str());
        }

    }
    uint32_t GetThreads() const { return threads_; }

    DECLARE_JSON_MAP(PiPedalConfiguration);
};


};