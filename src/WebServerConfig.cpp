 // Copyright (c) 2024 Robin Davies
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

#include "WebServerConfig.hpp"
#include "WebServer.hpp"
#include <boost/system/error_code.hpp>
#include <filesystem>
#include "PiPedalConfiguration.hpp"
#include "PiPedalModel.hpp"
#include "Banks.hpp"
#include "Ipv6Helpers.hpp"
#include <memory>
#include "ZipFile.hpp"
#include "PiPedalUI.hpp"
#include "ofstream_synced.hpp"


#define PRESET_EXTENSION ".piPreset"
#define BANK_EXTENSION ".piBank"
#define PLUGIN_PRESETS_EXTENSION ".piPluginPresets"


using namespace pipedal;
using namespace boost::system;

static std::vector<std::string> split(const std::string& s, char delimiter) {
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter)) {
        tokens.push_back(token);
    }
    return tokens;
}

class ExtensionChecker {
public:
    ExtensionChecker(const std::string&extensionList)
    :extensions(split(extensionList,','))
    {
    }
    bool IsValidExtension(const std::string&extension)
    {
        if (extensions.size() == 0) 
            return true;
        for (const auto&ext: extensions)
        {
            if (ext == extension) 
                return true;
        }
        return false;
    }
private:
    std::vector<std::string> extensions;
};

class DownloadIntercept : public RequestHandler
{
    PiPedalModel *model;

public:
    DownloadIntercept(PiPedalModel *model)
        : RequestHandler("/var"),
          model(model)
    {
    }

    virtual bool wants(const std::string &method, const uri &request_uri) const
    {
        if (request_uri.segment_count() != 2 || request_uri.segment(0) != "var")
        {
            return false;
        }
        std::string segment = request_uri.segment(1);
        if (segment == "uploadPluginPresets")
        {
            return true;
        }
        if (segment == "downloadPluginPresets")
        {
            return true;
        }
        if (segment == "downloadPreset")
        {
            std::string strInstanceId = request_uri.query("id");
            if (strInstanceId != "")
                return true;
        }
        else if (segment == "uploadPreset")
        {
            return true;
        }
        if (segment == "downloadBank")
        {
            std::string strInstanceId = request_uri.query("id");
            if (strInstanceId != "")
                return true;
        }
        else if (segment == "uploadBank")
        {
            return true;
        } else if (segment == "uploadUserFile")
        {
            return true;
        }
        return false;
    }

    std::string GetContentDispositionHeader(const std::string &name, const std::string &extension)
    {
        std::string fileName = name.substr(0, 64) + extension;
        std::stringstream s;

        s << "attachment; filename*=" << HtmlHelper::Rfc5987EncodeFileName(fileName) << "; filename=\"" << HtmlHelper::SafeFileName(fileName) << "\"";
        std::string result = s.str();
        return result;
    }

    void GetPluginPresets(const uri &request_uri, std::string *pName, std::string *pContent)
    {
        std::string pluginUri = request_uri.query("id");
        auto plugin = model->GetLv2Host().GetPluginInfo(pluginUri);
        *pName = plugin->name();

        PluginPresets pluginPresets = model->GetPluginPresets(pluginUri);

        std::stringstream s;
        json_writer writer(s,false);
        writer.write(pluginPresets);
        *pContent = s.str();
    }
    void GetPreset(const uri &request_uri, std::string *pName, std::string *pContent)
    {
        std::string strInstanceId = request_uri.query("id");
        int64_t instanceId = std::stol(strInstanceId);
        auto pedalboard = model->GetPreset(instanceId);

        // a certain elegance to using same file format for banks and presets.
        BankFile file;
        file.name(pedalboard.name());
        int64_t newInstanceId = file.addPreset(pedalboard);
        file.selectedPreset(newInstanceId);

        std::stringstream s;
        json_writer writer(s,false);
        writer.write(file);
        *pContent = s.str();
        *pName = pedalboard.name();
    }
    void GetBank(const uri &request_uri, std::string *pName, std::string *pContent)
    {
        std::string strInstanceId = request_uri.query("id");
        int64_t instanceId = std::stol(strInstanceId);
        BankFile bank;
        model->GetBank(instanceId, &bank);

        std::stringstream s;
        json_writer writer(s, true); // do what we can to reduce the file size.
        writer.write(bank);
        *pContent = s.str();
        *pName = bank.name();
    }

    virtual void head_response(
        const uri &request_uri,
        HttpRequest &req,
        HttpResponse &res,
        std::error_code &ec) override
    {
        try
        {
            std::string segment = request_uri.segment(1);
            if (segment == "downloadPluginPresets")
            {
                std::string name;
                std::string content;
                GetPluginPresets(request_uri, &name, &content);
                res.set(HttpField::content_type, "application/json");
                res.set(HttpField::cache_control, "no-cache");
                res.setContentLength(content.length());
                res.set(HttpField::content_disposition, GetContentDispositionHeader(name, PLUGIN_PRESETS_EXTENSION));
                return;
            }
            if (segment == "downloadPreset")
            {
                std::string name;
                std::string content;
                GetPreset(request_uri, &name, &content);

                res.set(HttpField::content_type, "application/json");
                res.set(HttpField::cache_control, "no-cache");
                res.setContentLength(content.length());
                res.set(HttpField::content_disposition, GetContentDispositionHeader(name, PRESET_EXTENSION));
                return;
            }
            if (segment == "downloadBank")
            {
                std::string name;
                std::string content;
                GetBank(request_uri, &name, &content);

                res.set(HttpField::content_type, "application/json");
                res.set(HttpField::cache_control, "no-cache");
                res.set(HttpField::content_disposition, GetContentDispositionHeader(name, BANK_EXTENSION));
                res.setContentLength(content.length());
                return;
            }
            throw PiPedalException("Not found.");
        }
        catch (const std::exception &e)
        {
            if (strcmp(e.what(), "Not found") == 0)
            {
                ec = boost::system::errc::make_error_code(boost::system::errc::no_such_file_or_directory);
            }
            else
            {
                ec = boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
            }
        }
    }

    virtual void get_response(
        const uri &request_uri,
        HttpRequest &req,
        HttpResponse &res,
        std::error_code &ec) override
    {
        try
        {
            std::string segment = request_uri.segment(1);

            if (segment == "downloadPluginPresets")
            {
                std::string name;
                std::string content;
                GetPluginPresets(request_uri, &name, &content);
                res.set(HttpField::content_type, "application/json");
                res.set(HttpField::cache_control, "no-cache");
                res.setContentLength(content.length());
                res.set(HttpField::content_disposition, GetContentDispositionHeader(name, PLUGIN_PRESETS_EXTENSION));
                res.setBody(content);
            }
            else if (segment == "downloadPreset")
            {
                std::string name;
                std::string content;
                GetPreset(request_uri, &name, &content);

                res.set(HttpField::content_type, "application/json");
                res.set(HttpField::cache_control, "no-cache");
                res.setContentLength(content.length());
                res.set(HttpField::content_disposition, GetContentDispositionHeader(name, PRESET_EXTENSION));
                res.setBody(content);
            }
            else if (segment == "downloadBank")
            {
                std::string name;
                std::string content;
                GetBank(request_uri, &name, &content);

                res.set(HttpField::content_type, "application/json");
                res.set(HttpField::cache_control, "no-cache");
                res.setContentLength(content.length());
                res.set(HttpField::content_disposition, GetContentDispositionHeader(name, BANK_EXTENSION));
                res.setBody(content);
            }
            else
            {
                throw PiPedalException("Not found");
            }
        }
        catch (const std::exception &e)
        {
            if (strcmp(e.what(), "Not found") == 0)
            {
                ec = boost::system::errc::make_error_code(boost::system::errc::no_such_file_or_directory);
            }
            else
            {
                ec = boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
            }
        }
    }
    static std::string GetFirstFolderOrFile(const std::vector<std::string>&fileNames)
    {
        for (const auto &fileName: fileNames)
        {
            size_t nPos = fileName.find('/');
            if (nPos != std::string::npos) {
                return fileName.substr(0,nPos);
            }
        }
        if (fileNames.size() == 0)
        {
            return 0;
        } else {
            return fileNames[0];
        }
    }
    virtual void post_response(
        const uri &request_uri,
        HttpRequest &req,
        HttpResponse &res,
        std::error_code &ec) override
    {
        try
        {
            std::string segment = request_uri.segment(1);

            if (segment == "uploadPluginPresets")
            {
                json_reader reader(req.get_body_input_stream());
                PluginPresets presets;
                reader.read(&presets);
                model->UploadPluginPresets(presets);

                res.set(HttpField::content_type, "application/json");
                res.set(HttpField::cache_control, "no-cache");
                std::stringstream sResult;
                sResult << -1;
                std::string result = sResult.str();
                res.setContentLength(result.length());

                res.setBody(result);
            }
            else if (segment == "uploadPreset")
            {
                json_reader reader(req.get_body_input_stream());

                uint64_t uploadAfter = -1;
                std::string strUploadAfter = request_uri.query("uploadAfter");
                if (strUploadAfter.length() != 0)
                {
                    uploadAfter = std::stol(strUploadAfter);
                }

                BankFile bankFile;
                reader.read(&bankFile);

                uint64_t instanceId = model->UploadPreset(bankFile, uploadAfter);

                res.set(HttpField::content_type, "application/json");
                res.set(HttpField::cache_control, "no-cache");
                std::stringstream sResult;
                sResult << instanceId;
                std::string result = sResult.str();
                res.setContentLength(result.length());

                res.setBody(result);
            }
            else if (segment == "uploadBank")
            {
                json_reader reader(req.get_body_input_stream());

                uint64_t uploadAfter = -1;
                std::string strUploadAfter = request_uri.query("uploadAfter");
                if (strUploadAfter.length() != 0)
                {
                    uploadAfter = std::stol(strUploadAfter);
                }

                BankFile bankFile;
                reader.read(&bankFile);

                uint64_t instanceId = model->UploadBank(bankFile, uploadAfter);

                res.set(HttpField::content_type, "application/json");
                res.set(HttpField::cache_control, "no-cache");
                std::stringstream sResult;
                sResult << instanceId;
                std::string result = sResult.str();
                res.setContentLength(result.length());

                res.setBody(result);
            } else if (segment == "uploadUserFile")
            {
                res.set(HttpField::content_type, "application/json");
                res.set(HttpField::cache_control, "no-cache");
                const std::string instanceId = request_uri.query("id");
                const std::string directory = request_uri.query("directory");
                const std::string filename = request_uri.query("filename");
                const std::string patchProperty = request_uri.query("property");


                if (patchProperty.length() == 0 && directory.length() == 0)
                {
                    throw PiPedalException("Malformed request.");

                }

                res.set(HttpField::content_type, "application/json");
                res.set(HttpField::cache_control, "no-cache");

                std::string outputFileName = std::filesystem::path(directory) / filename;

                if (filename.ends_with(".zip"))
                {
                    ExtensionChecker extensionChecker { request_uri.query("ext") };
                    namespace fs = std::filesystem;
                    try {
                        auto zipFile = ZipFile::Create(req.get_body_temporary_file());
                        std::vector<std::string> files = zipFile->GetFiles();
                        for (const auto&inputFile : files)
                        {
                            if (!inputFile.ends_with("/")) // don't process directory entries.
                            {
                                fs::path inputPath { inputFile};
                                std::string extension = inputPath.extension();
                                if (extensionChecker.IsValidExtension(extension))
                                {
                                    auto si = zipFile->GetFileInputStream(inputFile);
                                    std::string path = this->model->UploadUserFile(directory,patchProperty,inputFile, si,zipFile->GetFileSize(inputFile));
                                }
                            }
                        }
                        // set outputPath to the file or folder we would like focus to go to.
                        // almost always a single folder in the root.
                        std::string returnPath = GetFirstFolderOrFile(files);
                        outputFileName = this->model->GetPluginUploadDirectory() / directory / returnPath;
                    } catch (const std::exception &e)
                    {
                        Lv2Log::error(SS("Unzip failed. " << e.what()));
                        throw;
                    }
                    FileSystemSync();

                } else {
                    outputFileName = this->model->UploadUserFile(directory,patchProperty,filename,req.get_body_input_stream(), req.content_length());
                }

                std::stringstream ss;
                json_writer writer(ss);
                writer.write(outputFileName);
                std::string response = ss.str();

                res.setContentLength(response.length());
                res.setBody(response);
            }
            else
            {
                throw PiPedalException("Not found");
            }
        }
        catch (const std::exception &e)
        {
            if (strcmp(e.what(), "Not found") == 0)
            {
                ec = boost::system::errc::make_error_code(boost::system::errc::no_such_file_or_directory);
            }
            else
            {
                ec = boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
            }
        }
    }
};


static std::string StripPortNumber(const std::string &fromAddress)
{
    std::string address = fromAddress;

    if (address.size() == 0)
        return fromAddress;

    char lastChar = address[address.size() - 1];
    size_t pos = address.find_last_of(':');

    // if ipv6, make sure we found an actual port address.
    size_t posBracket = address.find_last_of(']');
    if (posBracket != std::string::npos && pos != std::string::npos)
    {
        if (posBracket > pos)
        {
            pos = std::string::npos;
        }
    }
    if (pos != std::string::npos)
    {
        address = address.substr(0, pos);
    }
    return address;
}

/* When hosting a react app, replace /var/config.json with
   data that will connect the react app with our socket server.
*/

class InterceptConfig : public RequestHandler
{
private:
    uint64_t maxUploadSize;
    int portNumber;

public:
    InterceptConfig(int portNumber, uint64_t maxUploadSize)
        : RequestHandler("/var/config.json"),
          maxUploadSize(maxUploadSize),
          portNumber(portNumber)
    {
    }
    std::string GetConfig(const std::string &fromAddress)
    {
        std::string webSocketAddress = GetNonLinkLocalAddress(StripPortNumber(fromAddress));
        Lv2Log::info(SS("Web Socket Address: " << webSocketAddress << ":" << portNumber));

        std::stringstream s;

        s << "{ \"socket_server_port\": " << portNumber
          << ", \"socket_server_address\": \"" << webSocketAddress << 
          "\", \"ui_plugins\": [ ], \"max_upload_size\": " << maxUploadSize << " }";

        return s.str();
    }
    virtual ~InterceptConfig() {}

    virtual void head_response(
        const uri &request_uri,
        HttpRequest &req,
        HttpResponse &res,
        std::error_code &ec) override
    {
        // intercepted. See the other overload.
    }

    virtual void head_response(
        const std::string &fromAddress,
        const uri &request_uri,
        HttpRequest &req,
        HttpResponse &res,
        std::error_code &ec) override
    {
        std::string response = GetConfig(fromAddress);
        res.set(HttpField::content_type, "application/json");
        res.set(HttpField::cache_control, "no-cache");
        res.setContentLength(response.length());
        return;
    }

    virtual void get_response(
        const uri &request_uri,
        HttpRequest &req,
        HttpResponse &res,
        std::error_code &ec) override
    {
        // intercepted. see the other overload.
    }

    virtual void get_response(
        const std::string &fromAddress,
        const uri &request_uri,
        HttpRequest &req,
        HttpResponse &res,
        std::error_code &ec) override
    {
        std::string response = GetConfig(fromAddress);
        res.set(HttpField::content_type, "application/json");
        res.set(HttpField::cache_control, "no-cache");
        res.setContentLength(response.length());
        res.setBody(response);
    }
};

void pipedal::ConfigureWebServer(
    WebServer&server,
    PiPedalModel&model,
    int port,
    size_t maxUploadSize)
{
        std::shared_ptr<RequestHandler> interceptConfig{new InterceptConfig(port, maxUploadSize)};
        server.AddRequestHandler(interceptConfig);

        std::shared_ptr<DownloadIntercept> downloadIntercept = std::make_shared<DownloadIntercept>(&model);
        server.AddRequestHandler(downloadIntercept);


}
