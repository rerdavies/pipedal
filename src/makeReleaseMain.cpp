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

using namespace std;
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
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmdRedirected.c_str(), "r"), pclose);
    if (!pipe)
    {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr)
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
        SS("/usr/bin/gpg --yes --default-key " << UPDATE_GPG_FINGERPRINT2
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
                                   << " --armor " << packagePath << ".asc " << packagePath.c_str());
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
int main(int argc, char **argv)
{
    CommandLineParser commandLine;
    try
    {
        SignPackage();
    }
    catch (const std::exception &e)
    {
        cout << "ERROR: " << e.what() << endl;
        return EXIT_FAILURE;
    }
}