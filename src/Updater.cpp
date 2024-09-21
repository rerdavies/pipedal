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

#include "Updater.hpp"
#include "util.hpp"
#include "json.hpp"
#include "config.hpp"
#include <sys/eventfd.h>
#include <unistd.h>
#include <stdexcept>
#include <chrono>
#include <poll.h>
#include "Lv2Log.hpp"
#include "SysExec.hpp"
#include "json_variant.hpp"
#include "ss.hpp"
#include "Lv2Log.hpp"
#include <algorithm>
#include "UpdaterSecurity.hpp"
#include "SysExec.hpp"
#include "ofstream_synced.hpp"
#include <fstream>
#include "TemporaryFile.hpp"
#include <limits>

using namespace pipedal;
namespace fs = std::filesystem;

#undef TEST_UPDATE

#ifndef DEBUG
#undef TEST_UPDATE // do NOT leat this leak into a production build!
#endif

namespace pipedal
{
    class GithubResponseHeaders
    {
    public:
        GithubResponseHeaders() {}
        GithubResponseHeaders(const std::filesystem::path path);
        int code_ = -1;
        std::chrono::system_clock::time_point date_;
        std::chrono::system_clock::time_point ratelimit_reset_;
        uint64_t ratelimit_limit_ = 60;
        uint64_t ratelimit_remaining_ = 60;
        uint64_t ratelimit_used_ = 0;
        std::string ratelimit_resource_;
        bool limit_exceeded() const { return code_ != 200 && ratelimit_limit_ != 0 && ratelimit_limit_ == ratelimit_used_; }
        void Load(const std::filesystem::path &filename);
        void Save(const std::filesystem::path &filename);
        DECLARE_JSON_MAP(GithubResponseHeaders);

    private:
        static std::filesystem::path FILENAME;
    };
}

class pipedal::UpdaterImpl : public Updater
{
public:
    UpdaterImpl(const std::filesystem::path &workingDirectory);
    UpdaterImpl() : UpdaterImpl("/var/pipedal/updates") {}

    virtual ~UpdaterImpl() noexcept;

    virtual void SetUpdateListener(UpdateListener &&listener) override;
    virtual void Start() override;
    virtual void Stop() override;

    virtual void CheckNow() override;

    virtual UpdatePolicyT GetUpdatePolicy() override;
    virtual void SetUpdatePolicy(UpdatePolicyT updatePolicy) override;
    virtual void ForceUpdateCheck() override;
    virtual void DownloadUpdate(const std::string &url, std::filesystem::path *file, std::filesystem::path *signatureFile) override;
    virtual UpdateStatus GetCurrentStatus() const override { return this->currentResult; }
    virtual UpdateStatus GetReleaseGeneratorStatus() override;

private:
    std::filesystem::path workingDirectory;
    std::filesystem::path updateStatusCacheFile;
    std::filesystem::path githubResponseHeaderFilename;

    GithubResponseHeaders githubResponseHeaders;

    using clock = std::chrono::steady_clock;

    void SetCachedUpdateStatus(UpdateStatus &updateStatus);

    UpdateStatus GetCachedUpdateStatus();

    void RetryAfter(clock::duration delay);

    template <typename REP, typename PERIOD>
    void RetryAfter(std::chrono::duration<REP, PERIOD> duration)
    {
        RetryAfter(std::chrono::duration_cast<clock::duration>(duration));
    }

    void SaveRetryTime(const std::chrono::system_clock::time_point &time);
    clock::time_point LoadRetryTime();

    std::string GetUpdateFilename(const std::string &url);
    std::string GetSignatureUrl(const std::string &url);

    UpdatePolicyT updatePolicy = UpdatePolicyT::ReleaseOrBeta;
    using UpdateReleasePredicate = std::function<bool(const GithubRelease &githubRelease)>;

    UpdateRelease getUpdateRelease(
        const std::vector<GithubRelease> &githubReleases,
        const std::string &currentVersion,
        const UpdateReleasePredicate &predicate);

    UpdateStatus cachedUpdateStatus;
    bool stopped = false;

    int event_reader = -1;
    int event_writer = -1;
    void ThreadProc();
    UpdateStatus DoUpdate(bool forReleaseGenerator);
    void CheckForUpdate(bool useCache);
    UpdateListener listener;

    std::unique_ptr<std::thread> thread;
    std::mutex mutex;

    bool hasInfo = false;
    UpdateStatus currentResult;
    static clock::duration updateRate;
    clock::time_point updateRetryTime;
};
Updater::ptr Updater::Create()
{
    return std::make_unique<UpdaterImpl>();
}
Updater::ptr Updater::Create(const std::filesystem::path &workingDirectory)
{
    return std::make_unique<UpdaterImpl>(workingDirectory);
}

static constexpr uint64_t CLOSE_EVENT = 0;
static constexpr uint64_t CHECK_NOW_EVENT = 1;
static constexpr uint64_t UNCACHED_CHECK_NOW_EVENT = 2;

UpdaterImpl::clock::duration UpdaterImpl::updateRate = std::chrono::duration_cast<UpdaterImpl::clock::duration>(std::chrono::days(1));
static std::chrono::system_clock::duration CACHE_DURATION = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::minutes(30));

std::mutex cacheMutex;
UpdateStatus UpdaterImpl::GetCachedUpdateStatus()
{
    std::lock_guard lock{cacheMutex};
    try
    {
        if (std::filesystem::exists(updateStatusCacheFile))
        {
            std::ifstream f{updateStatusCacheFile};
            if (f.is_open())
            {
                json_reader reader(f);
                UpdateStatus status;
                reader.read(&status);

                // cached curruent version might come from a different version.
                status.ResetCurrentVersion();
                return status;
            }
        }
    }
    catch (const std::exception &e)
    {
        Lv2Log::error(SS("Unable to read cached UpdateStatus. " << e.what()));
    }
    return UpdateStatus();
}

void UpdaterImpl::SetCachedUpdateStatus(UpdateStatus &updateStatus)
{
    std::lock_guard lock{cacheMutex};
    updateStatus.LastUpdateTime(std::chrono::system_clock::now());
    try
    {
        pipedal::ofstream_synced f{updateStatusCacheFile};
        json_writer writer{f};
        writer.write(updateStatus);
    }
    catch (const std::exception &e)
    {
        Lv2Log::error(SS("Unable to write cached UpdateStatus. " << e.what()));
    }
}

UpdaterImpl::UpdaterImpl(const std::filesystem::path &workingDirectory)
    : workingDirectory(workingDirectory),
      updateStatusCacheFile(workingDirectory / "updateStatus.json"),
      githubResponseHeaderFilename(workingDirectory / "githubHeaders.json")
{
    this->githubResponseHeaders.Load(githubResponseHeaderFilename);
    cachedUpdateStatus = GetCachedUpdateStatus();
    this->updatePolicy = cachedUpdateStatus.UpdatePolicy();
    currentResult = cachedUpdateStatus;
}
void UpdaterImpl::Start()
{
    #if !ENABLE_AUTO_UPDATE
    return;
    #endif
    int fds[2];
    int rc = pipe(fds);
    if (rc != 0)
    {
        throw std::runtime_error("Updater: cant create event pipe.");
    }
    this->event_reader = fds[0];
    this->event_writer = fds[1];

    this->thread = std::make_unique<std::thread>([this]()
                                                 { ThreadProc(); });
    CheckNow();
}
UpdaterImpl::~UpdaterImpl()
{
    try
    {
        Stop();
    }
    catch (const std::exception & /*ignored*/)
    {
    }
}
void UpdaterImpl::Stop()
{
    if (stopped)
    {
        return;
    }
    #if !ENABLE_AUTO_UPDATE
    return;
    #endif
    stopped = true;

    if (event_writer != -1)
    {
        uint64_t value = CLOSE_EVENT;
        write(this->event_writer, &value, sizeof(uint64_t));
    }
    if (thread)
    {
        thread->join();
        thread = nullptr;
    }
    if (event_reader != -1)
    {
        close(event_reader);
    }
    if (event_writer != -1)
    {
        close(event_writer);
    }
}

void UpdaterImpl::CheckNow()
{
    uint64_t value = CHECK_NOW_EVENT;
    write(this->event_writer, &value, sizeof(uint64_t));
}

void UpdaterImpl::SetUpdateListener(UpdateListener &&listener)
{
    std::lock_guard lock{mutex};
    this->listener = listener;
    if (hasInfo)
    {
        listener(currentResult);
    }
}

static void LogNextStartTime(const std::chrono::steady_clock::time_point &clockTime)
{
    using namespace std::chrono;
    // convert to system_clock;
    auto delay = clockTime - steady_clock::now();
    auto systemTime = system_clock::now() + duration_cast<system_clock::duration>(delay);
    auto t = system_clock::to_time_t(systemTime);
    Lv2Log::info(SS("Updater: Next update check at " << std::put_time(std::localtime(&t), "%Y-%m-%d %H:%M:%S")));
}
void UpdaterImpl::ThreadProc()
{
    SetThreadName("UpdateMonitor");
    struct pollfd pfd;
    pfd.fd = this->event_reader;
    pfd.events = POLLIN;

    auto lastRetryTime = clock::time_point();
    this->updateRetryTime = LoadRetryTime();
    try
    {
        while (true)
        {
            if (updateRetryTime != lastRetryTime) // has retry time changed?
            {
                lastRetryTime = updateRetryTime;
                if (updateRetryTime > clock::now())
                {
                    LogNextStartTime(updateRetryTime);
                }
            }
            auto clockDelay = this->updateRetryTime - clock::now();
            auto delayMs = std::chrono::duration_cast<std::chrono::milliseconds>(clockDelay).count();
            if (delayMs > std::numeric_limits<int>::max()) // could be as little as 32 seconds on a 16-bit int system.
            {
                delayMs = std::numeric_limits<int>::max();
            }

            int ret = poll(&pfd, 1, (int)delayMs); // 1000 ms timeout

            if (ret == -1)
            {
                Lv2Log::error("Updater: Poll error.");
                break;
            }
            else if (ret == 0)
            {
                if (clock::now() >= this->updateRetryTime)
                {
                    CheckForUpdate(true);
                }
            }
            else
            {
                // Event occurred
                uint64_t value;
                ssize_t s = read(event_reader, &value, sizeof(uint64_t));
                if (s == sizeof(uint64_t))
                {
                    if (value == CHECK_NOW_EVENT)
                    {

                        CheckForUpdate(true);
                    }
                    else if (value == UNCACHED_CHECK_NOW_EVENT)
                    {
                        CheckForUpdate(false);
                    }
                    else
                    {
                        break;
                    }
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        Lv2Log::error(SS("Updater: Service thread terminated abnormally: " << e.what()));
    }
}

namespace pipedal
{
    class GithubAsset
    {
    public:
        GithubAsset(json_variant &v);
        std::string name;
        std::string browser_download_url;
        std::string updated_at;
    };
    class GithubRelease
    {
    public:
        GithubRelease(json_variant &v);

        const GithubAsset *GetDownloadForCurrentArchitecture() const;
        const GithubAsset *GetGpgKeyForAsset(const std::string &name) const;

        bool draft = true;
        bool prerelease = true;
        std::string name;
        std::string url;
        std::string version;
        std::string body;
        std::vector<GithubAsset> assets;
        std::string published_at;
    };
}

GithubAsset::GithubAsset(json_variant &v)
{
    auto o = v.as_object();
    this->name = o->at("name").as_string();
    this->browser_download_url = o->at("browser_download_url").as_string();
    this->updated_at = o->at("updated_at").as_string();
}
GithubRelease::GithubRelease(json_variant &v)
{
    auto o = v.as_object();
    this->name = o->at("name").as_string();
    this->draft = o->at("draft").as_bool();
    this->prerelease = o->at("prerelease").as_bool();
    this->body = o->at("body").as_string();

    auto assets = o->at("assets").as_array();
    for (size_t i = 0; i < assets->size(); ++i)
    {
        auto &el = assets->at(i);
        this->assets.push_back(GithubAsset(el));
    }
    this->published_at = o->at("published_at").as_string();
}

static std::vector<std::string> split(const std::string &s, char delimiter)
{
    std::vector<std::string> tokens;
    std::string token;
    std::istringstream tokenStream(s);
    while (std::getline(tokenStream, token, delimiter))
    {
        tokens.push_back(token);
    }
    return tokens;
}
static std::string justTheVersion(const std::string &assetName)
{
    // eg. pipedal_1.2.41_arm64.deb
    auto t = split(assetName, '_');
    if (t.size() != 3)
    {
        throw std::runtime_error("Unable to parse version.");
    }
    return t[1];
}

static int compareVersions(const std::string &l, const std::string &r)
{
    std::stringstream sl(l);
    std::stringstream sr(r);

    int majorL = -1, majorR = 1, minorL = -1,
        minorR = -1, buildL = -1, buildR = -1;
    sl >> majorL;
    sr >> majorR;
    if (majorL != majorR)
    {
        return (majorL < majorR) ? -1 : 1;
    }
    char discard;
    sl >> discard >> minorL;
    sr >> discard >> minorR;
    if (minorL != minorR)
    {
        return minorL < minorR ? -1 : 1;
    }
    sl >> discard >> buildL;
    sr >> discard >> buildR;

    if (buildL != buildR)
    {
        return buildL < buildR ? -1 : 1;
    }
    return 0;
}
static std::string normalizeReleaseName(const std::string &releaseName)
{
    // e.g.   "PiPedal 1.2.34 Release" -> "PiPedal v1.2.34-Release"
    if (releaseName.empty())
        return "";

    std::string result = releaseName;

    auto nPos = result.find(' ');
    if (nPos == std::string::npos)
    {
        return result;
    }
    ++nPos;
    if (nPos >= result.length())
    {
        return result;
    }
    char c = releaseName[nPos];
    if (c >= '0' && c <= '9')
    {
        result.insert(result.begin() + nPos, 'v');
    }
    nPos = result.find(' ', nPos);
    if (nPos != std::string::npos)
    {
        result.at(nPos) = '-';
    }
    return result;
}

UpdateRelease UpdaterImpl::getUpdateRelease(
    const std::vector<GithubRelease> &githubReleases,
    const std::string &currentVersion,
    const UpdateReleasePredicate &predicate)
{
    for (const auto &githubRelease : githubReleases)
    {
        auto *asset = githubRelease.GetDownloadForCurrentArchitecture();
        if (!asset)
            continue;
        auto *pgpKey = githubRelease.GetGpgKeyForAsset(asset->name);
        if (!pgpKey)
        {
            continue;
        }

        if (!predicate(githubRelease))
            continue;
        UpdateRelease updateRelease;
        updateRelease.upgradeVersion_ = justTheVersion(asset->name);
        updateRelease.updateAvailable_ = compareVersions(currentVersion, updateRelease.upgradeVersion_) < 0;
        updateRelease.upgradeVersionDisplayName_ = normalizeReleaseName(githubRelease.name);
        updateRelease.assetName_ = asset->name;
        updateRelease.updateUrl_ = asset->browser_download_url;
        updateRelease.gpgSignatureUrl_ = pgpKey->browser_download_url;
        return updateRelease;
    }
    return UpdateRelease();
}

static void CheckUpdateHttpResponse(std::string errorCode)
{
    if (errorCode.starts_with("%"))
    {
        errorCode = errorCode.substr(1);
    }
    int code = -999;
    {
        std::istringstream ss{errorCode};
        ss >> code;
    }
    if (code == -999)
    {
        throw std::runtime_error(SS("Invalid curl response: " << errorCode));
    }
    if (code == 200)
    {
        return;
    }
    if (code == 0)
    {
        throw std::runtime_error("PiPedal server can't reach the internet.");
    }

    {
        std::string message = SS("HTTP error " << code << "");
        throw std::runtime_error(message);
    }
}

static std::chrono::system_clock::time_point http_date_to_time_point(const std::string &http_date)
{
    std::tm tm = {};
    std::istringstream ss(http_date);

    ss >> std::get_time(&tm, "%a, %d %b %Y %H:%M:%S GMT");

    if (ss.fail())
    {
        throw std::runtime_error("Failed to parse HTTP date: " + http_date);
    }
    auto tt = timegm(&tm);
    return std::chrono::system_clock::from_time_t(tt);
}

JSON_MAP_BEGIN(GithubResponseHeaders)
JSON_MAP_REFERENCE(GithubResponseHeaders, date)
JSON_MAP_REFERENCE(GithubResponseHeaders, ratelimit_reset)
JSON_MAP_REFERENCE(GithubResponseHeaders, ratelimit_limit)
JSON_MAP_REFERENCE(GithubResponseHeaders, ratelimit_remaining)
JSON_MAP_REFERENCE(GithubResponseHeaders, ratelimit_used)
JSON_MAP_REFERENCE(GithubResponseHeaders, ratelimit_resource)
JSON_MAP_END();

void GithubResponseHeaders::Load(const std::filesystem::path &FILENAME)
{
    std::ifstream f(FILENAME);
    if (f.is_open())
    {
        try
        {
            json_reader reader(f);
            reader.read(this);
        }
        catch (const std::exception &e)
        {
            Lv2Log::error(SS("Invalid file format: " << FILENAME << "."));
            *this = GithubResponseHeaders();
        }
    }
}
void GithubResponseHeaders::Save(const std::filesystem::path &FILENAME)
{
    std::filesystem::create_directories(FILENAME.parent_path());
    ofstream_synced f(FILENAME);
    if (!f.is_open())
    {
        Lv2Log::error(SS("Can't write to " << FILENAME << "."));
    }
    else
    {
        json_writer writer(f);
        writer.write(this);
    }
}

GithubResponseHeaders::GithubResponseHeaders(const std::filesystem::path path)
{
    // HTTP/2 403
    // date: Fri, 13 Sep 2024 17:31:22 GMT
    //...
    // x-ratelimit-limit: 60
    // x-ratelimit-remaining: 0
    // x-ratelimit-reset: 1726250807
    // x-ratelimit-resource: core
    // x-ratelimit-used: 60
    std::ifstream f{path};
    if (!f.is_open())
    {
        throw std::runtime_error(SS("Can't open file " << path << "."));
    }
    std::string http;
    f >> http >> code_;
    std::string line;
    std::getline(f, line);

    while (true)
    {
        std::getline(f, line);
        auto trimPos = line.find_last_not_of("\r\n");
        if (trimPos != std::string::npos)
        {
            line = line.substr(0, trimPos + 1);
        }

        if (!f)
            break;
        auto pos = line.find(':');
        if (pos != std::string::npos)
        {
            std::string tag = line.substr(0, pos);
            ++pos;
            while (pos < line.length() && line[pos] == ' ')
            {
                ++pos;
            }
            std::string value = line.substr(pos);
            uint64_t *pResult = nullptr;
            if (tag == "date")
            {
                this->date_ = http_date_to_time_point(value);
            }
            else if (tag == "x-ratelimit-limit")
            {
                pResult = &ratelimit_limit_;
            }
            else if (tag == "x-ratelimit-remaining")
            {
                pResult = &ratelimit_remaining_;
            }
            else if (tag == "x-ratelimit-reset")
            {
                std::istringstream ss{value};
                uint64_t reset = 0;
                ss >> reset;
                auto secs = std::chrono::seconds(reset); // value is seconds in UTC Unix epoch.
                this->ratelimit_reset_ = std::chrono::system_clock::time_point(secs);
            }
            else if (tag == "x-ratelimit-resource")
            {
                ratelimit_resource_ = value;
            }
            else if (tag == "x-ratelimit-used")
            {
                pResult = &ratelimit_used_;
            }
            if (pResult)
            {
                std::istringstream ss{value};
                ss >> *pResult;
            }
        }
    }
}

UpdateStatus UpdaterImpl::GetReleaseGeneratorStatus()
{
    UpdateStatus result = DoUpdate(true);
    return result;
}

void UpdaterImpl::CheckForUpdate(bool useCache)
{
    UpdateStatus updateResult;

    // if we've used up too many actual github requests, used the cached version even if the request is non-cached.
    if (githubResponseHeaders.ratelimit_remaining_ < 3 * githubResponseHeaders.ratelimit_limit_ / 4)
    {
        useCache = true;
    }

    if (useCache)
    {
        if (clock::now() < this->updateRetryTime)
        {
            std::lock_guard lock{mutex};
            {
                this->currentResult = cachedUpdateStatus;
                this->currentResult.UpdatePolicy(this->updatePolicy);
                if (listener)
                {
                    listener(this->currentResult);
                }
                return;
            }
        }
    }


    try
    {

        updateResult = DoUpdate(false);
    }
    catch (const std::exception &e)
    {
        Lv2Log::error(SS("Updater: Failed to fetch update info. " << e.what()));
        updateResult.errorMessage_ = e.what();
        updateResult.isValid_ = false;
        updateResult.isOnline_ = false;
    }
    {
        std::lock_guard lock{mutex};
        updateResult.UpdatePolicy(this->updatePolicy);
        if (updateResult.isOnline_ && updateResult.isValid_)
        {
            this->currentResult = updateResult;
        }
        SetCachedUpdateStatus(this->currentResult);
        if (listener)
        {
            listener(this->currentResult);
        }
    }
}

UpdateStatus UpdaterImpl::DoUpdate(bool forReleaseGenerator)
{
    UpdateStatus updateResult;
    if (forReleaseGenerator)
    {
        updateResult.currentVersion_ = "0.0.0";
    }

    // set default next retry time.  github failures may postpone even further.

    Lv2Log::info("Checking for updates.");
    // const std::string responseOption = "-w \"%{response_code}\"";
#ifdef WIN32
    responseOption = "-w \"%%{response_code}\""; // windows shell requires doubling of the %%.
#endif

    TemporaryFile headerFile{workingDirectory};
    std::string args = SS("-s -L " << GITHUB_RELEASES_URL << " -D " << headerFile.str());

    updateResult.errorMessage_ = "";

    auto result = sysExecForOutput("curl", args);
    if (result.exitCode != EXIT_SUCCESS)
    {
        RetryAfter(std::chrono::duration_cast<clock::duration>(std::chrono::hours(4)));
        throw std::runtime_error("Server has no internet access.");
    }
    else
    {
        // hard throttling for github.
        RetryAfter(std::chrono::duration_cast<clock::duration>(std::chrono::hours(8)));

        GithubResponseHeaders githubHeaders{headerFile.Path()};
        this->githubResponseHeaders = githubHeaders;
        this->githubResponseHeaders.Save(githubResponseHeaderFilename);

        if (githubHeaders.code_ != 200)
        {
            if (
                githubHeaders.ratelimit_limit_ != 0 &&
                githubHeaders.ratelimit_limit_ == githubHeaders.ratelimit_used_)
            {
                std::time_t time = std::chrono::system_clock::to_time_t(githubHeaders.ratelimit_reset_);
                std::string strTime = std::ctime(&time);

                this->RetryAfter(githubHeaders.ratelimit_reset_ - githubHeaders.date_);
                throw std::runtime_error(SS("Github API rate limit exceeded. Retrying at " << strTime));
            }
        }

        if (result.output.length() == 0)
        {
            throw std::runtime_error("Server has no internet access.");
        }
        std::stringstream ss(result.output);
        json_reader reader(ss);
        json_variant vResult(reader);

        if (vResult.is_object())
        {
            // an HTML error.
            updateResult.isOnline_ = false;
            auto o = vResult.as_object();
            std::string message = "Unknown error.";
            if (o->at("message").is_string())
            {
                message = o->at("message").as_string();
            }
            throw std::runtime_error(SS("Github Service error: " << message));
        }
        else if (!vResult.is_array())
        {
            throw std::runtime_error("Invalid file format error.");
        }
        else
        {
            json_variant::array_ptr vArray = vResult.as_array();

            std::vector<GithubRelease> releases;
            for (size_t i = 0; i < vArray->size(); ++i)
            {
                auto &el = vArray->at(i);
                GithubRelease release{el};
                if (!release.draft && release.GetDownloadForCurrentArchitecture() != nullptr)
                {
                    if (release.name.find("Experimental") == std::string::npos) // experimental releases do not participate in auto-updates (not even for dev stream)
                    {
                        releases.push_back(std::move(release));
                    }
                }
            }
            std::sort(
                releases.begin(),
                releases.end(),
                [](const GithubRelease &left, const GithubRelease &right)
                {
                    return left.published_at > right.published_at; // latest date first.
                });
            updateResult.releaseOnlyRelease_ = getUpdateRelease(
                releases,
                updateResult.currentVersion_,
                [](const GithubRelease &githubRelease)
                {
                    return !githubRelease.prerelease &&
                           githubRelease.name.find("Release") != std::string::npos;
                });

            updateResult.releaseOrBetaRelease_ = getUpdateRelease(
                releases,
                updateResult.currentVersion_,
                [](const GithubRelease &githubRelease)
                {
                    return !githubRelease.prerelease &&
                           (githubRelease.name.find("Release") != std::string::npos ||
                            githubRelease.name.find("Beta") != std::string::npos);
                });
            updateResult.devRelease_ = getUpdateRelease(
                releases,
                updateResult.currentVersion_,
                [](const GithubRelease &githubRelease)
                {
                    return true;
                });
#ifdef TEST_UPDATE
            updateResult.releaseOrBetaRelease_.upgradeVersionDisplayName_ = "PiPedal v1.2.41-Beta";
            updateResult.devRelease_.upgradeVersionDisplayName_ = "PiPedal v1.2.39-Experimental";
            updateResult.devRelease_.upgradeVersion_ = "1.2.39";
            updateResult.devRelease_.updateAvailable_ = false;
#endif
            updateResult.isValid_ = true;
            updateResult.isOnline_ = true;
        }
    }
    return updateResult;
}
bool UpdateRelease::operator==(const UpdateRelease &other) const
{
    return (updateAvailable_ == other.updateAvailable_) &&
           (upgradeVersion_ == other.upgradeVersion_) &&
           (upgradeVersionDisplayName_ == other.upgradeVersionDisplayName_) &&
           (assetName_ == other.assetName_) &&
           (updateUrl_ == other.updateUrl_);
}

bool UpdateStatus::operator==(const UpdateStatus &other) const
{
    return (lastUpdateTime_ == other.lastUpdateTime_) &&
           (isValid_ == other.isValid_) &&
           (errorMessage_ == other.errorMessage_) &&
           (isOnline_ == other.isOnline_) &&
           (currentVersion_ == other.currentVersion_) &&
           (currentVersionDisplayName_ == other.currentVersionDisplayName_) &&
           (updatePolicy_ == other.updatePolicy_) &&
           (releaseOnlyRelease_ == other.releaseOnlyRelease_) &&
           (releaseOrBetaRelease_ == other.releaseOrBetaRelease_) &&
           (devRelease_ == other.devRelease_);
}

UpdatePolicyT UpdaterImpl::GetUpdatePolicy()
{
    std::lock_guard lock{mutex};
    return updatePolicy;
}
void UpdaterImpl::SetUpdatePolicy(UpdatePolicyT updatePolicy)
{
    std::lock_guard lock{mutex};
    if (updatePolicy == this->updatePolicy)
        return;
    this->updatePolicy = updatePolicy;
    this->currentResult.UpdatePolicy(updatePolicy);
    SetCachedUpdateStatus(this->currentResult);
    if (listener)
    {
        listener(currentResult);
    }
}
void UpdaterImpl::ForceUpdateCheck()
{
    uint64_t value = UNCACHED_CHECK_NOW_EVENT;
    write(this->event_writer, &value, sizeof(uint64_t));
}

UpdateStatus::UpdateStatus()
{
    currentVersion_ = PROJECT_VER;
    currentVersionDisplayName_ = PROJECT_DISPLAY_VERSION;

#ifdef TEST_UPDATE
    // uncomment this line to test upgrading.
    currentVersion_ = "1.2.39";
    currentVersionDisplayName_ = "PiPedal 1.2.39-Debug";
#endif
}

void UpdateStatus::ResetCurrentVersion()
{
    currentVersion_ = PROJECT_VER;
    currentVersionDisplayName_ = PROJECT_DISPLAY_VERSION;

#ifdef TEST_UPDATE
    // uncomment this line to test upgrading.
    currentVersion_ = "1.2.39";
    currentVersionDisplayName_ = "PiPedal 1.2.39-Debug";
#endif
}

std::chrono::system_clock::time_point UpdateStatus::LastUpdateTime() const
{
    std::chrono::system_clock::duration duration{this->lastUpdateTime_};
    std::chrono::system_clock::time_point tp{duration};
    return tp;
}

void UpdateStatus::LastUpdateTime(const std::chrono::system_clock::time_point &timePoint)
{
    this->lastUpdateTime_ = timePoint.time_since_epoch().count();
}

const GithubAsset *GithubRelease::GetGpgKeyForAsset(const std::string &name) const
{
    std::string targetName = name + ".asc";

    for (auto &asset : assets)
    {
        if (asset.name == targetName)
        {
            return &asset;
        }
    }
    return nullptr;
}
const GithubAsset *GithubRelease::GetDownloadForCurrentArchitecture() const
{
    // deb package names end in {DEBIAN_ARCHITECTURE}.deb
    // pipedal build gets this value from `dpkg --print-architecture`
#ifndef DEBIAN_ARCHITECTURE // deb package names end in {DEBIAN_ARCHITECTURE}.deb
#error DEBIAN_ARCHITECTURE not defined
#endif
    std::string downloadEnding = SS("_" << (DEBIAN_ARCHITECTURE) << ".deb");

    for (auto &asset : assets)
    {
        if (asset.name.ends_with(downloadEnding))
        {
            return &asset;
        }
    }
    return nullptr;
}

UpdateRelease::UpdateRelease()
{
}

std::string UpdaterImpl::GetSignatureUrl(const std::string &url)
{

    // partialy whitelisting, partly avoiding having to parse a URL.
    if (this->currentResult.releaseOnlyRelease_.UpdateUrl() == url)
    {
        return this->currentResult.releaseOnlyRelease_.GpgSignatureUrl();
    }
    if (this->currentResult.releaseOrBetaRelease_.UpdateUrl() == url)
    {
        return this->currentResult.releaseOrBetaRelease_.GpgSignatureUrl();
    }
    if (this->currentResult.devRelease_.UpdateUrl() == url)
    {
        return this->currentResult.devRelease_.GpgSignatureUrl();
    }
    throw std::runtime_error("Permission denied. No signature URL.");
}

std::string UpdaterImpl::GetUpdateFilename(const std::string &url)
{

    // partialy whitelisting, partly avoiding having to parse a URL.
    if (this->currentResult.releaseOnlyRelease_.UpdateUrl() == url)
    {
        return this->currentResult.releaseOnlyRelease_.AssetName();
    }
    if (this->currentResult.releaseOrBetaRelease_.UpdateUrl() == url)
    {
        return this->currentResult.releaseOrBetaRelease_.AssetName();
    }
    if (this->currentResult.devRelease_.UpdateUrl() == url)
    {
        return this->currentResult.devRelease_.AssetName();
    }
    throw std::runtime_error("Permission denied. Invalid url.");
}
static std::string unCRLF(const std::string &text)
{
    std::ostringstream ss;
    for (char c : text)
    {
        if (c == '\r')
            continue;
        if (c == '\n')
        {
            ss << '/';
        }
        else
        {
            ss << c;
        }
    }
    return ss.str();
}

static void removeOldSiblings(int numberToKeep, const std::filesystem::path &fileToKeep)
{

    auto extension = fileToKeep.extension();
    auto directory = fileToKeep.parent_path();
    if (directory.empty())
        return; // superstition.
    struct RemoveEntry
    {
        fs::path path;
        fs::file_time_type time;
    };
    std::vector<RemoveEntry> entries;
    for (const auto &dirEntry : fs::directory_iterator(directory))
    {
        if (dirEntry.is_regular_file())
        {
            auto thisPath = dirEntry.path();
            if (thisPath != fileToKeep)
            {
                if (thisPath.extension() == extension)
                {
                    entries.push_back(
                        RemoveEntry{
                            .path = thisPath,
                            .time = dirEntry.last_write_time()});
                }
            }
        }
    }
    std::sort(
        entries.begin(), entries.end(),
        [](const RemoveEntry &left, const RemoveEntry &right)
        {
            return left.time > right.time; // by time descending
        });
    for (size_t i = numberToKeep; i < entries.size(); ++i)
    {
        fs::remove(entries[i].path);
    }
}

static std::string getFingerprint(const std::string &gpgText)
{
    std::string keyPosition = "using RSA key ";
    size_t nPos = gpgText.find(keyPosition);
    if (nPos == std::string::npos)
    {
        return "";
    }
    nPos += keyPosition.length();
    while (nPos < gpgText.length() && gpgText[nPos] == ' ')
    {
        ++nPos;
    }
    auto start = nPos;

    while (true)
    {
        char c = gpgText[nPos];
        if (nPos >= gpgText.length() || c == ' ' || c == '\r' || c == '\n' || c == '\t')
        {
            break;
        }
        ++nPos;
    }
    return gpgText.substr(start, nPos - start);
}
static bool IsSignatureGood(const std::string &gpgText)
{
    std::string originPosition = "gpg: Good signature from \"";

    size_t nPos = gpgText.find(originPosition);
    return nPos != std::string::npos;
}
static std::string getAddress(const std::string &gpgText)
{
    std::string originPosition = "gpg: Good signature from \"";

    size_t nPos = gpgText.find(originPosition);
    if (nPos == std::string::npos)
    {
        return "";
    }
    nPos += originPosition.length();
    auto start = nPos;
    while (true)
    {
        if (nPos >= gpgText.length() || gpgText[nPos] == '\"' || gpgText[nPos] == '\r' || gpgText[nPos] == '\n')
        {
            break;
        }
        ++nPos;
    }
    return gpgText.substr(start, nPos - start);
}
void Updater::ValidateSignature(const std::filesystem::path &file, const std::filesystem::path &signatureFile)
{
    // sudo gpg --home /var/pipedal/config/gpg  --verify pipedal_1.2.41_arm64.deb.asc  pipedal_1.2.41_arm64.deb
    // gpg: assuming signed data in 'pipedal_1.2.41_arm64.deb'
    // gpg: Signature made Tue 27 Aug 2024 09:25:06 PM EDT
    // gpg:                using RSA key 381124E2BB4478D225D2313B2AEF3F7BD53EAA59
    // gpg: Good signature from "Robin Davies <rerdavies@gmail.com>" [ultimate]
    std::ostringstream ss;
    ss << "--home " << PGP_UPDATE_KEYRING_PATH
       << " --verify "
       << signatureFile << " " << file;

    Lv2Log::debug(SS("/usr/bin/gpg " << ss.str()));
    auto gpgOutput = sysExecForOutput("/usr/bin/gpg", ss.str());
    if (gpgOutput.exitCode != EXIT_SUCCESS)
    {
        throw std::runtime_error("Update file is not correctly signed.");
    }
    const std::string &gpgText = gpgOutput.output;

    if (!IsSignatureGood(gpgText))
    {
        Lv2Log::error(gpgOutput.output);
        throw std::runtime_error("Update signature is not valid.");
    }
    std::string keyId = getFingerprint(gpgText);

    if (keyId != UPDATE_GPG_FINGERPRINT && keyId != UPDATE_GPG_FINGERPRINT2)
    {
        Lv2Log::error(gpgOutput.output);
        throw std::runtime_error(SS("Update signature has the wrong id: " << keyId));
    }
    std::string origin = getAddress(gpgText);
    if (origin != UPDATE_GPG_ADDRESS && origin != UPDATE_GPG_ADDRESS2)
    {
        Lv2Log::error(gpgOutput.output);
        throw std::runtime_error(SS("Update signature has an incorrect address." << origin));
    }
}

static bool badOutput(const std::filesystem::path &filePath)
{
    if (!fs::is_regular_file(filePath))
    {
        return true;
    }
    return fs::file_size(filePath) == 0;
}

static void checkCurlHttpResponse(std::string errorCode)
{
    if (errorCode.starts_with("%"))
    {
        errorCode = errorCode.substr(1);
    }
    int code = -999;
    {
        std::istringstream ss{errorCode};
        ss >> code;
    }
    if (code == -999)
    {
        throw std::runtime_error(SS("Download failed. Invalid curl response: " << errorCode));
    }
    if (code == 200)
    {
        return;
    }
    if (code == 0)
    {
        throw std::runtime_error("PiPedal server can't reach the internet.");
    }

    {
        std::string message = SS("Download failed (HTTP error " << code << ")");
        throw std::runtime_error(message);
    }
}
void UpdaterImpl::RetryAfter(clock::duration delay)
{
    namespace chron = std::chrono;
    clock::duration hours24 = chron::duration_cast<clock::duration>(chron::hours(24));
    if (delay.count() < 0 || delay >= hours24) // non-synced system clock (as Raspberry Pis are prone to without network connections)
    {
        delay = chron::duration_cast<clock::duration>(chron::hours(6));
    }
    SaveRetryTime(chron::system_clock::now() + std::chrono::duration_cast<chron::system_clock::duration>(delay));

    clock::time_point myRetryTime = clock::now() + delay;

    if (this->updateRetryTime < myRetryTime)
    {
        this->updateRetryTime = myRetryTime;
    }
}
void UpdaterImpl::SaveRetryTime(const std::chrono::system_clock::time_point &time)
{
    fs::path retryTimePath = workingDirectory / "retryTime.json";
    pipedal::ofstream_synced f{retryTimePath};
    if (f.is_open())
    {
        json_writer writer(f);
        auto rep = time.time_since_epoch().count();
        writer.write(rep);
    }
}
UpdaterImpl::clock::time_point UpdaterImpl::LoadRetryTime()
{
    using namespace std::chrono;

    fs::path retryTimePath = workingDirectory / "retryTime.json";
    std::ifstream f{retryTimePath};
    if (f.is_open())
    {
        system_clock::duration::rep tRep = 0;
        json_reader reader(f);
        reader.read(&tRep);

        system_clock::duration duration(tRep);

        system_clock::time_point systemTime = system_clock::time_point(duration);

        // now convert to stead_clock time with the understanding that system_clock time may be blown.
        system_clock::duration systemDuration = systemTime - system_clock::now();
        clock::duration clockDuration = duration_cast<clock::duration>(systemDuration);
        if (clockDuration > duration_cast<clock::duration>(hours(48))) // is system clock blown?
        {
            clockDuration = duration_cast<clock::duration>(hours(6));
        }
        return clock::now() + clockDuration;
    }
    return clock::now();
}

void UpdaterImpl::DownloadUpdate(const std::string &url, std::filesystem::path *file, std::filesystem::path *signatureFile)
{
    std::string filename, signatureUrl;
    {
        std::lock_guard lock{mutex};
        filename = GetUpdateFilename(url);
        signatureUrl = GetSignatureUrl(url);
    }

    // Only permit downloading of updates from the github releases for the pipedal project.
    // I don't think this will get past GetUpdateFileName, but defense in depth. We will
    // not download from anywhere we don't trust.
    if (!WhitelistDownloadUrl(url))
    {
        throw std::runtime_error(SS("Invalid update url. Downloads from this address are not permitted: " << url));
    }
    if (!WhitelistDownloadUrl(signatureUrl))
    {
        throw std::runtime_error(SS("Invalid update url. Downloads from this address are not permitted: " << signatureUrl));
    }
    auto downloadDirectory = workingDirectory / "downloads";
    std::filesystem::create_directories(downloadDirectory);

    auto downloadFilePath = downloadDirectory / filename;
    auto downloadSignaturePath = downloadDirectory / SS(filename << ".asc");

    try
    {
        fs::remove(downloadFilePath);
        fs::remove(downloadSignaturePath);

        const std::string responseOption = "-w \"%{response_code}\"";
#ifdef WIN32
        responseOption = "-w \"%%{response_code}\""; // windows shell requires doubling of the %%.
#endif
        std::string args = SS("-s -L " << responseOption << " " << url << " -o " << downloadFilePath.c_str());
        auto curlOutput = sysExecForOutput("/usr/bin/curl", args);
        if (curlOutput.exitCode != EXIT_SUCCESS || badOutput(downloadFilePath))
        {
            Lv2Log::error(SS("Update download failed. " << unCRLF(curlOutput.output)));
            throw std::runtime_error("PiPedal server does not have access to the internet.");
        }
        checkCurlHttpResponse(curlOutput.output);

        args = SS("-s -L " << responseOption << " " << signatureUrl << " -o " << downloadSignaturePath.c_str());

        curlOutput = sysExecForOutput("/usr/bin/curl", args);
        if (curlOutput.exitCode != EXIT_SUCCESS || badOutput(downloadSignaturePath))
        {
            Lv2Log::error(SS("Update download failed. " << unCRLF(curlOutput.output)));
            throw std::runtime_error("PiPedal server does not have access to the internet.");
        }
        checkCurlHttpResponse(curlOutput.output);

        try
        {
            removeOldSiblings(2, downloadFilePath);
            removeOldSiblings(2, downloadSignaturePath);
        }
        catch (const std::exception &e)
        {
            Lv2Log::error(SS("Can't remove download siblings" << e.what()));
            // and carry on.
        }

        // Can't only be performed under pipedal_d or root accouts.
        // This is as far as you can go while debugging.

        // The admin process will actually check the signature again running as root; but this is our last
        // chance to give a reasonable error message, so do it here as well, because any subsequent errors
        // can only go to systemd logs.
        ValidateSignature(downloadFilePath, downloadSignaturePath);
        *file = downloadFilePath;
        *signatureFile = downloadSignaturePath;
        return;
    }
    catch (const std::exception &e)
    {
        std::filesystem::remove(downloadFilePath);
        std::filesystem::remove(downloadSignaturePath);
        throw;
    }
}


void UpdateStatus::AddRelease(const std::string&packageName)
{
    this->isValid_ = true;
    this->errorMessage_ = "";
}
        
///////////////////////////////////////////////////////////////////////////////////////////////

JSON_MAP_BEGIN(UpdateRelease)
JSON_MAP_REFERENCE(UpdateRelease, updateAvailable)
JSON_MAP_REFERENCE(UpdateRelease, upgradeVersion)
JSON_MAP_REFERENCE(UpdateRelease, upgradeVersionDisplayName)
JSON_MAP_REFERENCE(UpdateRelease, assetName)
JSON_MAP_REFERENCE(UpdateRelease, updateUrl)
JSON_MAP_REFERENCE(UpdateRelease, gpgSignatureUrl)
JSON_MAP_END();

JSON_MAP_BEGIN(UpdateStatus)
JSON_MAP_REFERENCE(UpdateStatus, lastUpdateTime)
JSON_MAP_REFERENCE(UpdateStatus, isValid)
JSON_MAP_REFERENCE(UpdateStatus, errorMessage)
JSON_MAP_REFERENCE(UpdateStatus, isOnline)
JSON_MAP_REFERENCE(UpdateStatus, currentVersion)
JSON_MAP_REFERENCE(UpdateStatus, currentVersionDisplayName)
JSON_MAP_REFERENCE(UpdateStatus, updatePolicy)
JSON_MAP_REFERENCE(UpdateStatus, releaseOnlyRelease)
JSON_MAP_REFERENCE(UpdateStatus, releaseOrBetaRelease)
JSON_MAP_REFERENCE(UpdateStatus, devRelease)
JSON_MAP_END();
