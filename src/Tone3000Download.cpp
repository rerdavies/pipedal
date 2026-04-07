/*
 *   Copyright (c) 2026 Robin E. R. Davies
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

#include "ss.hpp"
#include "Tone3000Download.hpp"
#include "Tone3000Throttler.hpp"
#include "TemporaryFile.hpp"
#include <array>
#include <ctime>
#include "util.hpp"
#include "Tone3000DownloadProgress.hpp"
#include <thread>
#include "HtmlHelper.hpp"
#include "Curl.hpp"
#include "PiPedalModel.hpp"

using namespace pipedal;
using namespace pipedal::tone3000;
namespace fs = std::filesystem;

JSON_MAP_BEGIN(Tone3000Model)
JSON_MAP_REFERENCE(Tone3000Model, id)
JSON_MAP_REFERENCE(Tone3000Model, name)
JSON_MAP_REFERENCE(Tone3000Model, model_url)
JSON_MAP_REFERENCE(Tone3000Model, created_at)
JSON_MAP_REFERENCE(Tone3000Model, size)
JSON_MAP_REFERENCE(Tone3000Model, user_id)
JSON_MAP_END()

JSON_MAP_BEGIN(Tone3000User)
JSON_MAP_REFERENCE(Tone3000User, id)
JSON_MAP_REFERENCE(Tone3000User, username)
JSON_MAP_REFERENCE(Tone3000User, avatar_url)
JSON_MAP_REFERENCE(Tone3000User, url)
JSON_MAP_END()

JSON_MAP_BEGIN(Tone3000Download)
JSON_MAP_REFERENCE(Tone3000Download, id)
JSON_MAP_REFERENCE(Tone3000Download, user_id)
JSON_MAP_REFERENCE(Tone3000Download, title)
JSON_MAP_REFERENCE(Tone3000Download, description)
JSON_MAP_REFERENCE(Tone3000Download, created_at)
JSON_MAP_REFERENCE(Tone3000Download, updated_at)
JSON_MAP_REFERENCE(Tone3000Download, platform)
JSON_MAP_REFERENCE(Tone3000Download, gear)
JSON_MAP_REFERENCE(Tone3000Download, images)
JSON_MAP_REFERENCE(Tone3000Download, is_public)
JSON_MAP_REFERENCE(Tone3000Download, links)
JSON_MAP_REFERENCE(Tone3000Download, model_count)
JSON_MAP_REFERENCE(Tone3000Download, favorites_count)
JSON_MAP_REFERENCE(Tone3000Download, license)
JSON_MAP_REFERENCE(Tone3000Download, sizes)
JSON_MAP_REFERENCE(Tone3000Download, user)
JSON_MAP_REFERENCE(Tone3000Download, models)
JSON_MAP_END()

JSON_MAP_BEGIN(Tone3000DownloadResult)
JSON_MAP_REFERENCE(Tone3000DownloadResult, success)
JSON_MAP_REFERENCE(Tone3000DownloadResult, errorMessage)
JSON_MAP_END()

static const std::filesystem::path WEB_TEMP_DIR{"/var/pipedal/web_temp"};
static const std::filesystem::path TONE3000_THUMBNAIL_PATH{"/var/pipedal/tone3000_thumbnails"};

static std::string getCurlErrorFromExitCode(int exitCode)
{

    switch (exitCode)
    {
    case 3 * 256: // malformed URL
        return "Invalid URL.";
    case 6 * 256: // could not resolve hostname
        return "Could not resolve hostname. Check that the PiPedal server is connected to the Internet.";
    case 7 * 256: // failed to connect to host
        return "Failed to connect to host.";
    case 22 * 256: // HTTP error
        return "HTTP error downloading file.";
    case 28 * 256: // timeout
        return "Download timeout.";
    case 63 * 256: // file too large
        return "File too large.";
    default:
        return SS("Failed to download file (exit code: " << (exitCode / 256) << ")");
    }
}

static void cancellableSleep(std::chrono::steady_clock::duration duration, std::function<bool()> &isCancelled)
{
    std::chrono::steady_clock::time_point start = std::chrono::steady_clock::now();
    while (true)
    {
        if (std::chrono::steady_clock::now() - start >= duration)
        {
            break;
        }
        if (isCancelled())
        {
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

static std::string mdSanitize(const std::string &str, const std::string &illegalCharacters = "")
{
    std::ostringstream os;
    for (char c : str)
    {
        if ((unsigned char)c < 0x20)
            continue;
        if (c == '<')
        {
            os << "&lt;";
            continue;
        }
        auto npos = illegalCharacters.find_first_of(c);
        if (npos != std::string::npos)
        {
            continue;
        }
        os << c;
    }
    return os.str();
}

static std::string mdDate(const tone3000_time_point &date_)
{
    std::ostringstream os;
    // C++ doesn't handle timezones. Just zap the timezone, and replace it with "Z"
    std::string date = date_;

    // Remove timezone offset if present and replace with Z
    size_t plusPos = date.find('+');
    if (plusPos != std::string::npos)
    {
        date = date.substr(0, plusPos) + "Z";
    }
    // Parse ISO 8601 date string into tm
    std::tm tm = {};
    std::istringstream ss(date);
    ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
    if (ss.fail())
    {
        os << date;
        return os.str();
    }

    os << std::put_time(&tm, "%x");
    return os.str();
}

static std::string mdEnum(const std::string &value, const std::map<std::string, std::string> &enumValues)
{
    auto ff = enumValues.find(value);
    if (ff == enumValues.end())
    {
        return value;
    }
    return ff->second;
}

static std::string mdEnumList(
    const std::vector<std::string> &values,
    const std::map<std::string, std::string> &enumValues)
{
    if (values.size() == 1)
    {
        return mdEnum(values[0], enumValues);
    }
    std::ostringstream os;
    os << '[';
    bool first = true;
    for (const auto &value : values)
    {
        if (first)
        {
            first = false;
        }
        else
        {
            os << ", ";
        }
        os << mdEnum(value, enumValues);
    }
    os << ']';
    return os.str();
}

static std::map<std::string, std::string> sizeEnumValues =
    {
        {"standard", "Standard"},
        {"lite", "Lite"},
        {"feather", "Feather"},
        {"nano", "Nano"},
        {"custom", "Custom"}};
static std::string mdSizes(const std::vector<std::string> &sizes)
{
    return mdEnumList(sizes, sizeEnumValues);
}

static std::map<std::string, std::string> gearEnumValues =
    {
        {"amp", "Amp only"},
        {"full-rig", "Full rig"},
        {"pedal", "Pedal"},
        {"outboard", "Outboard"},
        {"ir", "I/R"}};

static std::string mdGear(const std::string &gear)
{
    return mdEnum(gear, gearEnumValues);
}

namespace
{
    enum class LicenseFlags
    {
        None = 0,
        Cc = 1,
        By = 2,
        CcBy = 3, // = Cc | By
        Sa = 4,
        Nc = 8,
        Nd = 16,
        Cc0 = 32,
    };

    static bool operator&(LicenseFlags v1, LicenseFlags v2)
    {
        return (((int)v1) & ((int)v2)) != 0;
    }
    static LicenseFlags operator|(LicenseFlags v1, LicenseFlags v2)
    {
        return (LicenseFlags)(((int)(v1)) | ((int)(v2)));
    }

    struct LicenseInfo
    {
        std::string key;
        std::string displayName;
        std::string url;
        LicenseFlags licenseFlags;
    };
}

static std::vector<LicenseInfo> licenses{
    {.key = "t3k",
     .displayName = "T3K",
     .url = "licenses/t3k_license.html",
     .licenseFlags = LicenseFlags::None},
    {.key = "cc-by",
     .displayName = "CC BY 4.0",
     .url = "https://creativecommons.org/licenses/by/4.0/",
     .licenseFlags = LicenseFlags::CcBy},
    {.key = "cc-by-sa",
     .displayName = "CC BY-SA 4.0",
     .url = "https://creativecommons.org/licenses/by-sa/4.0/",
     .licenseFlags = LicenseFlags::CcBy | LicenseFlags::Sa},
    {.key = "cc-by-nc",
     .displayName = "CC BY-NC 4.0",
     .url = "https://creativecommons.org/licenses/by-nc/4.0/",
     .licenseFlags = LicenseFlags::CcBy | LicenseFlags::Nc},
    {.key = "cc-by-nc-sa",
     .displayName = "CC BY-NC-SA 4.0",
     .url = "https://creativecommons.org/licenses/by-nc-sa/4.0/",
     .licenseFlags = LicenseFlags::CcBy | LicenseFlags::Nc | LicenseFlags::Sa},
    {.key = "cc-by-nd",
     .displayName = "CC BY-ND 4.0",
     .url = "https://creativecommons.org/licenses/by-nd/4.0/",
     .licenseFlags = LicenseFlags::CcBy | LicenseFlags::Nd},
    {.key = "cc-by-nc-nd",
     .displayName = "CC BY-NC-ND 4.0",
     .url = "https://creativecommons.org/licenses/by-nc-nd/4.0/",
     .licenseFlags = LicenseFlags::CcBy | LicenseFlags::Nc | LicenseFlags::Nd},
    {
        .key = "cco",
        .displayName = "CC0",
        .url = "https://creativecommons.org/publicdomain/zero/1.0/",
        .licenseFlags = LicenseFlags::Cc | LicenseFlags::Cc0,

    },
};

static std::map<std::string, LicenseInfo> makeLicenseEnum()
{
    std::map<std::string, LicenseInfo> result;
    for (const auto &license : licenses)
    {
        result[license.key] = license;
    }
    return result;
}

static std::map<std::string, LicenseInfo> licenseEnum = makeLicenseEnum();

static std::string mdLicenseIcon(LicenseFlags licenseFlag)
{
    std::ostringstream os;
    std::string url;

    switch (licenseFlag)
    {
    case LicenseFlags::Cc:
        url = "img/cc.svg";
        break;
    case LicenseFlags::By:
        url = "img/by.svg";
        break;
    case LicenseFlags::Sa:
        url = "img/sa.svg";
        break;
    case LicenseFlags::Nc:
        url = "img/nc.svg";
        break;
    case LicenseFlags::Nd:
        url = "img/nd.svg";
        break;
    case LicenseFlags::Cc0:
        url = "img/cc0.svg";
        break;

    case LicenseFlags::None:
        throw std::runtime_error("Invalid argument.");
    }
    os << "<img id='cc_img' src='" << url << "'/>";
    return os.str();
}

static void mdEscapeString(std::ostream &f, const std::string &str)
{
    size_t ix = 0;
    while (ix < str.size())
    {
        char c = str[ix++];
        if (c == '\n')
        {
            if (ix < str.size() && str[ix] == '\n')
            {
                f << "\n\n";
            }
            else
            {
                f << "  \n"; // a <br> in md.
            }
        }
        else
        {
            f << c;
        }
    }
}

static std::string mdHref(const std::string &url)
{
    std::ostringstream os;
    os << '\'';
    for (char c : url)
    {
        if (c == '\'')
        {
            continue;
        }
        os << c;
    }
    os << '\'';
    return os.str();
}
static std::string mdLicense(const std::string &license)
{

    auto ff = licenseEnum.find(license);
    LicenseInfo licenseInfo;
    if (ff != licenseEnum.end())
    {
        licenseInfo = ff->second;
    }
    else
    {
        licenseInfo.displayName = license;
    }

    std::ostringstream os;

    if (licenseInfo.licenseFlags != LicenseFlags::None)
    {
        for (LicenseFlags licenseFlag : std::vector<LicenseFlags>{LicenseFlags::Cc, LicenseFlags::By, LicenseFlags::Cc0, LicenseFlags::Nc, LicenseFlags::Sa, LicenseFlags::Nd})
        {
            if (licenseInfo.licenseFlags & licenseFlag)
            {
                os << mdLicenseIcon(licenseFlag);
            }
        }
        os << " ";
    }

    if (licenseInfo.url != "")
    {
        os << "<a href="
           << mdHref(licenseInfo.url)
           << " target='_blank' rel='noopener noreferrer' >" << mdSanitize(licenseInfo.displayName) << "</a>";
    }
    else
    {
        os << mdSanitize(licenseInfo.displayName);
    }
    return os.str();
}
// static std::string mdTestLicenses()
// {
//     std::ostringstream os;
//     for (auto license: licenseEnum)
//     {
//         os << "<br/>" << mdLicense(license.first) << "\n";
//     }
//     return os.str();
// }
static std::map<std::string, std::string> platformEnumValues =
    {
        {"nam", "NAM"},
        {"ir", "I/R"},
        {"aida-x", "Aida X"},
        {"aa-snapshot", "aa-snapshot"},
        {"proteus", "Proteus"}};
static std::string mdPlatform(const std::string &platform)
{
    return mdEnum(platform, platformEnumValues);
}
static std::string mdUser(const tone3000::Tone3000User &user)
{
    // clang-format off
        std::ostringstream os;
        os << 
            "<a target='_blank' rel='noopener noreferrer' href='" 
            << mdSanitize(user.url(),"'") 
               << "'>" << mdSanitize(user.username()) << "</a>";
        return os.str();
    // clang-format on
}

static void mdLink(std::ostream &f, const std::string &label, const std::string &url)
{
    f << "[" << mdSanitize(label, "]") << "](" << mdSanitize(url, ")") << ")";
}

static void writeReadme(const fs::path &path, const std::string &thumbnailUrl, const Tone3000Download &download)
{
    std::ofstream of{path};
    if (!of.is_open())
    {
        throw std::runtime_error(SS("Unable to create file " << path));
    }

    // clang-format off
    of <<
        
            "<div id='tone3000_float'  >\n";
    if (!thumbnailUrl.empty())
    {

        of <<  "    <img id='tone3000_thumbnail' src='" 
                        << thumbnailUrl << "' alt=''  />\n";
    }
    of  <<  "    <div id='tone3000_float_content' >\n"
        <<  "        <h3 id='tone3000_title'>" << mdSanitize(download.title()) << "</h3>\n" 
        <<  "        <div>\n"
        <<  "            <p id='tone3000_subtitle'>\n"
                        << mdDate(download.updated_at()) 
                        << " "
                        << mdUser(download.user())
        <<  "            <br/>\n"
        <<  "            ";
        if (download.sizes().has_value()) {
            of
                << mdSizes(download.sizes().value()) << ", "
            ;
        }
        if (download.platform() == "ir")
        {
            of 
                    << (mdPlatform(download.platform()));
        } else {
            of 
                    << mdGear(download.gear()) << ", " 
                    << mdPlatform(download.platform());
        }
        of           << "<br/>\n";
        if (download.license() != "") {
            of <<  "            License: " << mdLicense(download.license()) << "\n";
        }
        of  
//        << "          " << mdTestLicenses()
        <<  "        </p>\n"
        <<  "        </div>\n"
        <<  "    </div>\n"
        << "</div>\n";
    // clang-format on
    of << "\n\n";
    mdEscapeString(of, download.description());

    if (download.links().size() > 0)
    {
        of << std::endl;
        of << "## Links" << std::endl
           << std::endl;
        for (auto &link : download.links())
        {
            of << "- " << "[" << link << "](" << link << ")" << std::endl;
        }
    }
}

static void CleanUpFailedDownload(const std::filesystem::path &folder, const std::vector<CurlDownloadRequest> &requests)
{

    std::error_code ec; // do NOT throw errors.

    if (fs::exists(folder))
    {
        for (const auto &request : requests)
        {
            if (fs::exists(request.outputFile))
            {
                fs::remove(request.outputFile, ec); // ignoring errors.
            }
        }
        // If there's a readme file, remove that too.
        fs::path readmePath = folder / "README.md";
        if (fs::exists(readmePath))
        {
            fs::remove(readmePath, ec); // ignoring errors
        }

        // if the folder directory is empty, remove the the folder too.
        try
        {
            if (fs::is_empty(folder))
            {
                fs::remove(folder, ec); // ignoring errors.
            }
        }
        catch (const std::exception &)
        {
        }
    }
}

static std::string GetHeader(const std::string &headerName, const std::vector<std::string> &htmlHeaders)
{
    std::string needle = headerName;
    std::transform(needle.begin(), needle.end(), needle.begin(), [](unsigned char c)
                   { return std::tolower(c); });

    for (const auto &header : htmlHeaders)
    {
        auto pos = header.find(':');
        if (pos == std::string::npos)
        {
            continue;
        }
        std::string name = header.substr(0, pos);
        std::transform(name.begin(), name.end(), name.begin(), [](unsigned char c)
                       { return std::tolower(c); });
        if (name != needle)
        {
            continue;
        }
        std::string value = header.substr(pos + 1);
        auto first = value.find_first_not_of(" \t\r\n");
        if (first == std::string::npos)
        {
            return "";
        }
        auto last = value.find_last_not_of(" \t\r\n");
        return value.substr(first, last - first + 1);
    }
    return "";
}

static bool CancellableSleep(std::chrono::steady_clock::duration duration, std::function<bool()> &isCancelled)
{
    using clock_t = std::chrono::steady_clock;

    auto start = clock_t::now();
    while (true)
    {
        auto now = clock_t::now();
        auto elapsed = now - start;
        if (elapsed > duration)
        {
            return true;
        }
        if (isCancelled())
        {
            return false;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}
std::string pipedal::tone3000::DownloadTone3000Bundle(
    const std::filesystem::path &destinationFolder,
    const std::string &downloadUrl,
    int64_t handle,
    std::function<void(const Tone3000DownloadProgress &progress)> progressCallback,
    std::function<bool()> isCancelled)
{
    TemporaryFile downloadTemporaryFile{WEB_TEMP_DIR};
    fs::path downloadPath = downloadTemporaryFile.Path();
    std::vector<std::string> headers;

    std::string resultDirectory;
    Tone3000DownloadProgress progress;
    progress.handle(handle);
    progressCallback(progress);

    int httpResult = CurlGet(downloadUrl, downloadPath, &headers);
    if (httpResult != 200)
    {
        throw std::runtime_error(SS("Download failed: HTTP " << HtmlHelper::httpErrorString(httpResult)));
    }

    if (isCancelled())
    {

        return resultDirectory;
    }

    std::ifstream f{downloadPath};
    if (!f.is_open())
    {
        throw std::runtime_error(SS("Can't open file " << downloadPath));
    }
    json_reader reader(f);

    Tone3000Download tone3000Download;
    reader.read(&tone3000Download);

    if (isCancelled())
    {
        return resultDirectory;
    }
    progress.title(tone3000Download.title());
    progress.total(tone3000Download.models().size());
    progressCallback(progress);

    if (tone3000Download.platform() != "nam" && tone3000Download.platform() != "ir")
    {
        throw std::runtime_error(SS("Unexpected file format. (platform=\"" << tone3000Download.platform() << "\")"));
    }
    fs::path bundlePath = destinationFolder / StringToSafeFilename(tone3000Download.title());
    resultDirectory = bundlePath;

    fs::create_directories(bundlePath);

    std::vector<CurlDownloadRequest> requests;

    for (auto &model : tone3000Download.models())
    {
        fs::path modelPath;
        if (tone3000Download.platform() == "ir")
        {
            modelPath = bundlePath / SS(StringToSafeFilename(model.name()) << ".wav");
        }
        else
        {
            modelPath = bundlePath / SS(StringToSafeFilename(model.name()) << ".nam");
        }
        requests.push_back(CurlDownloadRequest{url : model.model_url(), outputFile : modelPath});
    }
    try
    {
        size_t downloadIndex = 0;
        while (downloadIndex < requests.size())
        {
            if (isCancelled())
            {
                throw std::runtime_error("Cancelled");
            }
            size_t thisTime = requests.size() - downloadIndex;
            thisTime = Tone3000Throttler::instance().ReserveDownloadSlots(thisTime);
            if (thisTime != 0)
            {
                std::vector<CurlDownloadRequest> requestsThisTime;
                for (size_t i = 0; i < thisTime; ++i)
                {
                    requestsThisTime.push_back(requests[downloadIndex + i]);
                }

                int result = CurlGet(
                    requestsThisTime,
                    [&](size_t completed, size_t total)
                    {
                        if (isCancelled())
                        {
                            return false;
                        }
                        progress.progress(completed + downloadIndex);
                        progress.total(requests.size());
                        progressCallback(progress);
                        return true;
                    },
                    &headers);

                if (isCancelled())
                {
                    throw std::runtime_error("Cancelled");
                }

                if (result != 200)
                {
                    throw std::runtime_error(SS("Server download failed. HTTP Error " << HtmlHelper::httpErrorString(result)));
                }

                downloadIndex += thisTime;
            }
            if (thisTime <= 1)
            {
                CancellableSleep(std::chrono::milliseconds(1000), isCancelled);
            }
        }

        fs::path readmePath = bundlePath / "README.md";

        std::string thumbnailUrl;
        if (tone3000Download.images().has_value() && tone3000Download.images().value().size() != 0)
        {

            TemporaryFile imageFile{TONE3000_THUMBNAIL_PATH};

            std::string imageUrl = tone3000Download.images().value()[0];
            std::vector<std::string> imageHeaders;
            try
            {
                size_t reserveCount;
                while (true) 
                {
                    reserveCount = Tone3000Throttler::instance().ReserveDownloadSlots(1);
                    if (reserveCount != 0) 
                    {
                        break;
                    }
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    if (isCancelled())
                    {
                        throw std::runtime_error("Cancelled");
                    }
                }
                if (CurlGet(imageUrl, imageFile.Path(), &imageHeaders) == 200)
                {
                    std::string mediaType = GetHeader("Content-Type", imageHeaders);
                    std::string extension;
                    if (mediaType == "image/jpeg")
                    {
                        extension = ".jpeg";
                    }
                    else if (mediaType == "image/webp")
                    {
                        extension = ".webp";
                    }
                    else if (mediaType == "image/png")
                    {
                        extension = ".png";
                    }
                    else
                    {
                        Lv2Log::warning("Tone3000 thumbnail has an unexpected media type of '%s'", mediaType.c_str());
                        throw std::runtime_error("Invalid thumbnail media type.");
                    }
                    if (mediaType == "image/jpeg")
                    {
                        fs::path thumnbailTempPath = imageFile.Detach();
                        fs::path finalPath = TONE3000_THUMBNAIL_PATH / SS(tone3000Download.id() << extension);
                        if (fs::exists(finalPath))
                        {
                            fs::remove(finalPath);
                        }
                        fs::create_directories(finalPath.parent_path());
                        fs::rename(thumnbailTempPath, finalPath);
                        thumbnailUrl = SS("var/tone3000_thumbnail?id=" << tone3000Download.id());
                        //                        thumbnailUrl = mdDataUrl("image/jpeg", finalPath);
                    }
                }
            }
            catch (const std::exception &)
            {
                // ignore.
            }
        }
        writeReadme(readmePath, thumbnailUrl, tone3000Download);
    }
    catch (const std::exception &e)
    {
        CleanUpFailedDownload(bundlePath, requests);
        if (isCancelled())
        {
            return "";
        }
        throw;
    }

    return resultDirectory;
}

Tone3000DownloadResult::Tone3000DownloadResult()
    : success_(true)
{
}
Tone3000DownloadResult::Tone3000DownloadResult(const std::exception &e)
    : success_(false), errorMessage_(e.what())
{
}
