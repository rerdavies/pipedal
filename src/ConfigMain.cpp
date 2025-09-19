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
#include "ModFileTypes.hpp"
#include "alsaCheck.hpp"
#include "RegDb.hpp"
#include "Locale.hpp"
#include "Finally.hpp"
#include "BootConfig.hpp"

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

#include <pwd.h>
#include <unistd.h>

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

void changeUserShell(const char *username, const char *newShell)
{
    struct passwd *pw;
    struct passwd p;
    char buf[1024];

    pw = getpwnam(username);
    if (pw == nullptr)
    {
        throw std::runtime_error("User not found");
    }
    if (strcmp(pw->pw_shell, newShell) == 0)
    {
        return;
    }
    std::string args = SS("/usr/sbin/usermod -s " << newShell << " " << username);

    if (silentSysExec(args.c_str()) != EXIT_SUCCESS)
    {
        cout << "Failed to set shell for " << username << endl;
    }
}

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

bool disableAvahiConfigLine(SystemConfigFile &avahi, const std::string &section, const std::string &lineStart)
{
    bool changed = false;
    // use-ipv6=no
    int line = avahi.GetLineThatStartsWith(lineStart + "yes");
    if (line != -1)
    {
        avahi.UndoableReplaceLine(line, lineStart + "no");
        changed = true;
    }
    else
    {
        if (avahi.GetLineThatStartsWith(lineStart + "no") == -1)
        {
            line = avahi.GetLineThatStartsWith(section);
            if (line == -1)
            {
                throw std::runtime_error("Unable to find [server] section.");
            }
            {
                ++line;
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
            avahi.UndoableAddLine(line, lineStart + "no");
            changed = true;
        }
    }
    return changed;
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

        changed |= disableAvahiConfigLine(avahi, "[server]", "use-ipv6=");
        changed |= disableAvahiConfigLine(avahi, "[publish]", "publish-aaaa-on-ipv4=");

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

    // old limits config file bumped limits for members of the audio group, and didn't have a proper priority.
    // remove it
    fs::path oldLimitsConfig = "/etc/security/limits.d/audio.conf";
    if (fs::exists(oldLimitsConfig))
    {
        fs::remove(oldLimitsConfig);
    }

    // bump limits for members of the pipedal_d group.
    fs::path limitsConfig = "/etc/security/limits.d/90-pipedal.conf";

    if (!fs::exists(limitsConfig))
    {
        ofstream output(limitsConfig);
        output << "# Realtime priority limits for the pipedal services"
               << "\n";
        output << "@pipedal_d   -  rtprio     95" << "\n";
        output << "@pipedal_d   -  nice     -20" << "\n";
        output << "@pipedal_d   -  memlock    unlimited" << "\n";
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
            if (fs::is_symlink(path))
            {
                return;
            }
            std::ignore = chown(path.c_str(), uid, gid);
            if (fs::is_directory(path))
            {
                fs::permissions(path, directoryPermissions, fs::perm_options::replace);
                for (const auto &entry : fs::recursive_directory_iterator(path))
                {

                    if (chown(entry.path().c_str(), uid, gid) != 0)
                    {
                        // Expect spurious pulse config files to be un-chown-able.
                        // std::cout << "Error: failed to set ownership of file " << entry.path() << std::endl;
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
                        // spurious errors on pulse config files.
                        // std::cout << "Error: failed to set permissions on file " << entry.path() << std::endl;
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

static uint32_t autoSelectPorts[] = {
    80,
    81,   // unofficial alternate
    8080, // unofficial alternate
    8081, // unofficial alternate
    8008, // unofficial alternate
    83    // Not official at all.
};

static bool isPortInUse(int port)
{
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        throw std::runtime_error(SS("Socket creation failed: " << strerror(errno)));
    }
    Finally ff{[sockfd]()
               {
                   close(sockfd);
               }};
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    // Try to bind to the port
    int result = bind(sockfd, (struct sockaddr *)&addr, sizeof(addr));

    if (result < 0)
    {
        if (errno == EADDRINUSE)
        {
            return true; // Port is in use
        }
        else
        {
            cout << "Warning: Unexpected error  binding socket to port " << port << ": " << strerror(errno) << endl;
            return true;
        }
    }

    return false; // Port is free
}

static std::string AutoSelectPort()
{
    constexpr size_t MAX_PORT = sizeof(autoSelectPorts) / sizeof(autoSelectPorts[0]);

    int32_t port = -1;
    for (size_t i = 0; i < MAX_PORT; ++i)
    {
        if (!isPortInUse(autoSelectPorts[i]))
        {
            port = autoSelectPorts[i];
            break;
        }
    }
    if (port == -1)
    {
        cout << "Warning: Can't find an available HTTP port. Setting to port 80 anyway." << endl;
        port = 80;
    }
    else if (port != 80)
    {
        cout << "Warning: Port 80 is already in use. Using port " << port << " instead." << endl;
    }

    return SS(port);
}

bool SetWebServerPort(std::string portOption)
{

    try
    {
        auto nPos = portOption.find_last_of(':');
        std::string strPort;
        if (nPos != std::string::npos)
        {
            strPort = portOption.substr(nPos + 1);
        }
        else
        {
            strPort = portOption;
        }
        std::istringstream ss(strPort);

        uint16_t iPort = 0;
        ss >> iPort;
        if (!ss)
        {
            throw std::runtime_error(SS("Invalid port number: " << strPort));
        }

        StopService(false);

        PrepareServiceConfigurationFile(iPort);

        StartService(true);

        cout << "PiPedal web server is listening on port " << iPort << endl;
    }
    catch (const std::exception &e)
    {
        cout << "Error: " << e.what() << endl;
        return false;
    }
    return true;
}

void CheckPreemptDynamicConfig()
{
    try
    {
        BootConfig bootConfig;

        if (bootConfig.KernelType() == "PREEMPT_DYNAMIC")
        {
            if (bootConfig.DynamicScheduler() != BootConfig::DynamicSchedulerT::Full)
            {
                cout << endl;
                cout << "Warning: PREEMPT_DYNAMIC kernel has not been configured for 'preempt=full'" << endl;
                cout << "Run pipedal_kconfig to configure the kernel for real-time audio." << endl;
                cout << endl;
            }
        }
    }
    catch (const std::exception & /* ignore */)
    {
    }
}

void Install(const fs::path &programPrefix, const std::string endpointAddress)
{
    cout << "Configuring pipedal" << endl;

    if (!UsingNetworkManager())
    {
        cout << "Warning: The current OS is not using the NetworkManager network stack." << endl;
        cout << "   The PiPedal Auto-Hotspot feature will be disabled." << endl
             << endl;
        cout << "   Consult PiPedal documentation to find out how to correct this " << endl;
        cout << "   issue on Ubuntu Server." << endl;
    }
    try
    {

        // apply policy changes we dropped into the polkit configuration files (allows pipedal_d to use NetworkManager dbus apis).
        silentSysExec(SYSTEMCTL_BIN " restart polkit.service");

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
        uint16_t port = 80;
        auto strPort = endpointAddress.substr(endpos + 1);
        try
        {
            auto lport = std::stoul(strPort);
            if (lport == 0 || lport >= std::numeric_limits<uint16_t>::max())
            {
                throw std::runtime_error("Port number is too large.");
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
            if (sysExec(USERADD_BIN " " SERVICE_ACCOUNT_NAME " -g " SERVICE_GROUP_NAME " -s /usr/sbin/nologin -m --home /var/pipedal/home -N -r") != EXIT_SUCCESS)
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

        try
        {
            changeUserShell(SERVICE_ACCOUNT_NAME, "/usr/sbin/nologon");
        }
        catch (const std::exception &e)
        {
            cout << "Error: Can't set user shell for pipedal_d. " << e.what() << std::endl;
        }
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

        ModFileTypes::CreateDefaultDirectories("/var/pipedal/audio_uploads");
        FixPermissions();

        StopService(false);
        AvahiInstall();
        InstallPgpKey();
        StartService(false);

        EnableService();

        // Restart Wi-Fi Direct if neccessary.
        OnWifiReinstall();

        CheckPreemptDynamicConfig();
    }
    catch (const std::exception &e)
    {
        // don't allow abnormal termination, which leaves the package in a state that's
        // difficult to uninstall.
        cout << "Error: " << e.what() << endl;
        cout << "      Run 'pipedalconfig  --install' again to complete setup of PiPedal." << endl;
    }
    const std::string ADDR_ANY = "0.0.0.0:";

    if (endpointAddress.starts_with(ADDR_ANY))
    {
        cout << "PiPedal web server is listening on port " << endpointAddress.substr(ADDR_ANY.length()) << endl;
    }
    else
    {
        cout << "PiPedal web server is listening on port " << endpointAddress << endl;
    }
}

static std::string GetCurrentWebServicePort()
{
    PiPedalConfiguration config;
    config.Load("/etc/pipedal/config", "");

    std::ostringstream ss;
    ss << config.GetSocketServerAddress() << ":" << config.GetSocketServerPort();
    return ss.str();
}

static void PrintHelp()
{
    PrettyPrinter pp;
    pp << "pipedalconfig - Command-line post-install configuration for PiPedal"
       << "\n"
       << "Copyright (c) 2022-2024 Robin E. R. Davies."
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

        << HangingIndent() << "    --port <port#>\t"
        << "Set the TCP/IP port that the web server will use."
        << "\n"

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
        << "\n\n"
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
        << HangingIndent() << "    --alsa-devices\tList available ALSA hw devices."
        << "\n\n"
        << HangingIndent() << "    --alsa-device [device_id]\tPrint info for the specified ALSA device."
        << "\n\n"
        << HangingIndent() << "    --list-wifi-channels [<country_code>] \tList valid Wifi channels for the current/specified country."
        << "\n\n"
        << HangingIndent() << "    --list-wifi-country-codes\tList valid country codes."
        << "\n\n"

        << HangingIndent() << "    --fix-permissions\t"
        << "Set correct permissions on /var/pipedal directories and sub-directories\n\n"

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
        << " Wi-Fi country codes are taken from the list of ISO 3661 "
        << "2-letter country codes.\n\n"
        << "To see a list of countries and their country codes, run: \n\n"
        << Indent(8) << "pipedalconfig --list-wifi-country-codes"
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

void ListValidCountryCodes()
{
    RegDb regdb;

    auto ccs = regdb.getRegulatoryDomains();

    if (ccs.size() == 0)
    {
        cout << "No regulatory domains found." << endl;
    }
    else
    {
        using pair = std::pair<std::string, std::string>;
        std::vector<pair> list;
        for (auto &cc : ccs)
        {
            list.push_back(pair(cc.first, cc.second));
        }

        auto collator = Locale::GetInstance()->GetCollator();
        auto compare = [&collator](
                           pair &left,
                           pair &right)
        {
            return collator->Compare(
                       left.second, right.second) < 0;
        };
        std::sort(list.begin(), list.end(), compare);

        for (const auto &entry : list)
        {
            cout << entry.first << " - " << entry.second << endl;
        }
    }
}

static std::string ProcessPortOption(std::string portOption)
{
    if (portOption == "" || portOption == "0") // allow testing of port autodetect.
    {
        std::string t = GetCurrentWebServicePort();
        if (portOption != "0")
        { // allows easy debugging of port AutoSelectPort.
            portOption = t;
        }
        else
        {
            portOption = "";
        }
        if (portOption == "")
        {
            portOption = AutoSelectPort();
        }
    }
    std::string ADDR_ANY = "0.0.0.0:";

    if (portOption.find(':') == string::npos)
    {
        portOption = ADDR_ANY + portOption;
    }
    return portOption;
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
    bool alsaDevices = false;
    std::string alsaDevice;
    std::string prefixOption;
    std::string portOption;
    std::string homeNetwork;
    bool noEthernet = false;
    bool list_wifi_country_codes = false;
    bool noWifi = false;

    parser.AddOption("--nosudo", &nosudo); // strictly a debugging aid. Run without sudo, until we fail because of permissions.
    parser.AddOption("--list-wifi-country-codes", &list_wifi_country_codes);
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
    parser.AddOption("--home-network", &homeNetwork);
    parser.AddOption("--no-ethernet", &noEthernet);
    parser.AddOption("--no-wifi", &noWifi);
    parser.AddOption("--alsa-devices", &alsaDevices);
    parser.AddOption("--alsa-device", &alsaDevice);
    parser.AddOption("--fix-permissions", &fix_permissions);

    try
    {
        parser.Parse(argc, (const char **)argv);

        int actionCount =
            (!alsaDevice.empty()) + alsaDevices + help + get_current_port + install +
            uninstall + stop + start + enable + disable + enable_hotspot +
            disable_hotspot + restart + enable_p2p + disable_p2p + list_p2p_channels +
            fix_permissions + list_wifi_country_codes;

        if (actionCount == 0 && portOption.length() != 0)
        {
            ++actionCount;
        }
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

        if (portOption.length() != 0 && !install)
        {
            throw std::runtime_error("The --port option can only be specified in conjunction with the --install option.");
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
            (homeNetwork.length() != 0 ? 1 : 0) + noEthernet + noWifi;

        if (enable_hotspot)
        {
            if (hotspotOptionCount > 1)
            {
                throw std::runtime_error("Only one hotspot option at a time can be specified.");
            }
        }
        else
        {
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
    if (list_wifi_country_codes)
    {
        ListValidCountryCodes();
        return EXIT_SUCCESS;
    }
    if (get_current_port)
    {
        std::cout << "current port: " << GetCurrentWebServicePort() << std::endl;
        return EXIT_SUCCESS;
    }
    if (alsaDevices)
    {
        try
        {
            list_alsa_devices();
        }
        catch (const std::exception &e)
        {
            cerr << "Error: " << e.what() << endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    if (alsaDevice.length() != 0)
    {
        try
        {
            check_alsa_channel_configs(alsaDevice.c_str());
        }
        catch (const std::exception &e)
        {
            cerr << "Error: " << e.what() << endl;
            return EXIT_FAILURE;
        }
        return EXIT_SUCCESS;
    }
    if (list_p2p_channels)
    {
        return ListP2PChannels(parser.Arguments());
    }

    auto uid = getuid();
    if (uid != 0 && !nosudo)
    {
        return SudoExec(argc, argv);
    }

    try
    {
        if (portOption.length() != 0 && !install)
        {
            portOption = ProcessPortOption(portOption);
            SetWebServerPort(portOption);
            return EXIT_SUCCESS;
        }
        if (fix_permissions)
        {
            FixPermissions();
            return EXIT_SUCCESS;
        }
        if (install)
        {
            try
            {
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

                portOption = ProcessPortOption(portOption);

                Install(prefix, portOption);
                FileSystemSync();
            }
            catch (const std::exception &e)
            {
                cout << "ERROR: " << e.what() << endl;
                FileSystemSync();
                return EXIT_SUCCESS; // say we succeeded so we don't put APT into a hellish state.
            }
        }
        else if (uninstall)
        {
            try
            {
                Uninstall();
                FileSystemSync();
            }
            catch (const std::exception &e)
            {
                cout << "ERROR: " << e.what() << endl;
                FileSystemSync();
                return EXIT_SUCCESS; // Say we succeeds so that we don't put APT into a hellish state.
            }
        }
        else if (stop)
        {
            StopService();
        }
        else if (start)
        {
            StartService();
        }
        else if (restart)
        {
            RestartService(excludeShutdownService);
        }
        else if (enable)
        {
            EnableService();
            FileSystemSync();
        }
        else if (disable)
        {
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
            }
            else if (noEthernet)
            {
                startMode = HotspotAutoStartMode::NoEthernetConnection;
            }
            else if (noWifi)
            {
                startMode = HotspotAutoStartMode::NoRememberedWifiConections;
            }

            settings.ParseArguments(argv, startMode, homeNetwork);

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