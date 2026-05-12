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

#include "Tone3000DownloaderImpl.hpp"
#include "Tone3000DownloadProgress.hpp"
#include "Tone3000Download.hpp"
#include "Uri.hpp"
#include <functional>
#include "HtmlHelper.hpp"
#include "Curl.hpp"
#include <charconv>
#include <limits>
#include <type_traits>
#include <filesystem>
#include <fstream>
#include "TemporaryFile.hpp"
#include "HtmlHelper.hpp"

using namespace pipedal;
namespace fs = std::filesystem;

static const std::filesystem::path WEB_TEMP_DIR{"/var/pipedal/web_temp"};
static const std::filesystem::path TONE3000_THUMBNAIL_PATH{"/var/pipedal/tone3000_thumbnails"};

class Tone3000CancelledError : std::exception
{
public:
    Tone3000CancelledError() = default;
    virtual ~Tone3000CancelledError() = default;

    virtual const char *what() const noexcept override
    {
        std::exception::what();
        return "Cancelled.";
    }
};

std::shared_ptr<Tone3000Downloader> Tone3000Downloader::Create()
{
    return std::make_shared<Tone3000DownloaderImpl>();
}

Tone3000Downloader::Tone3000Downloader()
{
}

Tone3000Downloader::~Tone3000Downloader()
{
}

Tone3000DownloaderImpl::Tone3000DownloaderImpl()
{
}
Tone3000DownloaderImpl::~Tone3000DownloaderImpl()
{
    Close();
    this->thread = nullptr;
}

void Tone3000DownloaderImpl::SetListener(Listener *listener)
{
    std::lock_guard<std::mutex> lockGuard{this->mutex};
    this->listener = listener;
}

Tone3000DownloaderImpl::handle_t Tone3000DownloaderImpl::NextHandle()
{
    handle_t result = this->nextHandle++;
    return result;
}

void Tone3000DownloaderImpl::CancelDownload(
    handle_t handle)
{
    std::lock_guard<std::mutex> lockGuard{this->mutex};
    requestQueue.Erase([handle](std::shared_ptr<DownloadRequest> entry)
                       { return entry->handle = handle; });
    if (activeRequest)
    {
        if (activeRequest->handle == handle)
        {
            activeRequest->cancelled = true;
        }
    }
}

void Tone3000DownloaderImpl::Close()
{
    bool notify = false;
    {
        std::lock_guard<std::mutex> lockGuard{this->mutex};
        if (!closed)
        {
            closed = true;
            notify = true;
        }

        Tone3000DownloadProgress progress;
        fgDownloadProgress = progress;

        if (listener)
        {
            listener->OnTone3000Progress(progress);
        }
        if (this->activeRequest != nullptr)
        {
            activeRequest->cancelled = true;
        }
    }
    this->requestQueue.CloseAndClear();
}
Tone3000DownloadProgress Tone3000DownloaderImpl::GetDownloadStatus()
{
    Tone3000DownloadProgress result;
    {
        std::lock_guard<std::mutex> lockGuard{this->mutex};
        result = fgDownloadProgress;
    }
    return result;
}

void Tone3000DownloaderImpl::bgUpdateDownloadProgress(const Tone3000DownloadProgress &progress)
{

    std::lock_guard<std::mutex> lockGuard{this->mutex};
    if (closed)
    {
        return;
    }
    if (activeRequest != nullptr && !activeRequest->cancelled)
    {
        this->fgDownloadProgress = progress;
        if (listener)
        {
            listener->OnTone3000Progress(this->fgDownloadProgress);
        }
    }
}

static void ValidateTone3000Url(const std::string &url)
{
    uri downloadUri{url};
    if (!downloadUri.authority().ends_with(".tone3000.com"))
    {
        throw std::runtime_error("Invalid Tone3000 URL address.");
    }
}

namespace
{
    template <typename T>
    static T ParseIntegerOrThrow(const std::string &text, const char *fieldName)
    {
        static_assert(std::is_integral_v<T>, "ParseIntegerOrThrow requires an integral type.");

        T value{};
        const char *begin = text.data();
        const char *end = begin + text.size();
        auto [ptr, ec] = std::from_chars(begin, end, value);
        if (ec == std::errc::invalid_argument || ptr != end)
        {
            throw std::runtime_error(SS("Tone3000 Auth: invalid " << fieldName << "."));
        }
        if (ec == std::errc::result_out_of_range)
        {
            throw std::runtime_error(SS("Tone3000 Auth: " << fieldName << " is out of range."));
        }
        return value;
    }
}

void Tone3000DownloaderImpl::ThreadProc()
{
    while (true)
    {
        Listener *currentListener = nullptr;

        Tone3000DownloadProgress progress;

        auto request = requestQueue.Take();
        if (request == nullptr)
        {
            return;
        }

        {
            std::unique_lock<std::mutex> lock{this->mutex};
            activeRequest = request;
            currentListener = listener;
        }

        // Notify download started
        if (currentListener)
        {
            currentListener->OnStartTone3000Download(request->handle, "");
        }

        // Create cancellation predicate that checks the atomic flag
        auto isCancelled = [request]() -> bool
        {
            return request->cancelled.load();
        };

        try
        {
            this->DownloadTone3000ToneBg(
                request);
            // std::string outputPath;

            // Tone3000DownloadProgress progress;
            // progress.total(request->tone3000DownloadRequest.models().size());
            // progress.handle(request->handle);

            // this->bgUpdateDownloadProgress(progress);

            // outputPath = PerformFileDownloads(
            //     request->tone3000DownloadRequest,
            //     progress,
            //     [this](Tone3000DownloadProgress &progress)
            //     { bgUpdateDownloadProgress(progress); },
            //     isCancelled);

            // // Notify completion
            // {
            //     std::lock_guard lockGuard{mutex};
            //     if (currentListener)
            //     {
            //         currentListener->OnTone3000DownloadComplete(request->handle, outputPath);
            //     }
            // }
        }
        catch (const std::exception &e)
        {
            {
                std::lock_guard lockGuard{mutex};
                // Notify error
                if (!request->cancelled.load() && currentListener)
                {
                    currentListener->OnTone3000DownloadError(request->handle, e.what());
                }
            }
        }

        {
            std::lock_guard<std::mutex> lock{this->mutex};
            activeRequest = nullptr;
        }
    }
}

void Tone3000DownloaderImpl::DownloadTone3000ToneBg(
    std::shared_ptr<DownloadRequest> request)
{
    handle_t handle = request->handle;
    uri requestUri{request->requestUri};
    const Tone3000PkceParams &pkceParams = request->pkceParams;
    const std::string &downloadPath = request->downloadPath;

    Tone3000DownloadProgress progress;
    progress.handle(handle);

    try
    {
        if (listener)
        {
            listener->OnStartTone3000Download(progress.handle(), "Authenticating...");
        }
        progress.title("Authenticating...");
        bgUpdateDownloadProgress(progress);

        auto oAuthResult = handleOAuthCallback(requestUri, pkceParams);
        if (!oAuthResult.toneId.has_value())
        {
            throw std::runtime_error("Tone3000 Auth failed. tone ID not received.");
        }
        std::shared_ptr<Tone3000Download> download = GetTone3000Tone(oAuthResult.toneId.value());
    }
    catch (const Tone3000CancelledError &e)
    {
        OnTone3000DownloadCancelled(progress.handle());
        return;
    }
    catch (const std::exception &e)
    {
        OnTone3000DownloadError(progress.handle(), e.what());
        return;
    }
    OnTone3000DownloadComplete(progress.handle(), "");
}

Tone3000DownloaderImpl::OAuthCallbackResult Tone3000DownloaderImpl::handleOAuthCallback(
    const uri &callbackUri,
    const Tone3000PkceParams &pkce)
{
    OAuthCallbackResult result;

    try
    {
        std::map<std::string, std::string> params;
        std::string code = callbackUri.requiredQuery("code");
        std::optional<std::string> error = callbackUri.optionalQuery("error");
        std::string returnedState = callbackUri.requiredQuery("state");
        std::optional<std::string> toneId = callbackUri.optionalQuery("tone_id");
        std::optional<std::string> modelId = callbackUri.optionalQuery("model_id");
        bool canceled = callbackUri.query("canceled") == "true";
        if (error.has_value())
        {
            throw std::runtime_error(error.value());
        }

        if (canceled)
        {
            throw Tone3000CancelledError();
        }

        std::string codeVerifier = pkce.codeVerifier();

        // Access denied — e.g. model is private and user clicked "Back"
        if (error.has_value())
        {
            throw std::runtime_error(error.value());
        }

        if (code.empty() || codeVerifier.empty())
        {
            throw std::runtime_error("Tone3000 Auth: missing code.");
        }

        std::string body = HtmlFormBuilder({{"grant_type", "authorization_code"},
                                            {"code", code},
                                            {"code_verifier", codeVerifier},
                                            {"redirect_uri", pkce.redirectUrl()},
                                            {"client_id", pkce.publishableKey()}})
                               .build();

        auto postResult = Post(
            uri("http://www.tone3000.com/api/v1/oauth/token"),
            {{"Content-Type: application/x-www-form-urlencoded"}},
            body);
        if (!postResult.ok)
        {
            throw std::runtime_error(postResult.error);
        }
        Tone3000AuthResponse authResponse(postResult.body);
        this->accessTokens = std::make_shared<Tone3000AccessTokens>(authResponse);

        OAuthCallbackResult result;
        if (toneId.has_value())
        {
            result.toneId = ParseIntegerOrThrow<int64_t>(toneId.value(), "tone_id");
        }
        if (modelId.has_value())
        {
            result.modelId = ParseIntegerOrThrow<int64_t>(modelId.value(), "model_id");
        }
        return result;
    }
    catch (const std::exception &e)
    {
        throw;
    }
}


std::string Tone3000DownloaderImpl::GetBearerToken() const
{
    if (!accessTokens)
    {
        throw std::runtime_error("Tone3000 auth failure. No access token.");
    }
    return accessTokens->access_token();
}


std::string Tone3000DownloaderImpl::Tone3000GetText(
    const uri &requestedUri
)
{
    TemporaryFile tempFile{WEB_TEMP_DIR};

    std::vector<std::string> inputHeaders {
        SS("Authorization: Bearer " << this->GetBearerToken())  
    };
    int httpCode = CurlGet(requestedUri.str(),tempFile.Path(),nullptr,&inputHeaders);
    if (httpCode != 200)
    {
        throw std::runtime_error(
            SS("Http error " << HtmlHelper::httpErrorString(httpCode)));
    }
    std::string result;

    std::ifstream ifs(tempFile.Path(), std::ios::binary);
    if (!ifs)
    {
        throw std::runtime_error("Tone3000: failed to open response file.");
    }
    result.assign(std::istreambuf_iterator<char>(ifs), std::istreambuf_iterator<char>());
    return result;
}

Tone3000DownloaderImpl::PostResult Tone3000DownloaderImpl::Post(
    const uri &requestedUri,
    std::vector<std::string> headers,
    const std::string &body)
{
    PostResult result;
    try
    {
        int response = CurlPostStrings(
            requestedUri.str(),
            body,
            result.body,
            nullptr,
            &headers

        );
        result.errorCode = response;
        if (response != 200)
        {
            result.ok = false;
            result.error = SS("HTPP Error " << response);
        }
        else
        {
            result.ok = true;
        }
    }
    catch (const std::exception &e)
    {
        result.ok = false;
        result.error = e.what();
    }
    return result;
}

Tone3000AuthResponse::Tone3000AuthResponse(const std::string &responseBody)
{
    std::stringstream ss(responseBody);
    json_reader reader(ss);
    reader.read(this);
    expires_at_ = clock_t::now() + std::chrono::seconds(expires_in_);
}

Tone3000AccessTokens::Tone3000AccessTokens(const Tone3000AuthResponse &authResponse)
    : access_token_(authResponse.access_token()),
      expires_in_(authResponse.expires_in()),
      refresh_token_(authResponse.refresh_token()),
      token_type_(authResponse.token_type())
{
}

Tone3000DownloaderImpl::handle_t Tone3000DownloaderImpl::RequestTone3000Download(
    const uri &uri,
    const Tone3000PkceParams &pkceParams,
    const std::string &downloadPath,
    Tone3000DownloadType downloadType)
{
    handle_t handle;
    {
        std::lock_guard<std::mutex> lockGuard{this->mutex};
        handle = NextHandle();
        std::shared_ptr<DownloadRequest> request =
            std::make_shared<DownloadRequest>(handle, uri.str(), pkceParams, downloadPath, downloadType);

        this->requestQueue.Put(request);
        request = nullptr;
        if (!this->thread)
        {
            this->thread = std::make_unique<std::jthread>([this]
                                                          { this->ThreadProc(); });
        }
    }
    return handle;
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
static std::string mdUser(const std::string &user)
{
    return mdSanitize(user);
}

static void mdLink(std::ostream &f, const std::string &label, const std::string &url)
{
    f << "[" << mdSanitize(label, "]") << "](" << mdSanitize(url, ")") << ")";
}

static void writeReadme(const fs::path &path, const std::string &thumbnailUrl, const Tone3000FileDownloadRequest &download)
{
    //     std::ofstream of{path};
    //     if (!of.is_open())
    //     {
    //         throw std::runtime_error(SS("Unable to create file " << path));
    //     }

    //     // clang-format off
    //     of <<

    //             "<div id='tone3000_float'  >\n";
    //     if (!thumbnailUrl.empty())
    //     {

    //         of <<  "    <img id='tone3000_thumbnail' src='"
    //                         << thumbnailUrl << "' alt=''  />\n";
    //     }
    //     of  <<  "    <div id='tone3000_float_content' >\n"
    //         <<  "        <h3 id='tone3000_title'>" << mdSanitize(download.title()) << "</h3>\n"
    //         <<  "        <div>\n"
    //         <<  "            <p id='tone3000_subtitle'>\n"
    //                         << mdDate(download.updated_at())
    //                         << " "
    //                         << mdUser(download.user())
    //         <<  "            <br/>\n"
    //         <<  "            ";
    //         if (download.sizes().size() != 0) {
    //             of
    //                 << mdSizes(download.sizes()) << ", "
    //             ;
    //         }
    //         if (download.platform() == "ir")
    //         {
    //             of
    //                     << (mdPlatform(download.platform()));
    //         } else {
    //             of
    //                     << mdGear(download.gear()) << ", "
    //                     << mdPlatform(download.platform());
    //         }
    //         of           << "<br/>\n";
    //         if (download.license() != "") {
    //             of <<  "            License: " << mdLicense(download.license()) << "\n";
    //         }
    //         of
    // //        << "          " << mdTestLicenses()
    //         <<  "        </p>\n"
    //         <<  "        </div>\n"
    //         <<  "    </div>\n"
    //         << "</div>\n";
    //     // clang-format on
    //     of << "\n\n";
    //     mdEscapeString(of, download.description());

    //     if (download.links().size() > 0)
    //     {
    //         of << std::endl;
    //         of << "## Links" << std::endl
    //            << std::endl;
    //         for (auto &link : download.links())
    //         {
    //             of << "- " << "[" << link << "](" << link << ")" << std::endl;
    //         }
    //     }
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
std::string Tone3000DownloaderImpl::DownloadTone3000Files(
    const Tone3000FileDownloadRequest &request,
    Tone3000DownloadProgress &progress)
{

    // fs::path bundlePath = fs::path(request.downloadPath()) / StringToSafeFilename(request.title());
    // fs::path resultDirectory = bundlePath;

    // fs::create_directories(bundlePath);

    // std::vector<CurlDownloadRequest> requests;

    // auto&models = request.models();

    // std::vector<std::string> headers; //  yyy: make sure we have a user agent header!

    // headers.push_back(
    //     SS("Authorization: Bearer " << request.authToken())
    // );
    // try
    // {
    //     size_t downloadIndex = 0;
    //     while (downloadIndex < models.size())
    //     {
    //         if (isCancelled())
    //         {
    //             throw std::runtime_error("Cancelled");
    //         }
    //         size_t thisTime = models.size() - downloadIndex;
    //         thisTime = Tone3000Throttler::instance().ReserveDownloadSlots(thisTime);
    //         if (thisTime != 0)
    //         {
    //             std::vector<CurlDownloadRequest> requestsThisTime;
    //             for (size_t i = 0; i < thisTime; ++i)
    //             {
    //                 auto &model = models[downloadIndex+i];

    //                 requestsThisTime.push_back(
    //                     CurlDownloadRequest{
    //                         url: model.url(),
    //                         outputFile: bundlePath / StringToSafeFilename(model.name())
    //                     });
    //             }

    //             int result = CurlGet(
    //                 requestsThisTime,
    //                 [&](size_t completed, size_t total)
    //                 {
    //                     if (isCancelled())
    //                     {
    //                         return false;
    //                     }
    //                     progress.progress(completed + downloadIndex);
    //                     progress.total(requests.size());
    //                     progressCallback(progress);
    //                     return true;
    //                 },
    //                 &headers);

    //             if (isCancelled())
    //             {
    //                 throw std::runtime_error("Cancelled");
    //             }

    //             if (result != 200)
    //             {
    //                 throw std::runtime_error(SS("Server download failed. HTTP Error " << HtmlHelper::httpErrorString(result)));
    //             }

    //             downloadIndex += thisTime;
    //         }
    //         if (thisTime <= 1)
    //         {
    //             CancellableSleep(std::chrono::milliseconds(1000), isCancelled);
    //         }
    //     }

    //     fs::path readmePath = bundlePath / "README.md";

    //     std::string thumbnailUrl;
    //     if (!request.imageUrl().empty())
    //     {

    //         TemporaryFile imageFile{TONE3000_THUMBNAIL_PATH};

    //         std::string imageUrl = request.imageUrl();
    //         std::vector<std::string> imageHeaders;
    //         try
    //         {
    //             size_t reserveCount;
    //             while (true)
    //             {
    //                 reserveCount = Tone3000Throttler::instance().ReserveDownloadSlots(1);
    //                 if (reserveCount != 0)
    //                 {
    //                     break;
    //                 }
    //                 std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    //                 if (isCancelled())
    //                 {
    //                     throw std::runtime_error("Cancelled");
    //                 }
    //             }
    //             if (CurlGet(imageUrl, imageFile.Path(), &imageHeaders) == 200)
    //             {
    //                 std::string mediaType = GetHeader("Content-Type", imageHeaders);
    //                 std::string extension;
    //                 if (mediaType == "image/jpeg")
    //                 {
    //                     extension = ".jpeg";
    //                 }
    //                 else if (mediaType == "image/webp")
    //                 {
    //                     extension = ".webp";
    //                 }
    //                 else if (mediaType == "image/png")
    //                 {
    //                     extension = ".png";
    //                 }
    //                 else
    //                 {
    //                     Lv2Log::warning("Tone3000 thumbnail has an unexpected media type of '%s'", mediaType.c_str());
    //                     throw std::runtime_error("Invalid thumbnail media type.");
    //                 }
    //                 if (mediaType == "image/jpeg")
    //                 {
    //                     fs::path thumnbailTempPath = imageFile.Detach();
    //                     fs::path finalPath = TONE3000_THUMBNAIL_PATH / SS(request.id() << extension);
    //                     if (fs::exists(finalPath))
    //                     {
    //                         fs::remove(finalPath);
    //                     }
    //                     fs::create_directories(finalPath.parent_path());
    //                     fs::rename(thumnbailTempPath, finalPath);
    //                     thumbnailUrl = SS("var/tone3000_thumbnail?id=" << request.id());
    //                 }
    //             }
    //         }
    //         catch (const std::exception &)
    //         {
    //             // ignore.
    //         }
    //     }
    //     writeReadme(readmePath, thumbnailUrl, request);
    // }
    // catch (const std::exception &e)
    // {
    //     CleanUpFailedDownload(bundlePath, requests);
    //     if (isCancelled())
    //     {
    //         return "";
    //     }
    //     throw;
    // }

    // return resultDirectory;
    throw std::runtime_error("Not implemented");
}


std::shared_ptr<Tone3000Download> Tone3000DownloaderImpl::GetTone3000Tone(
    int64_t toneId)
{
    std::string jsonResult = Tone3000GetText(SS("https://www.tone3000.com/api/v1/tones/" << toneId));
    std::shared_ptr<Tone3000Download> tone = std::make_shared<Tone3000Download>(jsonResult);
    
    return tone;
}

//////////////////////////////////////////////////////////////////////////////////

JSON_MAP_BEGIN(Tone3000PkceParams)
JSON_MAP_REFERENCE(Tone3000PkceParams, publishableKey)
JSON_MAP_REFERENCE(Tone3000PkceParams, redirectUrl)
JSON_MAP_REFERENCE(Tone3000PkceParams, codeVerifier)
JSON_MAP_REFERENCE(Tone3000PkceParams, codeChallenge)
JSON_MAP_REFERENCE(Tone3000PkceParams, state)
JSON_MAP_END();

JSON_MAP_BEGIN(Tone3000ModelInfo)
JSON_MAP_REFERENCE(Tone3000ModelInfo, url)
JSON_MAP_REFERENCE(Tone3000ModelInfo, name)
JSON_MAP_END();

// JSON_MAP_BEGIN(Tone3000FileDownloadRequest)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, downloadType)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, downloadPath)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, id)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, codeVerifier)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, state)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, codeChallenge)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, authToken)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, title)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, description)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, imageUrl)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, models)

// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, updated_at)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, user)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, sizes)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, platform)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, gear)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, license)
// JSON_MAP_REFERENCE(Tone3000FileDownloadRequest, links)
// JSON_MAP_END();

JSON_MAP_BEGIN(Tone3000AuthResponse)
JSON_MAP_REFERENCE(Tone3000AuthResponse, access_token)
JSON_MAP_REFERENCE(Tone3000AuthResponse, expires_in)
JSON_MAP_REFERENCE(Tone3000AuthResponse, refresh_token)
JSON_MAP_REFERENCE(Tone3000AuthResponse, token_type)
JSON_MAP_REFERENCE(Tone3000AuthResponse, tone_id)
JSON_MAP_REFERENCE(Tone3000AuthResponse, model_id)
JSON_MAP_REFERENCE(Tone3000AuthResponse, canceled)
JSON_MAP_END()
