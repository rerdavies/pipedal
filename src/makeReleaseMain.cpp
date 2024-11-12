#include "CommandLineParser.hpp"
#include <filesystem>
#include "ss.hpp"
#include "config.hpp"
#include <stdexcept>
#include <iostream>
#include <memory>
#include <array>
#include "unistd.h"
#include "UpdaterSecurity.hpp"
#include "Updater.hpp"
#include <fstream>
#include "SysExec.hpp"
#include "TemporaryFile.hpp"
#include "json_variant.hpp"
#include "GithubResponseHeaders.hpp"
#include "Finally.hpp"

using namespace std;
using namespace pipedal;
namespace fs = std::filesystem;

std::string getVersionString()
{
    std::string displayVersion = PROJECT_DISPLAY_VERSION;
    auto npos = displayVersion.find_last_of('-');
    if (npos == std::string::npos)
    {
        throw std::runtime_error(SS("PROJECT_DISPLAY_VERSION does not contain a '-' character." << displayVersion));
    }
    std::string releaseKind = displayVersion.substr(npos + 1);

    return SS(PROJECT_VER << "-" << releaseKind);
}

std::string psystem(const std::string &command)
{
    std::string cmdRedirected = command + " 2>&1";
    std::array<char, 128> buffer;
    std::string result;
    FILE *pipe = popen(cmdRedirected.c_str(), "r");
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    Finally ff{[pipe]() {
        pclose(pipe);
    }};
    while (fgets(buffer.data(), buffer.size()-1, pipe) != nullptr)
    {
        result += buffer.data();
    }
    return result;
}

using namespace pipedal;

void SignPackage()
{
    auto versionString = getVersionString();
    cout << "Processing release " << versionString << endl;

    // build/pipedal_1.2.49_arm64.deb

    fs::path packagePath = SS("build/pipedal_" << PROJECT_VER << "_arm64.deb");
    packagePath = fs::absolute(packagePath);
    if (!fs::exists(packagePath))
    {
        throw std::runtime_error(SS("File does not exist: " << packagePath));
    }

    // sign the package.
    // gpg --armor --output "$filename".asc -b "$filename"
    cout << "--------------------------------------------------------------" << endl;
    cout << "Signing the package" << endl;
    cout << "--------------------------------------------------------------" << endl;

    std::string signCmd =
        SS("/usr/bin/gpg --pinentry-mode loopback --yes --default-key " << UPDATE_GPG_FINGERPRINT2
                                               << " --armor --output " << packagePath << ".asc"
                                               << " --detach-sign " << packagePath.c_str());
    int result = system(signCmd.c_str());
    if (result != EXIT_SUCCESS)
    {
        throw std::runtime_error("Failed to sign package.");
    }

    cout << "--------------------------------------------------------------" << endl;
    cout << "Building a test keyring." << endl;
    cout << "--------------------------------------------------------------" << endl;

    fs::path keyringPath = fs::absolute("build/gpg");
    fs::path keyPath = fs::absolute("config/updatekey2.asc");
    fs::remove_all(keyringPath);
    fs::create_directories(keyringPath);
    std::ostringstream importCommand;

    importCommand << "/usr/bin/gpg  --homedir " << keyringPath.c_str() << " --import " << keyPath.c_str();
    int rc = system(importCommand.str().c_str());
    if (rc != EXIT_SUCCESS)
    {
        throw std::runtime_error("Failed  to create test keyring.");
    }

    cout << "--------------------------------------------------------------" << endl;
    cout << "Verifying signature" << endl;
    cout << "--------------------------------------------------------------" << endl;

    // verify the signature.
    // warnings are unavoidable as there's no way to create a root certificate without
    // an elaborate manual procedure.
    cout << "Warnings are expected, since there's no convenient way to install a " << endl;
    cout << "trusted certificate authority in the Updater keychain." << endl;
    std::string verifyCommand = SS("/usr/bin/gpg --verify --no-default-keyring "
                                   << "--homedir  " << keyringPath.c_str()
                                   << " --armor " << packagePath.c_str() << ".asc " << packagePath.c_str());

    cout << verifyCommand << endl;
    auto output = psystem(verifyCommand.c_str());
    cout << output << endl;
    auto npos = output.find("Good signature from \"" UPDATE_GPG_ADDRESS2 "\"");
    if (npos == std::string::npos)
    {
        throw std::runtime_error("Signature validation failed.");
    }
    npos == output.find("using RSA key " UPDATE_GPG_FINGERPRINT2);
    if (npos == std::string::npos)
    {
        throw std::runtime_error("Signature validation failed.");
    }
    cout << "Signature check succeeded." << endl;
    cout << "--------------------------------------------------------------" << endl;
    cout << "Validating signing procedure" << endl;
    cout << "--------------------------------------------------------------" << endl;
    // Actually a test to make sure that signature checks fail on a modified binary.
    // Calling the test "Testing signature failure" is a more disconcerting than it needs to be.
    // Just one last sanity check before letting this procedure loose in the world.

    // create a bad test file.
    fs::path badPackage = "build/gpgtest/bad_package.deb";
    fs::create_directories("build/gpgtest");
    fs::remove(badPackage);
    fs::copy_file(packagePath, badPackage);
    {
        std::ofstream f;
        f.open(badPackage, ios_base::out | ios_base::ate);
        if (!f.is_open())
        {
            throw std::runtime_error(SS("Can't open " << badPackage));
        }
        f << "Bad stuff.";
    }

    // verify the signature.
    {
        std::string verifyCommand = SS("/usr/bin/gpg --verify --no-default-keyring "
                                       << "--homedir  " << keyringPath.c_str()
                                       << " --armor " << packagePath << ".asc " << badPackage.c_str());
        auto output = psystem(verifyCommand.c_str());
        auto npos = output.find("BAD signature from ");
        if (npos == std::string::npos)
        {
            throw std::runtime_error("Signature validation succeeded when it should not have.");
        }
    }
    cout << "Test succeeded." << endl;
}

void GenerateUpdateJson()
{
    cout << "--------------------------------------------------------------" << endl;
    cout << "Validating signing procedure" << endl;
    cout << "--------------------------------------------------------------" << endl;
    // Actually a test to make sure that signature checks fail on a modified binary.
    // Calling the test "Testing signature failure" is a more disconcerting than it needs to be.
    // Just one last sanity check in the
    std::filesystem::path workingPath = fs::absolute("build/updater");
    Updater::ptr updater = Updater::Create(workingPath); // do NOT start it. We'll do the github request on the current thread.
    std::string packageName = SS("build/pipedal_" << PROJECT_VER << "_arm64.deb");

    // pipedal_1.2.3_arm64.deb
    UpdateStatus updateStatus = updater->GetReleaseGeneratorStatus();
    updateStatus.AddRelease(packageName);
}

class DownloadCountAsset
{
public:
    DownloadCountAsset(const json_variant &v)
    {
        auto o = v.as_object();
        this->name = o->at("name").as_string();
        this->browser_download_url = o->at("browser_download_url").as_string();
        this->updated_at = o->at("updated_at").as_string();
        this->downloads = (uint64_t)(o->at("download_count").as_number());
    }
    std::string name;
    std::string browser_download_url;
    std::string updated_at;
    uint64_t downloads;
};
class DownloadCountRelease
{
public:
    DownloadCountRelease(const json_variant &v)
    {
        auto o = v.as_object();
        this->name = o->at("name").as_string();
        this->draft = o->at("draft").as_bool();
        this->prerelease = o->at("prerelease").as_bool();

        auto assets = o->at("assets").as_array();
        for (size_t i = 0; i < assets->size(); ++i)
        {
            auto &el = assets->at(i);
            this->assets.push_back(DownloadCountAsset(el));
        }
        this->published_at = o->at("published_at").as_string();
    }
    std::string name;
    bool draft = false, prerelease = false;
    std::vector<DownloadCountAsset> assets;
    std::string published_at;
};

static std::string timePointToISO8601(const std::chrono::system_clock::time_point &tp)
{
    auto tt = std::chrono::system_clock::to_time_t(tp);
    std::tm tm = *std::gmtime(&tt);
    std::stringstream ss;
    ss << std::put_time(&tm,"%Y-%m-%dT%H:%M:%S") << "Z";
    return ss.str();
    
}

void GetDownloadCounts(bool gplotDownloads)
{
    // error handling and temporary file use is different enough that it justifies
    // not including this code in production code.

    // const std::string responseOption = "-w \"%{response_code}\"";
#ifdef WIN32
    responseOption = "-w \"%%{response_code}\""; // windows shell requires doubling of the %%.
#endif

    TemporaryFile headerFile{"/tmp"};
    std::string args = SS("-s -L " << GITHUB_RELEASES_URL << " -D " << headerFile.str());

    auto result = sysExecForOutput("curl", args);
    if (result.exitCode != EXIT_SUCCESS)
    {
        throw std::runtime_error("Github APIs not available.");
    }
    else
    {
        // hard throttling for github.

        GithubResponseHeaders githubHeaders{headerFile.Path()};

        if (githubHeaders.code_ != 200)
        {
            if (
                githubHeaders.ratelimit_limit_ != 0 &&
                githubHeaders.ratelimit_limit_ == githubHeaders.ratelimit_used_)
            {
                std::time_t time = std::chrono::system_clock::to_time_t(githubHeaders.ratelimit_reset_);
                std::string strTime = std::ctime(&time);

                throw std::runtime_error(SS("Github API rate limit exceeded. Try again at " << strTime));
            }
        }

        if (result.output.length() == 0)
        {
            throw std::runtime_error("No internet access.");
        }
        std::stringstream ss(result.output);
        json_reader reader(ss);
        json_variant vResult(reader);

        if (vResult.is_object())
        {
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

            std::vector<DownloadCountRelease> releases;
            for (size_t i = 0; i < vArray->size(); ++i)
            {
                auto &el = vArray->at(i);
                DownloadCountRelease release{el};
                if (!release.draft)
                {
                    releases.push_back(std::move(release));
                }
            }
            if (gplotDownloads)
            {
                std::sort(
                    releases.begin(),
                    releases.end(),
                    [](const DownloadCountRelease &left, const DownloadCountRelease &right)
                    {
                        return left.published_at < right.published_at; // date ascending
                    });

                for (size_t i = 0; i < releases.size()-1; ++i)
                {
                    releases[i].published_at = releases[i+1].published_at;
                }
                if (releases.size() >= 1)
                {
                    releases[releases.size()-1].published_at = timePointToISO8601(std::chrono::system_clock::now());
                }
                uint64_t cumulativeCount = 0;
                for (const auto &release : releases)
                {
                    for (const auto &asset : release.assets)
                    {
                        if (asset.name.ends_with(".deb"))
                        {
                            cumulativeCount += asset.downloads;
                        }
                    }
                    cout << release.published_at << " " << cumulativeCount << endl;
                }
            }
            else
            {
                std::sort(
                    releases.begin(),
                    releases.end(),
                    [](const DownloadCountRelease &left, const DownloadCountRelease &right)
                    {
                        return left.published_at > right.published_at; // latest date first.
                    });
                for (const auto &release : releases)
                {
                    cout << release.name << endl;
                    for (const auto &asset : release.assets)
                    {
                        cout << "    " << asset.name << ": " << asset.downloads << endl;
                    }
                }
            }
        }
    }
}
int main(int argc, char **argv)
{
    CommandLineParser commandLine;
    bool getDownloadCounts = false;
    bool gPlotDownloads = false;

    try
    {
        commandLine.AddOption("--downloads", &getDownloadCounts);
        commandLine.AddOption("--gplot-downloads", &gPlotDownloads);
        commandLine.Parse(argc, (const char **)argv);
        if (commandLine.Arguments().size() != 0)
        {
            throw std::runtime_error("Invalid arguments.");
        }
        if (getDownloadCounts || gPlotDownloads)
        {
            GetDownloadCounts(gPlotDownloads);
        }
        else
        {
            SignPackage();
        }
    }
    catch (const std::exception &e)
    {
        cout << "ERROR: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}