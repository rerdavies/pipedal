/*
 *   Copyright (c) 2025 Robin E. R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.

 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#include "WebServerMod.hpp"
#include <filesystem>
#include "json_variant.hpp"
#include "ss.hpp"
#include "ModTemplateGenerator.hpp"
#include "HtmlHelper.hpp"
#include "MimeTypes.hpp"

using namespace pipedal;
namespace fs = std::filesystem;

namespace pipedal::impl
{

    class ModWebInterceptImpl : public ModWebIntercept
    {
    private:
        using super = ModWebIntercept;
        using self = ModWebInterceptImpl;

        PiPedalModel *model;

    public:
        ModWebInterceptImpl(PiPedalModel *model)
            : super("/resources"),
              model(model)
        {
        }
        virtual bool wants(const std::string &method, const uri &request_uri) const override;

        virtual void head_response(
            const uri &request_uri,
            HttpRequest &req,
            HttpResponse &res,
            std::error_code &ec) override;

        virtual void get_response(
            const uri &request_uri,
            HttpRequest &req,
            HttpResponse &res,
            std::error_code &ec) override;

    private:
        std::string GenerateTemplate(
            const fs::path &templateFile,
            std::shared_ptr<Lv2PluginInfo> pluginInfo);
    };
}

using namespace pipedal::impl;

ModWebIntercept::ptr ModWebIntercept::Create(PiPedalModel *model)
{
    return std::make_shared<impl::ModWebInterceptImpl>(model);
}

bool ModWebInterceptImpl::wants(const std::string &method, const uri &request_uri) const
{
    if (request_uri.segment_count() < 2 || request_uri.segment(0) != "resources")
    {
        return false;
    }
    return true;
}

void ModWebInterceptImpl::head_response(
    const uri &request_uri,
    HttpRequest &req,
    HttpResponse &res,
    std::error_code &ec)
{
    get_response(request_uri, req, res, ec);
    res.clearBody();
}

static void setCacheControl(HttpResponse &res, const fs::path &path)
{
    if (fs::exists(path))
    {
        res.set("Cache-Control", "public, max-age=31536000"); // 1 month
        auto lastModified = std::filesystem::last_write_time(path);
        res.set(HttpField::LastModified, HtmlHelper::timeToHttpDate(lastModified));
    }
    else
    {
        res.set("Cache-Control", "no-cache, no-store, must-revalidate");
    }
}
void ModWebInterceptImpl::get_response(
    const uri &request_uri,
    HttpRequest &req,
    HttpResponse &res,
    std::error_code &ec)
{
    try
    {
        std::string ns = request_uri.query("ns");
        std::string strVersion = request_uri.query("v");
        (void)strVersion; // this is just cache-busting, so we don't actually use it.

        std::shared_ptr<Lv2PluginInfo> pluginInfo;
        if (!ns.empty())
        {
            pluginInfo = model->GetPluginInfo(ns);
            if (!pluginInfo)
            {
                throw std::runtime_error("Plugin not found.");
            }
        }
        if (!pluginInfo->modGui())
        {
            throw std::runtime_error("Plugin does not have a ModGui.");
        }
        if (request_uri.segment_count() < 2)
        {
            throw std::runtime_error("Invalid request URI.");
        }
        if (request_uri.segment(0) != "resources")
        {
            throw std::runtime_error("Invalid request URI: expected 'resources'.");
        }   
        std::string segment = request_uri.segment(1);
        if (segment == "_")
        {
            segment = request_uri.segment(2);
            if (segment == "iconTemplate")
            {
                res.set("Content-Type", "text/html");
                res.setBody(GenerateTemplate(
                    pluginInfo->modGui()->iconTemplate(),
                    pluginInfo));
                setCacheControl(res, pluginInfo->modGui()->iconTemplate());
            }
            else if (segment == "stylesheet")
            {
                res.set("Content-Type", "text/css");
                res.setBody(GenerateTemplate(
                    pluginInfo->modGui()->stylesheet(),
                    pluginInfo));
                setCacheControl(res, pluginInfo->modGui()->stylesheet());

            }

            else if (segment == "screenshot")
            {
                std::filesystem::path path = pluginInfo->modGui()->screenshot();
                if (!fs::exists(path))
                {
                    ec =  std::make_error_code(std::errc::no_such_file_or_directory);
                    return;
                }
                if (!fs::is_regular_file(path))
                {
                    throw std::runtime_error("Screenshot is not a regular file: " + path.string());
                }

                size_t size = fs::file_size(path);
                std::string mimeType = MimeTypes::instance().MimeTypeFromExtension(path.extension().string());
                if (mimeType.empty())
                {
                    throw std::runtime_error("Unknown file type for screenshot: " + path.string());
                }
                res.setBodyFile(
                    path,
                    false); // delete when done
                res.set("Content-Type", mimeType);
                res.setContentLength(size);
                setCacheControl(res, path);

            }
            else if (segment == "thumbnail")
            {
                std::filesystem::path path = pluginInfo->modGui()->thumbnail();
                if (!fs::exists(path))
                {
                    throw std::runtime_error("Thumbnail not found: " + path.string());
                }
                if (!fs::is_regular_file(path))
                {
                    throw std::runtime_error("Thumbnail is not a regular file: " + path.string());
                }

                size_t size = fs::file_size(path);
                std::string mimeType = MimeTypes::instance().MimeTypeFromExtension(path.extension().string());
                if (mimeType.empty())
                {
                    throw std::runtime_error("Unknown file type for thumbnail: " + path.string());
                }
                res.setBodyFile(
                    path,
                    false); // delete when done
                res.set("Content-Type", mimeType);
                setCacheControl(res, path);
                res.setContentLength(size);
            } else {
                throw std::runtime_error("Unknown resource: _/" + segment);
            }
        }
        else
        {
            // a request for a plugin resource file.
            fs::path resourcefile = pluginInfo->modGui()->resourceDirectory();
            for (size_t i = 1; i < request_uri.segment_count(); ++i)
            {
                resourcefile /= request_uri.segment(i);
            }
            if (!fs::exists(resourcefile) && !fs::is_regular_file(resourcefile))
            {
                ec =  std::make_error_code(std::errc::no_such_file_or_directory);
                return;
            }
            size_t size = fs::file_size(resourcefile);
            std::string mimeType = MimeTypes::instance().MimeTypeFromExtension(resourcefile.extension().string());
            if (mimeType.empty())
            {
                throw std::runtime_error("Unknown file type for resource: " + resourcefile.string());
            }
            res.setBodyFile(
                resourcefile,
                false); // delete when done
            res.set("Content-Type", mimeType);
            Lv2Log::debug(SS("ModWebIntercept: Serving resource file: " << resourcefile.string() << " (" << size << " bytes)"));
            Lv2Log::debug(SS("    url: " << request_uri.str()));
            setCacheControl(res, resourcefile);
            res.setContentLength(size);
        }
    }
    catch (const std::exception &e)
    {
        res.setBody("Error: " + std::string(e.what()));
        ec = boost::system::errc::make_error_code(boost::system::errc::invalid_argument);
        return;
    }
}

static std::string makeCns(const std::string &encodedUri, int64_t instanceId)
{
    std::stringstream ss;
    ss << "_";
    for (char c : encodedUri)
    {
        if (
            (c >= 'a' && c <= 'z') || (c >= 'A' & c <= 'Z') || (c >= '0' && c <= '9'))
        {
            ss << c;
        }
        else
        {
            ss << '_'; // replace non-alphanumeric characters with '_'
        }
    }
    ss << "_";
    if (instanceId < 0)
    {
        ss << "_";
        instanceId = -instanceId;
    }
    ss << instanceId;
    return ss.str();
}

std::string ModWebInterceptImpl::GenerateTemplate(
    const fs::path &templateFile,
    std::shared_ptr<Lv2PluginInfo> pluginInfo)
{
    if (!fs::exists(templateFile))
    {
        throw std::runtime_error("File not found. " + templateFile.string());
    }

    json_variant context = MakeModGuiTemplateData(pluginInfo);
    int64_t version = pluginInfo->minorVersion() * 1000 +
                      pluginInfo->microVersion();

    std::string encodedUri = HtmlHelper::encode_url_segment(pluginInfo->uri(), true);
    std::string ns = SS("?ns=" << encodedUri << "&v=" << version);
    context["_ns"] = ns;

    std::string cns = makeCns(encodedUri, version);
    context["_cns"] = cns;

    return GenerateFromTemplateFile(
        templateFile,
        context);
}
