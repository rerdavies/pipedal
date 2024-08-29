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

using namespace pipedal;
namespace fs = std::filesystem;

#undef TEST_UPDATE

#ifndef DEBUG
#undef TEST_UPDATE // do NOT leat this leak into a production build!
#endif

static constexpr uint64_t CLOSE_EVENT = 0;
static constexpr uint64_t CHECK_NOW_EVENT = 1;
static constexpr uint64_t UNCACHED_CHECK_NOW_EVENT = 2;

static std::filesystem::path WORKING_DIRECTORY = "/var/pipedal/updates";
static std::filesystem::path UPDATE_STATUS_CACHE_FILE = WORKING_DIRECTORY / "updateStatus.json";

Updater::clock::duration Updater::updateRate = std::chrono::duration_cast<Updater::clock::duration>(std::chrono::days(1));
static std::chrono::system_clock::duration CACHE_DURATION = std::chrono::duration_cast<std::chrono::system_clock::duration>(std::chrono::minutes(30));

std::mutex cacheMutex;
static UpdateStatus GetCachedUpdateStatus()
{
    std::lock_guard lock{cacheMutex};
    try
    {
        if (std::filesystem::exists(UPDATE_STATUS_CACHE_FILE))
        {
            std::ifstream f{UPDATE_STATUS_CACHE_FILE};
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

static void SetCachedUpdateStatus(UpdateStatus &updateStatus)
{
    std::lock_guard lock{cacheMutex};
    updateStatus.LastUpdateTime(std::chrono::system_clock::now());
    try
    {
        std::ofstream f{UPDATE_STATUS_CACHE_FILE};
        json_writer writer{f};
        writer.write(updateStatus);
    }
    catch (const std::exception &e)
    {
        Lv2Log::error(SS("Unable to write cached UpdateStatus. " << e.what()));
    }
}
Updater::Updater()
{
    cachedUpdateStatus = GetCachedUpdateStatus();
    this->updatePolicy = cachedUpdateStatus.UpdatePolicy();
    currentResult = cachedUpdateStatus;

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
Updater::~Updater()
{
    Stop();
}
void Updater::Stop()
{
    if (stopped)
    {
        return;
    }
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

void Updater::CheckNow()
{
    uint64_t value = CHECK_NOW_EVENT;
    write(this->event_writer, &value, sizeof(uint64_t));
}

void Updater::SetUpdateListener(UpdateListener &&listener)
{
    std::lock_guard lock{mutex};
    this->listener = listener;
    if (hasInfo)
    {
        listener(currentResult);
    }
}

void Updater::ThreadProc()
{
    SetThreadName("UpdateMonitor");
    struct pollfd pfd;
    pfd.fd = this->event_reader;
    pfd.events = POLLIN;

    while (true)
    {
        int ret = poll(&pfd, 1, std::chrono::duration_cast<std::chrono::milliseconds>(updateRate).count()); // 1000 ms timeout

        if (ret == -1)
        {
            Lv2Log::error("Updater: Poll error.");
            break;
        }
        else if (ret == 0)
        {
            CheckForUpdate(true);
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

int compareVersions(const std::string &l, const std::string &r)
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

static bool IsCacheValid(const UpdateStatus &updateStatus)
{
    if (!updateStatus.IsValid() || !updateStatus.IsOnline())
    {
        return false;
    }
    auto now = std::chrono::system_clock::now();
    auto validStart = updateStatus.LastUpdateTime();
    auto validEnd = validStart + CACHE_DURATION;
    return now >= validStart && now < validEnd;
}

UpdateRelease Updater::getUpdateRelease(
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

void Updater::CheckForUpdate(bool useCache)
{
    UpdateStatus updateResult;

    {
        std::lock_guard lock{mutex};
        if (useCache && IsCacheValid(cachedUpdateStatus))
        {
            this->currentResult = cachedUpdateStatus;
            this->currentResult.UpdatePolicy(this->updatePolicy);
            if (listener)
            {
                listener(this->currentResult);
            }
            return;
        }
        updateResult = this->currentResult;
    }

    //const std::string responseOption = "-w \"%{response_code}\"";
#ifdef WIN32
    responseOption = "-w \"%%{response_code}\""; // windows shell requires doubling of the %%.
#endif

    std::string args = SS("-s -L " << GITHUB_RELEASES_URL);

    updateResult.errorMessage_ = "";

    try
    {
        auto result = sysExecForOutput("curl", args);
        if (result.exitCode != EXIT_SUCCESS)
        {
            throw std::runtime_error("Server has no internet access.");
        }
        else
        {
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
                std::string message = o->at("message").as_string();
                auto status_code = o->at("status_code").as_int64();
                throw std::runtime_error(SS("Service error. ()" << status_code << ": " << message << ")"));
            }
            else
            {
                json_variant::array_ptr vArray = vResult.as_array();

                std::vector<GithubRelease> releases;
                for (size_t i = 0; i < vArray->size(); ++i)
                {
                    auto &el = vArray->at(0);
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
    }
    catch (const std::exception &e)
    {
        Lv2Log::error(SS("Failed to fetch update info. " << e.what()));
        updateResult.errorMessage_ = e.what();
        updateResult.isValid_ = false;
        updateResult.isOnline_ = false;
    }
    {
        std::lock_guard lock{mutex};
        updateResult.UpdatePolicy(this->updatePolicy);
        this->currentResult = updateResult;
        SetCachedUpdateStatus(this->currentResult);
        if (listener)
        {
            listener(this->currentResult);
        }
    }
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

UpdatePolicyT Updater::GetUpdatePolicy()
{
    std::lock_guard lock{mutex};
    return updatePolicy;
}
void Updater::SetUpdatePolicy(UpdatePolicyT updatePolicy)
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
void Updater::ForceUpdateCheck()
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

std::string Updater::GetSignatureUrl(const std::string &url)
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

std::string Updater::GetUpdateFilename(const std::string &url)
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
static bool IsSignatureGood(const std::string&gpgText)
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

    Lv2Log::info(SS("/usr/bin/gpg " << ss.str()));
    auto gpgOutput = sysExecForOutput("/usr/bin/gpg", ss.str());
    if (gpgOutput.exitCode != EXIT_SUCCESS)
    {
        throw std::runtime_error("Update file is not correctly signed.");
    }
    const std::string &gpgText = gpgOutput.output;

    if (!IsSignatureGood(gpgText))
    {
        throw std::runtime_error("Update signature is not valid.");
    }
    std::string keyId = getFingerprint(gpgText);

    Lv2Log::info(unCRLF(gpgText)); // yyy delete me.

    if (keyId != UPDATE_GPG_FINGERPRINT)
    {
        throw std::runtime_error(SS("Update signature has the wrong fingerprint: " << keyId));
    }
    std::string origin = getAddress(gpgText);
    if (origin != UPDATE_GPG_ADDRESS)
    {
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
void Updater::DownloadUpdate(const std::string &url, std::filesystem::path *file, std::filesystem::path *signatureFile)
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
    auto downloadDirectory = WORKING_DIRECTORY / "downloads";
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
        Lv2Log::info(SS("/usr/bin/curl " << args)); // yyy delete me.
        auto curlOutput = sysExecForOutput("/usr/bin/curl", args);
        if (curlOutput.exitCode != EXIT_SUCCESS || badOutput(downloadFilePath))
        {
            Lv2Log::error(SS("Update download failed. " << unCRLF(curlOutput.output)));
            throw std::runtime_error("PiPedal server does not have access to the internet.");
        }
        checkCurlHttpResponse(curlOutput.output);

        args = SS("-s -L " << responseOption << " " << signatureUrl << " -o " << downloadSignaturePath.c_str());
        Lv2Log::info(SS("/usr/bin/curl " << args));

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
