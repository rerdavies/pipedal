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

#include <iostream>
#include <utility>
#include <stdlib.h>
#include <unistd.h>
#include "CommandLineParser.hpp"
#include "SystemConfigFile.hpp"

#include <filesystem>
#include <stdlib.h>
#include "WriteTemplateFile.hpp"
#include <fstream>
#include "SetWifiConfig.hpp"
#include "SysExec.hpp"
#include <sys/wait.h>
#include <pwd.h>
#include "JackServerSettings.hpp"
#include "P2pConfigFiles.hpp"
#include "PrettyPrinter.hpp"
#include "ServiceConfiguration.hpp"
#include <uuid/uuid.h>
#include <random>
#include "AudioConfig.hpp"
#include "WifiChannelSelectors.hpp"
#include "PiPedalConfiguration.hpp"
#include <grp.h>
#include "ofstream_synced.hpp"

#define P2PD_DISABLED

#if JACK_HOST
#define INSTALL_JACK_SERVICE 1
#else
#define INSTALL_JACK_SERVICE 0
#endif

#define UNINSTALL_JACK_SERVICE 1

#define SS(x) (((std::stringstream &)(std::stringstream() << x)).str())

#define TEMPLATE_PATH(filename) ("/etc/pipedal/config/templates/" filename ".service")

using namespace std;
using namespace pipedal;
namespace fs = std::filesystem;

#define SERVICE_ACCOUNT_NAME "pipedal_d"
#define SERVICE_GROUP_NAME "pipedal_d"
#define JACK_SERVICE_ACCOUNT_NAME "jack"
#define AUDIO_SERVICE_GROUP_NAME "audio"
#define JACK_SERVICE_GROUP_NAME AUDIO_SERVICE_GROUP_NAME
#define NETDEV_GROUP_NAME "netdev"

#define SYSTEMCTL_BIN "/usr/bin/systemctl"
#define GROUPADD_BIN "/usr/sbin/groupadd"
#define USERADD_BIN "/usr/sbin/useradd"
#define USERMOD_BIN "/usr/sbin/usermod"
#define CHGRP_BIN "/usr/bin/chgrp"
#define CHOWN_BIN "/usr/bin/chown"
#define CHMOD_BIN "/usr/bin/chmod"

#define SERVICE_PATH "/usr/lib/systemd/system"
#define PIPEDALD_SERVICE "pipedald"
#define ADMIN_SERVICE "pipedaladmind"
#define PIPEDAL_P2PD_SERVICE "pipedal_p2pd"
#define PIPEDAL_NM_P2PD_SERVICE "pipedal_nm_p2pd"
#define NETWORK_MANAGER_SERIVCE "NetworkManager"
#define JACK_SERVICE "jack"

#define REMOVE_OLD_SERVICE 0 // Grandfathering: whether to remove the old shutdown service (now pipedaladmind)
#define OLD_SHUTDOWN_SERVICE "pipedalshutdownd"

fs::path GetServiceFileName(const std::string &serviceName)
{
    return fs::path(SERVICE_PATH) / (serviceName + ".service");
}

fs::path findOnPath(const std::string &command)
{
    std::string path = getenv("PATH");
    std::vector<std::string> paths;
    size_t t = 0;
    while (t < path.length())
    {
        size_t pos = path.find(':', t);
        if (pos == string::npos)
        {
            pos = path.length();
        }
        std::string thisPath = path.substr(t, pos - t);
        fs::path path = fs::path(thisPath) / command;
        if (fs::exists(path))
        {
            return path;
        }
        t = pos + 1;
    }
    std::stringstream s;
    s << "'" << command << "' is not installed.";
    throw std::runtime_error(s.str());
}

void EnableService()
{
    if (silentSysExec(SYSTEMCTL_BIN " enable " PIPEDALD_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to enable the " PIPEDALD_SERVICE " service.\n";
    }
    if (silentSysExec(SYSTEMCTL_BIN " enable " ADMIN_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to enable the " ADMIN_SERVICE " service.\n";
    }
}
void DisableService()
{
#if INSTALL_JACK_SERVICE || UNINSTALL_JACK_SERVICE
    if (silentSysExec(SYSTEMCTL_BIN " disable " JACK_SERVICE ".service") != EXIT_SUCCESS)
    {
#if INSTALL_JACK_SERVICE
// cout << "Error: Failed to disable the " JACK_SERVICE " service.";
#endif
    }
#endif
    if (sysExec(SYSTEMCTL_BIN " disable " PIPEDALD_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to disable the " PIPEDALD_SERVICE " service.\n";
    }
    if (sysExec(SYSTEMCTL_BIN " disable " ADMIN_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to disable the " ADMIN_SERVICE " service.\n";
    }
#if REMOVE_OLD_SERVICE
    if (sysExec(SYSTEMCTL_BIN " disable " OLD_SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to disable the " OLD_SHUTDOWN_SERVICE " service.\n";
    }
#endif
}

static void RestartAvahiService()
{
    silentSysExec(SS(SYSTEMCTL_BIN << " restart avahi-daemon.service").c_str());
}
static void AvahiInstall()
{
    // disable IPv6 mdns broadcasts. Avahi broadcasts link-local IPV6 addresses which are unusually difficult to deal with.

    try
    {
        std::filesystem::path avahiConfig("/etc/avahi/avahi-daemon.conf");
        SystemConfigFile avahi(avahiConfig);

        bool changed = avahi.RemoveUndoableActions();
        int line = avahi.GetLineThatStartsWith("use-ipv6=yes");
        if (line != -1)
        {
            avahi.UndoableReplaceLine(line, "use-ipv6=no");
            changed = true;
        }
        else
        {
            if (avahi.GetLineThatStartsWith("use-ipv6=no") == -1)
            {
                line = avahi.GetLineThatStartsWith("[server]");
                if (line == 1)
                {
                    throw std::runtime_error("Unable to find [server] section.");
                }
                {
                    // increment to end of section.
                    while (line < avahi.GetLineCount())
                    {
                        const auto &txt = avahi.Get(line);
                        if (txt.empty())
                        {
                            break;
                        }
                        if (txt.starts_with("[")) // start of next section.
                        {
                            break;
                        }
                        ++line;
                    }
                }
                avahi.UndoableAddLine(avahi.GetLineCount(), "use-ipv6=no");
                changed = true;
            }
        }
        if (changed)
        {
            avahi.Save();
            RestartAvahiService();
        }
    }
    catch (const std::exception &e)
    {
        cout << "Warning: Unabled to disable Ipv6 mDNS announcements. " << e.what() << endl;
    }
}

static void AvahiUninstall()
{
    try
    {
        std::filesystem::path avahiConfig("/etc/avahi/avahi-daemon.conf");
        SystemConfigFile avahi(avahiConfig);

        if (avahi.RemoveUndoableActions())
        {
            avahi.Save();
            RestartAvahiService();
        }
    }
    catch (const std::exception &e)
    {
        cout << " Warning: Unable to restore Avahi Daemon configuration. " << e.what() << endl;
    }
}

void StopService(bool excludeShutdownService = false)
{
    if (sysExec(SYSTEMCTL_BIN " stop " PIPEDALD_SERVICE ".service") != EXIT_SUCCESS)
    {
        cout << "Error: Failed to stop the " PIPEDALD_SERVICE " service.\n";
    }
    if (!excludeShutdownService)
    {
        if (sysExec(SYSTEMCTL_BIN " stop " ADMIN_SERVICE ".service") != EXIT_SUCCESS)
        {
            cout << "Error: Failed to stop the " ADMIN_SERVICE " service.\n";
        }
#if REMOVE_OLD_SERVICE
        if (sysExec(SYSTEMCTL_BIN " stop " OLD_SHUTDOWN_SERVICE ".service") != EXIT_SUCCESS)
        {
            cout << "Error: Failed to stop the " OLD_SHUTDOWN_SERVICE " service.\n";
        }
#endif
    }
#if INSTALL_JACK_SERVICE || UNINSTALL_JACK_SERVICE
    if (silentSysExec(SYSTEMCTL_BIN " stop " JACK_SERVICE ".service") != EXIT_SUCCESS)
    {
#if INSTALL_JACK_SERVICE
// cout << std::runtime_error("Failed to stop the " JACK_SERVICE " service.");
#endif
    }
#endif
}

void StartService(bool excludeShutdownService = false)
{
    if (!UsingNetworkManager())
    {
        silentSysExec("/usr/bin/pulseaudio --kill"); // interferes with Jack audio service startup.
    }
    if (!excludeShutdownService)
    {
        if (sysExec(SYSTEMCTL_BIN " start " ADMIN_SERVICE ".service") != EXIT_SUCCESS)
        {
            throw std::runtime_error("Failed to start the " ADMIN_SERVICE " service.");
        }
    }
    if (silentSysExec(SYSTEMCTL_BIN " start " PIPEDALD_SERVICE ".service") != EXIT_SUCCESS)
    {
        throw std::runtime_error("Failed to start the " PIPEDALD_SERVICE " service.");
    }
#if INSTALL_JACK_SERVICE
    JackServerSettings serverSettings;
    serverSettings.ReadJackDaemonConfiguration();
    if (serverSettings.IsValid())
    {
        if (sysExec(SYSTEMCTL_BIN " start " JACK_SERVICE ".service") != EXIT_SUCCESS)
        {
            throw std::runtime_error("Failed to start the " JACK_SERVICE " service.");
        }
    }
#endif
}
void RestartService(bool excludeShutdownService)
{

    StopService(excludeShutdownService);
    StartService(excludeShutdownService);
}

static bool userExists(const char *userName)
{
    struct passwd pwd;
    struct passwd *result;
    char *buf;
    size_t bufsize;
    int s;

    bufsize = sysconf(_SC_GETPW_R_SIZE_MAX);
    if (bufsize == -1)   /* Value was indeterminate */
        bufsize = 16384; /* Should be more than enough */

    buf = (char *)malloc(bufsize);
    if (buf == NULL)
    {
        perror("malloc");
        exit(EXIT_FAILURE);
    }

    s = getpwnam_r(userName, &pwd, buf, bufsize, &result);
    if (result == NULL)
    {
        return false;
    }
    return true;
}

static void RemoveLine(const std::string &path, const std::string lineToRemove)
{
    std::vector<std::string> lines;
    try
    {
        if (fs::exists(path))
        {
            {
                ifstream f(path);
                if (!f.is_open())
                {
                    throw std::runtime_error(SS("Can't open " << path));
                }
                while (true)
                {
                    std::string line;
                    if (f.eof())
                        break;
                    std::getline(f, line);
                    if (line != lineToRemove)
                    {
                        lines.push_back(line);
                    }
                }
            }

            {
                std::ofstream f(path);
                if (!f.is_open())
                {
                    throw std::runtime_error(SS("Can't write to " << path));
                }
                for (auto &line : lines)
                {
                    f << line << "\n";
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        cout << "Failed to update " << path << ". " << e.what();
    }
}

const std::string PAM_LINE = "JACK_PROMISCUOUS_SERVER      DEFAULT=" JACK_SERVICE_GROUP_NAME;
void UninstallPamEnv()
{
#if UNINSTALL_JACK_SERVICE
    RemoveLine(
        "/etc/security/pam_env.conf",
        PAM_LINE);
#endif
}
void InstallPamEnv()
{
#if INSTALL_JACK_SERVICE
    std::string newLine = PAM_LINE;
    std::vector<std::string> lines;
    fs::path path = "/etc/security/pam_env.conf";
    try
    {
        if (fs::exists(path))
        {
            {
                ifstream f(path);
                if (!f.is_open())
                {
                    throw std::runtime_error(SS("Can't open " << path));
                }
                while (true)
                {
                    std::string line;
                    if (f.eof())
                        break;
                    std::getline(f, line);
                    lines.push_back(line);
                }
            }

            for (auto &line : lines)
            {
                if (line == newLine)
                {
                    return;
                }
            }
            lines.push_back(newLine);

            {
                std::ofstream f(path);
                if (!f.is_open())
                {
                    throw std::runtime_error(SS("Can't write to " << path));
                }
                for (auto &line : lines)
                {
                    f << line << "\n";
                }
            }
        }
    }
    catch (const std::exception &e)
    {
        cout << "Failed to update " << path << ". " << e.what();
    }
#endif
}

void InstallLimits()
{
    if (sysExec(GROUPADD_BIN " -f " AUDIO_SERVICE_GROUP_NAME) != EXIT_SUCCESS)
    {
        throw std::runtime_error("Failed to create audio service group.");
    }

    fs::path limitsConfig = "/etc/security/limits.d/audio.conf";

    if (!fs::exists(limitsConfig))
    {
        ofstream output(limitsConfig);
        output << "# Realtime priority group used by pipedal audio (and also by jack)"
               << "\n";
        output << "@audio   -  rtprio     95"
               << "\n";
        output << "@audio   -  memlock    unlimited"
               << "\n";
    }
}

#if JACK_HOST
void MaybeStartJackService()
{
    fs::path drcFile = "/etc/jackdrc";

    if (fs::exists(drcFile) && fs::file_size(drcFile) != 0)
    {
        sysExec(SYSTEMCTL_BIN " start jack");
    }
    else
    {
        silentSysExec(SYSTEMCTL_BIN " mask jack");
    }
}
#endif

void InstallAudioService()
{
    InstallLimits();
    InstallPamEnv();

#if INSTALL_JACK_SERVICE
    if (sysExec(GROUPADD_BIN " -f " JACK_SERVICE_GROUP_NAME) != EXIT_SUCCESS)
    {
        throw std::runtime_error("Failed to create jack service group.");
    }
    if (!userExists(JACK_SERVICE_ACCOUNT_NAME))
    {
        if (sysExec(USERADD_BIN " " JACK_SERVICE_ACCOUNT_NAME " -g " JACK_SERVICE_GROUP_NAME " -m -N -r") != EXIT_SUCCESS)
        {
            //  throw std::runtime_error("Failed to create service account.");
        }
        // lock account for login.
        silentSysExec("passwd -l " JACK_SERVICE_ACCOUNT_NAME);
    }
    // Add to audio groups.
    // sysExec(USERMOD_BIN " -a -G " JACK_SERVICE_GROUP_NAME " " JACK_SERVICE_ACCOUNT_NAME);
    sysExec(USERMOD_BIN " -a -G" AUDIO_SERVICE_GROUP_NAME " " JACK_SERVICE_ACCOUNT_NAME);

// If ONBOARDING, add to bluetooth group.
#ifdef ONBOARDING
    sysExec(USERMOD_BIN " -a -G bluetooth " JACK_SERVICE_ACCOUNT_NAME);
#endif

    // deploy the systemd service file
    std::map<std::string, std::string> map; // nothing to customize.

    WriteTemplateFile(map, GetServiceFileName(JACK_SERVICE));

    MaybeStartJackService();
#endif
}

int SudoExec(int argc, char **argv)
{
    // re-execute with SUDO in order to prompt for SUDO credentials once only.
    std::vector<char *> args;
    std::string pkexec = "/usr/bin/sudo"; // staged because "ISO C++ forbids converting a string constant to std::vector<char*>::value_type"(!)
    args.push_back((char *)(pkexec.c_str()));

    std::string sPath = getSelfExePath();
    args.push_back(const_cast<char *>(sPath.c_str()));
    for (int arg = 1; arg < argc; ++arg)
    {
        args.push_back(const_cast<char *>(argv[arg]));
    }

    args.push_back(nullptr);

    char **newArgs = &args[0];

    int pbPid;
    int returnValue = 0;

    if ((pbPid = fork()) == 0)
    {
        return execv(newArgs[0], newArgs);
    }
    else
    {
        waitpid(pbPid, &returnValue, 0);
        int exitStatus = WEXITSTATUS(returnValue);
        return exitStatus;
    }
}

static bool IsP2pServiceEnabled()
{
    WifiDirectConfigSettings settings;
    settings.Load();
    return settings.enable_;
}

void Uninstall()
{
    // if NetworkManager isn't installed, Install will have failed.
    // Do cleanup as if the Isntall had not failed.

    PretendNetworkManagerIsInstalled();

    try
    {
        OnWifiUninstall(true);

        StopService();
        DisableService();

#if UNINSTALL_JACK_SERVICE
        silentSysExec(SYSTEMCTL_BIN " stop jack");
        silentSysExec(SYSTEMCTL_BIN " disable jack");
#endif

        try
        {
            fs::remove("/usr/bin/systemd/system/" OLD_SHUTDOWN_SERVICE ".service");
        }
        catch (...)
        {
        }

        try
        {
            fs::remove("/usr/bin/systemd/system/" ADMIN_SERVICE ".service");
        }
        catch (...)
        {
        }
        try
        {
            fs::remove("/usr/bin/systemd/system/" PIPEDAL_NM_P2PD_SERVICE ".service");
        }
        catch (...)
        {
        }
        try
        {
            fs::remove("/usr/bin/systemd/system/" PIPEDAL_P2PD_SERVICE ".service");
        }
        catch (...)
        {
        }

        try
        {
            fs::remove("/usr/bin/systemd/system/" PIPEDALD_SERVICE ".service");
        }
        catch (...)
        {
        }

        try
        {
            fs::remove("/usr/bin/systemd/system/" JACK_SERVICE ".service");
        }
        catch (...)
        {
        }
        UninstallPamEnv();
        AvahiUninstall();

        // UninstallLimits();
        sysExec(SYSTEMCTL_BIN " daemon-reload");
    }
    catch (const std::exception &e)
    {
        // We should NEVER get here. But the consequences of failure are high (causes permanent apt installs/uninstall problems), so be safe.
        cout << "ERROR: Unexpected error while uninstalling pipedal. (" << e.what() << ")" << endl;
    }
    catch (...)
    {
        // We should NEVER get here. But the consequences of failure are high (causes permanent apt installs/uninstall problems), so be safe.
        cout << "ERROR: Unexpected error while uninstalling pipedal." << endl;
    }
}

std::random_device randdev;

std::string MakeUuid()
{
    uuid_t uuid;
    uuid_generate(uuid);
    char strUid[36 + 1];
    uuid_unparse(uuid, strUid);
    return strUid;
}

static std::string RandomChars(int nchars)
{
    std::stringstream s;

    std::uniform_int_distribution distr(0, 26 + 26 + 10 - 1);

    for (int i = 0; i < nchars; ++i)
    {
        int v = distr(randdev);
        char c;
        if (v < 10)
        {
            c = (char)('0' + v);
        }
        else
        {
            v -= 10;
            if (v < 26)
            {
                c = (char)('a' + v);
            }
            else
            {
                v -= 26;
                c = (char)('A' + v);
            }
        }
        s << c;
    }
    return s.str();
}

static std::map<fs::path, fs::perms> sPermissionExceptions({{"/var/pipedal/config/NetworkManagerP2P.json",
                                                             fs::perms::owner_read | fs::perms::owner_write | fs::perms::group_read | fs::perms::group_write

}});

void SetVarPermissions(
    const fs::path &path,
    fs::perms directoryPermissions,
    fs::perms filePermissions,
    uid_t uid, gid_t gid)
{
    using namespace std::filesystem;
    try
    {
        if (fs::exists(path))
        {
            std::ignore = chown(path.c_str(), uid, gid);
            if (fs::is_directory(path))
            {
                fs::permissions(path, directoryPermissions, fs::perm_options::replace);
                for (const auto &entry : fs::recursive_directory_iterator(path))
                {

                    if (chown(entry.path().c_str(), uid, gid) != 0)
                    {
                        std::cout << "Error: failed to set ownership of file " << entry.path() << std::endl;
                    }

                    try
                    {
                        if (fs::is_directory(entry.path()))
                        {
                            fs::permissions(entry.path(), directoryPermissions, fs::perm_options::replace);
                        }
                        else
                        {
                            fs::permissions(entry.path(), filePermissions, fs::perm_options::replace);
                        }
                    }
                    catch (const std::exception &e)
                    {
                        std::cout << "Error: failed to set permissions on file " << entry.path() << std::endl;
                    }
                }
            }
            else
            {
                if (sPermissionExceptions.contains(path))
                {
                    fs::permissions(path, sPermissionExceptions[path], fs::perm_options::replace);
                }
                else
                {
                    fs::permissions(path, filePermissions, fs::perm_options::replace);
                }
            }
        }
        else
        {
            std::cout << "Error: Path does not exist: " << path << std::endl;
        }
    }
    catch (const fs::filesystem_error &e)
    {
        std::cerr << "Error: " << e.what() << std::endl;
    }
}

namespace fs = std::filesystem;

static void FixPermissions()
{
    namespace fs = std::filesystem;
    using namespace std::filesystem;

    fs::create_directories("/var/pipedal");

    fs::perms directoryPermissions =
        fs::perms::owner_read | fs::perms::owner_write | fs::perms::owner_exec |
        fs::perms::group_read | fs::perms::group_write | fs::perms::group_exec |
        fs::perms::others_read | fs::perms::others_exec |
        fs::perms::set_gid;

    fs::perms filePermissions =
        fs::perms::owner_read | fs::perms::owner_write |
        fs::perms::group_read | fs::perms::group_write |
        fs::perms::others_read;

    uid_t uid;
    struct passwd *passwd;
    if ((passwd = getpwnam("pipedal_d")) == nullptr)
    {
        cout << "Error: "
             << "User 'pipedal_d' does not exist." << endl;
        return;
    }
    uid = passwd->pw_uid;
    gid_t gid = passwd->pw_gid;
    SetVarPermissions("/var/pipedal", directoryPermissions, filePermissions, uid, gid);

    fs::perms wpa_supplicant_perms =
        fs::perms::owner_read | fs::perms::owner_write |
        fs::perms::group_read | fs::perms::group_write;

    fs::path wpa_config_path{"/etc/pipedal/config/wpa_supplicant/wpa_supplicant-pipedal.conf"};
    try
    {
        fs::permissions(wpa_config_path, wpa_supplicant_perms, fs::perm_options::replace);

        struct group *grp = getgrnam("netdev");
        if (grp != nullptr)
        {
            if (chown(wpa_config_path.c_str(), 0, grp->gr_gid) != 0)
            {
                cout << "Error: Failed to change ownership of " << wpa_config_path << endl;
            }
        }
    }
    catch (const std::exception &e)
    {
        cout << "Error: Failed to set permissions on " << wpa_config_path << ". " << e.what() << endl;
    }
}
static void PrepareServiceConfigurationFile(uint16_t portNumber)
{
    ServiceConfiguration serviceConfiguration;
    try
    {
        serviceConfiguration.Load();
    }
    catch (const std::exception &)
    {
    }
    if (serviceConfiguration.deviceName == "" || serviceConfiguration.uuid == "" || portNumber != serviceConfiguration.server_port)
    {
        if (serviceConfiguration.deviceName == "")
        {
            serviceConfiguration.deviceName = "PiPedal";
        }
        if (serviceConfiguration.uuid == "")
        {
            serviceConfiguration.uuid = MakeUuid();
        }
        serviceConfiguration.server_port = portNumber;
        serviceConfiguration.Save();
    }
}

void CopyWpaSupplicantConfigFile()
{
    try
    {
        const char NM_SUPPLICANT_CONFIG_PATH[] = "/etc/pipedal/config/wpa_supplicant/wpa_supplicant-pipedal.conf";
        const char NM_SUPPLICANT_CONFIG_SOURCE_PATH[] = "/etc/pipedal/config/templates/wpa_supplicant-pipedal.conf";
        fs::path destinationPath = NM_SUPPLICANT_CONFIG_PATH;
        fs::path sourcePath = NM_SUPPLICANT_CONFIG_SOURCE_PATH;

        fs::create_directories(destinationPath.parent_path());
        fs::copy_file(sourcePath, destinationPath, fs::copy_options::overwrite_existing);
    }
    catch (const std::exception &e)
    {
        cout << "ERROR: " << e.what() << endl;
    }
}

void DeployVarConfig()
{
    fs::path varConfig = "/var/pipedal/config/config.json";
    if (!fs::exists(varConfig))
    {
        auto directory = varConfig.parent_path();
        fs::create_directories(directory);
        fs::path templateFile = "/etc/pipedal/config/templates/var_config.json";
        try
        {
            fs::copy(templateFile, varConfig);
        }
        catch (const std::exception &e)
        {
            std::cout << "Error: Failed to create " << varConfig << std::endl;
        }
    }
}

void InstallPgpKey()
{
    fs::path homeDir = "/var/pipedal/config/gpg";
    fs::path keyPath = "/etc/pipedal/config/updatekey.asc";
    fs::path keyPath2 = "/etc/pipedal/config/updatekey2.asc";

    fs::create_directories(homeDir);
    std::ignore = chmod(homeDir.c_str(), 0700);

    {
        std::stringstream s;
        s << (CHOWN_BIN " " SERVICE_GROUP_NAME ":" SERVICE_GROUP_NAME " ") << homeDir.c_str();
        sysExec(s.str().c_str());
    }
    {
        std::ostringstream ss;
        ss << "/usr/bin/gpg  --homedir " << homeDir.c_str() << " --import " << keyPath.c_str();
        int rc = silentSysExec(ss.str().c_str());
        if (rc != EXIT_SUCCESS)
        {
            cout << "Error: Failed  to create update keyring." << endl;
        }
    }
    {
        std::ostringstream ss;
        ss << "/usr/bin/gpg  --homedir " << homeDir.c_str() << " --import " << keyPath2.c_str();
        int rc = silentSysExec(ss.str().c_str());
        if (rc != EXIT_SUCCESS)
        {
            cout << "Error: Failed  to add update key." << endl;
        }
    }
    {
        std::stringstream ss;
        ss << (CHOWN_BIN " -R " SERVICE_GROUP_NAME ":" SERVICE_GROUP_NAME " ") << homeDir.c_str();
        std::string cmd = ss.str();
        sysExec(cmd.c_str());
    }
}
void Install(const fs::path &programPrefix, const std::string endpointAddress)
{
    cout << "Configuring pipedal" << endl;

    if (!UsingNetworkManager())
    {
        throw std::runtime_error("The current OS is not using NetworkManager. Services not configured.");
    }
    try
    {
        DeployVarConfig();

        if (sysExec(GROUPADD_BIN " -f " AUDIO_SERVICE_GROUP_NAME) != EXIT_SUCCESS)
        {
            throw std::runtime_error("Failed to create audio service group.");
        }
        if (sysExec(GROUPADD_BIN " -f " SERVICE_GROUP_NAME) != EXIT_SUCCESS)
        {
            throw std::runtime_error("Failed to create pipedald service group.");
        }
        // defensively disable wifi p2p if some leftover config file left it enabled.
#ifdef P2PD_DISABLED
        try
        {
            if (IsP2pServiceEnabled())
            {
                WifiDirectConfigSettings settings;
                settings.Load();
                settings.enable_ = false;
                settings.Save();
            }
        }
        catch (const std::exception &)
        {
        }
#endif

        InstallAudioService();
        auto endpos = endpointAddress.find_last_of(':');
        if (endpos == string::npos)
        {
            throw std::runtime_error("Invalid endpoint address: " + endpointAddress);
        }
        uint16_t port;
        auto strPort = endpointAddress.substr(endpos + 1);
        try
        {
            auto lport = std::stoul(strPort);
            if (lport == 0 || lport >= std::numeric_limits<uint16_t>::max())
            {
                throw std::runtime_error("out of range.");
            }
            port = (uint16_t)lport;
            std::stringstream s;
            s << port;
            strPort = s.str(); // normalized.
        }
        catch (const std::exception &)
        {
            std::stringstream s;
            s << "Invalid port number: " << strPort;
            throw std::runtime_error(s.str());
        }
        PrepareServiceConfigurationFile(port);

        bool authBindRequired = port < 512;

        // Create and configure service account.

        if (sysExec(GROUPADD_BIN " -f " SERVICE_GROUP_NAME) != EXIT_SUCCESS)
        {
            throw std::runtime_error("Failed to create service group.");
        }

        if (!userExists(SERVICE_ACCOUNT_NAME))
        {
            if (sysExec(USERADD_BIN " " SERVICE_ACCOUNT_NAME " -g " SERVICE_GROUP_NAME " -m --home /var/pipedal/home -N -r") != EXIT_SUCCESS)
            {
                //  throw std::runtime_error("Failed to create service account.");
            }
            // lock account for login.
            silentSysExec("passwd -l " SERVICE_ACCOUNT_NAME);
        }

        // Add to audio groups.
        sysExec(USERMOD_BIN " -a -G  " AUDIO_SERVICE_GROUP_NAME " " SERVICE_ACCOUNT_NAME);
        // add to netdev group
        sysExec(USERMOD_BIN " -a -G  " NETDEV_GROUP_NAME " " SERVICE_ACCOUNT_NAME);


        // create and configure /var directory.

        fs::path varDirectory("/var/pipedal");
        fs::create_directory(varDirectory);

        {
            std::stringstream s;
            s << CHGRP_BIN " " SERVICE_GROUP_NAME " " << varDirectory.c_str();
            sysExec(s.str().c_str());
        }
        {
            std::stringstream s;
            s << CHOWN_BIN << " " << SERVICE_ACCOUNT_NAME << " " << varDirectory.c_str();
            sysExec(s.str().c_str());
        }

        {
            std::stringstream s;
            s << CHMOD_BIN << " 775 " << varDirectory.c_str();
            sysExec(s.str().c_str());
        }
        {
            std::stringstream s;
            s << CHMOD_BIN << " g+s " << varDirectory.c_str(); // child files/directories inherit ownership.
            sysExec(s.str().c_str());
        }

        // authbind port.

        if (authBindRequired)
        {
            fs::create_directories("/etc/authbind/byport");
            fs::path portAuthFile = fs::path("/etc/authbind/byport") / strPort;

            {
                // create it.
                std::ofstream f(portAuthFile);
                if (!f.is_open())
                {
                    throw std::runtime_error("Failed to create " + portAuthFile.string());
                }
            }
            {
                // own it.
                std::stringstream s;
                s << CHOWN_BIN << " " SERVICE_ACCOUNT_NAME " " << portAuthFile.c_str();
                sysExec(s.str().c_str());
            }
            {
                // group own it.
                std::stringstream s;
                s << CHGRP_BIN << " " SERVICE_GROUP_NAME " " << portAuthFile.c_str();
                sysExec(s.str().c_str());
            }
            {
                std::stringstream s;
                s << CHMOD_BIN << " 770 " << portAuthFile.c_str();
                sysExec(s.str().c_str());
            }
        }

        std::map<string, string> map;

        int shutdownPort = 3147;
        map["DESCRIPTION"] = "PiPedal Web Service";
        {
            std::stringstream s;

            if (authBindRequired)
            {
                s << findOnPath("authbind").string() << " --deep ";
            }
            s
                << (programPrefix / "sbin" / PIPEDALD_SERVICE).string()
                << " /etc/pipedal/config /etc/pipedal/react -systemd";

            map["COMMAND"] = s.str();
        }
        WriteTemplateFile(map, GetServiceFileName(PIPEDALD_SERVICE));

        map["DESCRIPTION"] = "PiPedal Admin Service";
        {
            std::stringstream s;

            s
                << (programPrefix / "sbin" / ADMIN_SERVICE).string()
                << " -port " << shutdownPort;

            map["COMMAND"] = s.str();
        }
        WriteTemplateFile(map, GetServiceFileName(ADMIN_SERVICE));

        // /usr/bin/pipedal_p2pd --config-file /etc/pipedal/config/template_p2pd.conf

        if (UsingNetworkManager())
        {
            std::string pipedal_nm_p2pd_cmd = SS(
                (programPrefix / "sbin" / "pipedal_nm_p2pd").string()
                //<< " --config-file " << PIPEDAL_P2PD_CONF_PATH
            );
            map["COMMAND"] = pipedal_nm_p2pd_cmd;

            WriteTemplateFile(map, GetServiceFileName(PIPEDAL_NM_P2PD_SERVICE));
        }
        else
        {
            std::string pipedal_p2pd_cmd = SS(
                (programPrefix / "sbin" / "pipedal_p2pd").string()
                << " --config-file " << PIPEDAL_P2PD_CONF_PATH);
            map["COMMAND"] = pipedal_p2pd_cmd;

            WriteTemplateFile(map, GetServiceFileName(PIPEDAL_P2PD_SERVICE));
        }
        CopyWpaSupplicantConfigFile();

        sysExec(SYSTEMCTL_BIN " daemon-reload");

        FixPermissions();

        StopService(false);
        AvahiInstall();
        InstallPgpKey();
        StartService(false);

        EnableService();

        // Restart Wi-Fi Direct if neccessary.
        OnWifiReinstall();
    }
    catch (const std::exception &e)
    {
        // don't allow abnormal termination, which leaves the package in a state that's
        // difficult to uninstall.
        cout << "Error: " << e.what();
        cout << "      Run 'pipedalconfig  --install' again to complete setup of PiPedal." << endl;
    }
}

static std::string GetCurrentWebServicePort()
{
    PiPedalConfiguration config;
    config.Load("/etc/pipedal/config", "");

    std::ostringstream ss;
    ss << config.GetSocketServerPort();
    return ss.str();
}

static void PrintHelp()
{
    PrettyPrinter pp;
    pp << "pipedalconfig - Command-line post-install configuration for PiPedal"
       << "\n"
       << "Copyright (c) 2022-2024 Robin Davies."
       << "\n"
       << "\n"
       << "See https://rerdavies.github.io/pipedal/Documentation.html for "
       << "online documentation."
       << "\n"
       << "\n"
       << "Syntax:"
       << "\n";
    pp << Indent(4)
       << "pipedalconfig [Options...]"
       << "\n"
       << "\n";
    pp.Indent(0)
        << "Options: "
        << "\n\n";
    pp.Indent(20);

    pp
        << HangingIndent() << "    -h --help\t"
        << "Display this message."
        << "\n"
        << "\n"

        << HangingIndent() << "    --install [--port <port#>]\t"
        << "Install or re-install PiPedal services and service accounts."
        << "\n"
        << "\n"
        << "The --port option controls which TCP/IP port the web server uses."
        << "\n\n"

        << HangingIndent() << "    --uninstall\t"
        << "Remove installed services."
        << "\n"
        << "\n"

        << HangingIndent() << "    --enable\t"
        << "Start PiPedal services at boot time."
        << "\n"
        << "\n"

        << HangingIndent() << "    --disable\tDo not start PiPedal services at boot time."
        << "\n"
        << "\n"
        << HangingIndent() << "    --start\tStart PiPedal services."
        << "\n"
        << "\n"
        << HangingIndent() << "    --stop\tStop PiPedal services."
        << "\n"
        << "\n"
        << HangingIndent() << "    --restart\tRestart PiPedal services."
        << "\n"
        << "\n"
        << HangingIndent() << "    --enable-hotspot\t <country_code> <ssid> <wep_password> <channel> [<hotspot-option>]"
        << "\n\nEnable Wi-Fi hotspot."
        << "\n\n"
        << "PiPedal's Wi-Fi hotspot allows you to connect to your Raspberry Pi when you don't have a Wi-Fi router, for example, when you are "
        << "performing away from home. It is most particularly useful when using the PiPedal Android client. Consult PiPedal's online documentation "
        << "for guidance on configuring a Wi-Fi hotspot."
        << "\n\n"
        << "One of the following hotspot options can be specifed. If no hotspot option is given, the hotspot will always be enabled. "
        << "If one of the hotspot options are given, the PiPedal server will turn the hotspot on or off automatically, as conditions change."
        <<  "\n\n"
        << "--home-network <wifi-ssid>\n"
        << AddIndent(4) << "Hotspot is disabled if the specificed Wi-Fi network is detected.\n"
        << AddIndent(-4)
        << "--no-ethernet\n" 
        << AddIndent(4) << "Hotspot is disabled if an ethernet network is connected.\n"
        << AddIndent(-4)
        << "--no-wifi\n"
        << AddIndent(4) << "Hotspot is disabled if a remembered Wi-Fi network is detected.\n"
        << AddIndent(-4)
        << "\n"
        << "Caution: Wi-Fi connections are disabled when the hotspot is activated. If you currently access your Raspberry Pi using  "
        << "Wi-Fi, choose your hotspot options carefully, to ensure that you can still access your Raspberry Pi."
        << "\n\n"
        << "country_code is the 2-letter ISO-3166 country code for "
        << "the country you are in. see below for further notes."
        << "\n\n"
        << "If the Wi-Fi channel is not specified, a Wi-Fi channel will be automatically selected."
        << "\n\n"

        << HangingIndent() << "    --disable-hotspot\tDisabled the Wi-Fi hotspot."
        << "\n\n"
        << HangingIndent() << "    --list-wifi-channels [<country_code>] \tList valid Wifi channels for the current/specified country."
        << "\n\n"

        << Indent(0) << "Country codes:"
        << "\n\n"
        << Indent(4)
        << "Country codes are used to determine Wi-Fi-regulatory regime - the "
        << "channels you are legally allowed to use, and the features which must "
        << "be enabled or disabled on the channels you can use. "
        << "\n\n"
        << "Without a country code, Wi-Fi must be restricted to channels 1 through 11 "
        << "with reduced amplitude and feature sets."
        << "\n\n"
        << "For the most part, Wi-Fi country codes are taken from the list of ISO 3661 "
        << "2-letter country codes; although there are a handful of exceptions for small "
        << "countries and islands. See the Alpha-2 code column of "
        << "\n\n"
        << Indent(8) << "https://en.wikipedia.org/wiki/List_of_ISO_3166_country_codes."
        << "\n\n";
}

static int ListP2PChannels(const std::vector<std::string> &arguments)
{
    try
    {
        std::string country;
        if (arguments.size() >= 2)
        {
            throw std::runtime_error("Invalid arguments.");
        }
        if (arguments.size() == 1)
        {
            country = arguments[0];
        }
        else
        {
            // use the currently selected country by default.
            WifiDirectConfigSettings settings;
            settings.Load();
            if (settings.countryCode_.empty())
            {
                throw std::runtime_error("A country has not yet been selected. Please provde a country code.");
            }
            country = settings.countryCode_;
        }
        auto channelSelectors = getWifiChannelSelectors(country.c_str(), true);
        for (const auto &channelSelector : channelSelectors)
        {
            cout << channelSelector.channelName_ << endl;
        }
    }
    catch (const std::exception &e)
    {
        cout << "ERROR: " << e.what() << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}

void RequireNetworkManager()
{
    if (!UsingNetworkManager())
    {
        throw std::runtime_error("The current OS is not using NetworkManager.");
    }
}
int main(int argc, char **argv)
{
    CommandLineParser parser;
    bool install = false;
    bool uninstall = false;
    bool help = false;
    bool helpError = false;
    bool stop = false, start = false;
    bool enable = false, disable = false, restart = false;
    bool enable_hotspot = false, disable_hotspot = false;
    bool enable_p2p = false, disable_p2p = false;
    bool list_p2p_channels = false;
    bool get_current_port = false;
    bool fix_permissions = false;
    bool nosudo = false;
    bool excludeShutdownService = false;
    std::string prefixOption;
    std::string portOption;
    std::string homeNetwork;
    bool noEthernet = false;
    bool noWifi = false;

    parser.AddOption("--nosudo", &nosudo); // strictly a debugging aid. Run without sudo, until we fail because of permissions.
    parser.AddOption("--install", &install);
    parser.AddOption("--uninstall", &uninstall);
    parser.AddOption("--stop", &stop);
    parser.AddOption("--start", &start);
    parser.AddOption("--enable", &enable);
    parser.AddOption("--disable", &disable);
    parser.AddOption("--restart", &restart);
    parser.AddOption("-h", &help);
    parser.AddOption("--help", &help);
    parser.AddOption("--prefix", &prefixOption);
    parser.AddOption("--port", &portOption);
    parser.AddOption("--enable-hotspot", &enable_hotspot);
    parser.AddOption("--disable-hotspot", &disable_hotspot);
    parser.AddOption("--home-network",&homeNetwork);
    parser.AddOption("--no-ethernet",&noEthernet);
    parser.AddOption("--no-wifi",&noWifi);
    try
    {
        parser.Parse(argc, (const char **)argv);

        int actionCount =
            help + get_current_port + install + uninstall + stop + start + enable + disable + enable_hotspot + disable_hotspot + restart + enable_p2p + disable_p2p + list_p2p_channels + fix_permissions;
        if (actionCount > 1)
        {
            throw std::runtime_error("Please provide only one action.");
        }
        if (argc == 1)
        {
            help = true;
        }
        else if (actionCount == 0)
        {

            throw std::runtime_error("No action provided.");
        }
        if ((!enable_p2p) && (!enable_hotspot) && (!list_p2p_channels))
        {
            if (parser.Arguments().size() != 0)
            {
                cout << "Error: Unexpected argument : " << parser.Arguments()[0] << "\n";
                helpError = true;
            }
        }
        int hotspotOptionCount =
            (homeNetwork.length() != 0 ? 1: 0)
            + noEthernet
            + noWifi;

        if (enable_hotspot)
        {
            if (hotspotOptionCount > 1)
            {
                throw std::runtime_error("Only one hotspot option at a time can be specified.");
            }
        } else {
            if (hotspotOptionCount > 0)
            {
                throw std::runtime_error("Hotspot options only only valid when the --enable-hotspot option has been supplied.");
            }
        }
    }
    catch (const std::exception &e)
    {
        std::string s = e.what();
        cout << "Error: " << s << "\n";
        helpError = true;
    }
    if (helpError)
    {
        cout << endl;
        return EXIT_FAILURE; // don't scroll the error off the screen.
    }

    if (help)
    {
        PrintHelp();
        return EXIT_SUCCESS;
    }
    if (get_current_port)
    {
        std::cout << "current port: " << GetCurrentWebServicePort() << std::endl;
        return EXIT_SUCCESS;
    }
    if (list_p2p_channels)
    {
        return ListP2PChannels(parser.Arguments());
    }
    if (portOption.size() != 0 && !install)
    {
        cout << "Error: -port option can only be specified with the -install option."
             << "\n";
        exit(EXIT_FAILURE);
    }

    auto uid = getuid();
    if (uid != 0 && !nosudo)
    {
        return SudoExec(argc, argv);
    }

    try
    {
        if (fix_permissions)
        {
            FixPermissions();
            return EXIT_SUCCESS;
        }
        if (install)
        {
            try {
                fs::path prefix;
                if (prefixOption.length() != 0)
                {
                    prefix = fs::path(prefixOption);
                }
                else
                {
                    prefix = fs::path(argv[0]).parent_path().parent_path();

                    fs::path pipedalPath = prefix / "sbin" / "pipedald";
                    if (!fs::exists(pipedalPath))
                    {
                        std::stringstream s;
                        s << "Can't find pipedald executable at " << pipedalPath << ". Try again using the -prefix option.";
                        throw std::runtime_error(s.str());
                    }
                }

                if (portOption == "")
                {
                    portOption = GetCurrentWebServicePort();
                    if (portOption == "")
                    {
                        portOption = "80";
                    }
                }
                if (portOption.find(':') == string::npos)
                {
                    portOption = "0.0.0.0:" + portOption;
                }
                Install(prefix, portOption);
                FileSystemSync();

            } catch (const std::exception&e)
            {
                cout << "ERROR: " << e.what() << endl;
                FileSystemSync();
                return EXIT_SUCCESS; // say we succeeded so we don't put APT into a hellish state.
            }
        }
        else if (uninstall)
        {
            try {
                Uninstall();
                FileSystemSync();
            } catch (const std::exception &e)
            {
                cout << "ERROR: " << e.what() << endl;
                FileSystemSync();
                return EXIT_SUCCESS; // Say we succeeds so that we don't put APT into a hellish state.
            }
        }
        else if (stop)
        {
            RequireNetworkManager();
            StopService();

        }
        else if (start)
        {
            RequireNetworkManager();
            StartService();
        }
        else if (restart)
        {
            RequireNetworkManager();
            RestartService(excludeShutdownService);
        }
        else if (enable)
        {
            RequireNetworkManager();
            EnableService();
            FileSystemSync();

        }
        else if (disable)
        {
            RequireNetworkManager();
            DisableService();
            FileSystemSync();

        }
        else if (enable_p2p)
        {
            throw std::runtime_error("Wi-Fi p2p connections are no longer supported. Use hotspots instead.");
            // try
            // {
            //     auto argv = parser.Arguments();
            //     WifiDirectConfigSettings settings;
            //     settings.ParseArguments(argv);
            //     settings.valid_ = true;
            //     settings.enable_ = true;
            //     SetWifiDirectConfig(settings);
            //     RestartService(true); // also have to retart web service so that it gets the correct device name.
            // }
            // catch (const std::exception &e)
            // {
            //     cout << "ERROR: " << e.what() << endl;
            //     return EXIT_FAILURE;
            // }
        }
        else if (disable_p2p)
        {
            RequireNetworkManager();
            WifiDirectConfigSettings settings;
            settings.Load();
            settings.enable_ = false;
            SetWifiDirectConfig(settings);
            
            
            RestartService(true);
            return EXIT_SUCCESS;
        }
        else if (enable_hotspot)
        {
            RequireNetworkManager();

            auto argv = parser.Arguments();
            WifiConfigSettings settings;

            HotspotAutoStartMode startMode = HotspotAutoStartMode::Always;
            if (homeNetwork.length() != 0)
            {
                startMode = HotspotAutoStartMode::NotAtHome;
            } else if (noEthernet)
            {
                startMode = HotspotAutoStartMode::NoEthernetConnection;
            } else if (noWifi)
            {
                startMode = HotspotAutoStartMode::NoRememberedWifiConections;
            }

            settings.ParseArguments(argv,startMode,homeNetwork);

            if (settings.hasPassword_)
            {
                settings.hasSavedPassword_ = true;
            }
            SetWifiConfig(settings);
            if (silentSysExec(SYSTEMCTL_BIN " restart " PIPEDALD_SERVICE ".service") != EXIT_SUCCESS)
            {
                throw std::runtime_error("Failed to restart the " PIPEDALD_SERVICE " service.");
            }
            FileSystemSync();
        }
        else if (disable_hotspot)
        {
            RequireNetworkManager();

            WifiConfigSettings settings;
            settings.valid_ = true;
            settings.autoStartMode_ = (uint16_t)HotspotAutoStartMode::Never;
            SetWifiConfig(settings);
            if (silentSysExec(SYSTEMCTL_BIN " restart " PIPEDALD_SERVICE ".service") != EXIT_SUCCESS)
            {
                throw std::runtime_error("Failed to restart the " PIPEDALD_SERVICE " service.");
            }
            FileSystemSync();
        }
    }
    catch (const std::exception &e)
    {
        cout << "Error: " << e.what() << "\n";
        exit(EXIT_FAILURE);
    }
}