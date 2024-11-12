// Copyright (c) 2022 Robin Davies
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

#include "pch.h"
#include "util.hpp"
#include "Locale.hpp"
#include "AudioConfig.hpp"
#include "WebServer.hpp"
#include <iostream>
#include "Lv2Log.hpp"
#include "ServiceConfiguration.hpp"
#include "AvahiService.hpp"
#include "WebServerConfig.hpp"
#include <execinfo.h>
#include "PiPedalSocket.hpp"
#include "PluginHost.hpp"
#include <boost/system/error_code.hpp>
#include <filesystem>
#include "PiPedalConfiguration.hpp"
#include "AdminClient.hpp"
#include "CommandLineParser.hpp"
#include "Lv2SystemdLogger.hpp"
#include <sys/stat.h>
#include <boost/asio.hpp>
#include "HtmlHelper.hpp"
#include <thread>
#include <atomic>

#include <signal.h>
#include <semaphore.h>

#include <systemd/sd-daemon.h>

using namespace pipedal;

#ifdef __ARM_ARCH_ISA_A64
#define AARCH64
#endif

class application_category : public boost::system::error_category
{
public:
    const char *name() const noexcept { return "PiPedal"; }
    std::string message(int ev) const { return "error message"; }
};

static std::atomic<bool> g_SigBreak = false;
static std::atomic<bool> g_restart = false;

void sig_handler(int signo)
{
    // we're using sig_wait. No need to do anything.
}

void throwSystemError(int error)
{
}

static bool isJackServiceRunning()
{
    // look for the jack shmem .
    std::filesystem::path path = "/dev/shm/jack_default_0";
    return std::filesystem::exists(path);
}

static void AsanCheck()
{
    char *t = new char[5];
    t[5] = 'x';
    delete t;
    exit(EXIT_FAILURE);
}

#if ENABLE_BACKTRACE
void segvHandler(int sig) {
    void *array[10];

    // Get void*'s for all entries on the stack
    size_t size;
    size = backtrace(array, 10);

    // Print out all the frames to stderr
    const char *message = "Error: SEGV signal received.\n";
    auto _ = write(STDERR_FILENO,message,strlen(message));

    backtrace_symbols_fd(array+2, size-2, STDERR_FILENO);
    _exit(EXIT_FAILURE);
}


static void EnableBacktrace()
{
    signal(SIGSEGV, segvHandler); 

}
#endif

static bool TryGetLogLevel(const std::string&strLogLevel,LogLevel*result)
{
    if (strLogLevel == "debug") {*result= LogLevel::Debug; return true;}
    if (strLogLevel == "info") {*result= LogLevel::Info; return true;}
    if (strLogLevel == "warning") {*result= LogLevel::Warning; return true;}
    if (strLogLevel == "error") { *result= LogLevel::Error; return true;}
    *result = LogLevel::Info;
    return false;
}


int main(int argc, char *argv[])
{

#ifndef WIN32
    umask(002); // newly created files in /var/pipedal get 7cd75-ish permissions, which improves debugging/live-service interaction.
#endif

#if ENABLE_BACKTRACE
    EnableBacktrace();
#endif

    // Check command line arguments.
    bool help = false;
    bool error = false;
    bool systemd = false;
    bool testExtraDevice = false;
    std::string logLevel;
    std::string portOption;

    CommandLineParser parser;
    parser.AddOption("-h", &help);
    parser.AddOption("--help", &help);
    parser.AddOption("-systemd", &systemd);
    parser.AddOption("-log-level",&logLevel);
    parser.AddOption("-port", &portOption);
    parser.AddOption("-test-extra-device", &testExtraDevice); // advertise two different devices (for testing multi-device connect)

    try
    {
        parser.Parse(argc, (const char **)argv);
        if (parser.Arguments().size() > 2)
        {
            throw PiPedalException("Too many arguments.");
        }
        if (parser.Arguments().size() == 0)
        {
            throw PiPedalException("<config_root> not provided.");
        }
        if (help || parser.Arguments().size() == 0)
        {
            std::cout << "pipedald - Pipedal web socket server.\n"
                         "Copyright (c) 2022-2024 Robin Davies.\n"
                         "\n";
        }
    }
    catch (const std::exception &e)
    {
        std::cout << "Error: " << e.what() << "\n\n";
        error = true;
        help = true;
    }

    if (help)
    {
        std::cout << "Usage: pipedald <doc_root> [<web_root>] [options...]\n\n"
                  << "Options:\n"
                  << "   -systemd: Log to systemd journals instead of to the console.\n"
                  << "   -port: Port to listen on e.g. 80, or 0.0.0.0:80\n"
                  << "   -log-level: (debug|info|warning|error)"
                  << "Example:\n"
                  << "    pipedald /etc/pipedal/config /etc/pipedal/react -port 80 \n"
                     "\n"
                     "Description:\n\n"
                     "    Configuration is read from <doc_root>/config.json\n"
                     "\n"
                     "    If <web_root> is not provided, pipedal will serve from <doc_root>\n"
                     "\n"
                     "    While debugging, bind the port to 0.0.0.0:8080, and connect to the default React\n"
                     "    server that's provided when you run 'npm run start' in 'react/src'. By default, the\n"
                     "    React debug server will connect to the socket server on 0.0.0.0:8080.\n\n";
        return error ? EXIT_FAILURE : EXIT_SUCCESS;
    }

    if (systemd)
    {
        Lv2Log::set_logger(MakeLv2SystemdLogger());
    }
    SetThreadName("main");

    std::filesystem::path doc_root = parser.Arguments()[0];
    std::filesystem::path web_root = doc_root;
    if (parser.Arguments().size() >= 2)
    {
        web_root = parser.Arguments()[1];
    }

    PiPedalConfiguration configuration;
    try
    {
        configuration.Load(doc_root, web_root);
    }
    catch (const std::exception &e)
    {
        std::stringstream s;
        s << "Unable to read configuration from '" << (doc_root / "config.json") << "'. (" << e.what() << ")";
        Lv2Log::error(s.str());
        return EXIT_SUCCESS; // indicate to systemd that we don't want a restart.
    }

    Lv2Log::log_level(configuration.GetLogLevel());
    if (logLevel.length() != 0)
    {
        LogLevel lv2LogLevel;
        if (TryGetLogLevel(logLevel,&lv2LogLevel))
        {
            Lv2Log::log_level(lv2LogLevel);

        } else {
            Lv2Log::error(SS("Invalid log level: " << logLevel));
            return EXIT_SUCCESS; // indicate to systemd that we don't want a restart.
        }
    }

    if (portOption.length() != 0)
    {
        configuration.SetSocketServerEndpoint(portOption);
    }

    uint16_t port;
    std::shared_ptr<WebServer> server;
    try
    {
        auto const address = boost::asio::ip::make_address(configuration.GetSocketServerAddress());
        port = static_cast<uint16_t>(configuration.GetSocketServerPort());

        auto const threads = std::max<int>(1, configuration.GetThreads());

        server = WebServer::create(
            address, port, web_root.c_str(), threads, configuration.GetMaxUploadSize());

        Lv2Log::info("Document root: %s Threads: %d", doc_root.c_str(), (int)threads);

        server->SetLogHttpRequests(configuration.LogHttpRequests());
    }
    catch (const std::exception &e)
    {
        std::stringstream s;
        s << "Fatal error: " << e.what() << std::endl;
        Lv2Log::error(s.str());
        return EXIT_SUCCESS; // indiate to systemd that we don't want a restart.
    }

    try
    {
        {
            {
                auto locale = Locale::GetInstance();
                Lv2Log::info(SS("Locale: " << locale->CurrentLocale()));
                try
                {
                    auto collator = locale->GetCollator();
                }
                catch (std::exception &e)
                {
                    Lv2Log::error(e.what());
                    return EXIT_SUCCESS; // tell systemd not to auto-restart.
                }
            }
            // only accept signals on the main thread.
            // block now so that threads spawned by model inherit the block mask.
            int sig;
            sigset_t sigSet;
            int s;
            sigemptyset(&sigSet);
            sigaddset(&sigSet, SIGINT);
            sigaddset(&sigSet, SIGTERM);
            sigaddset(&sigSet, SIGUSR1);

            s = pthread_sigmask(SIG_BLOCK, &sigSet, NULL);
            if (s != 0)
            {
                throw std::logic_error("pthread_sigmask failed.");
            }

            PiPedalModel model;

            model.SetNetworkChangedListener(
                [&server]() mutable
                {
                    if (server)
                    {
                        server->DisplayIpAddresses();
                    }
                });

            model.SetRestartListener(
                []()
                {
                    g_restart = true;
                    raise(SIGTERM); // throws an exception under gdb, but correctly restarts the service when running live.
                });

            model.Init(configuration);

            // Get heavy IO out of the way before letting dependent (Jack/ALSA) services run.
            model.LoadLv2PluginInfo();
            if (systemd)
            {
                // Tell systemd we're done.
                sd_notifyf(0, "READY=1\n"
                              "MAINPID=%lu",
                           (unsigned long)getpid());
            }

            model.WaitForAudioDeviceToComeOnline();

#if JACK_HOST
            if (systemd)
            {

                if (!isJackServiceRunning())
                {
                    Lv2Log::info("Waiting for Jack service.");
                    // wait up to 15 seconds for the jack service to come online.
                    for (int i = 0; i < 15; ++i)
                    {
                        // use the time to prepopulate ALSA device cache before jack
                        // opens the device and we can't read properties.
                        model.GetAlsaDevices();

                        sleep(1);
                        if (isJackServiceRunning())
                        {
                            break;
                        }
                    }
                }
                if (isJackServiceRunning())
                {
                    Lv2Log::info("Found  Jack service.");
                    sleep(3); // jack needs a little time to get up to speed.
                }
                else
                {
                    Lv2Log::info("Jack service not started.");
                }
                model.GetAlsaDevices();
            }
#endif

            model.Load();

            auto pipedalSocketFactory = MakePiPedalSocketFactory(model);

            server->AddSocketFactory(pipedalSocketFactory);

            ConfigureWebServer(*server, model, port, configuration.GetMaxUploadSize());
            {
                server->RunInBackground(-1);

                model.StartHotspotMonitoring();

                {
                    sigwait(&sigSet, &sig);

                    if (systemd)
                    {
                        sd_notify(0, "STOPPING=1");
                    }
                }
            }

            Lv2Log::info("Closing audio session.");
            server->StopListening(); // prevents premature reconnect attempts while we're shutting down clients in an orderly manner.
            model.Close();

            Lv2Log::info("Stopping web server.");
            server->ShutDown(5000);
            server->Join();
        }
        Lv2Log::info("Shutdown complete.");

        if (g_restart)
            return EXIT_FAILURE; // indicate to systemd that we want a restart.
    }
    catch (const std::exception &e)
    {
        Lv2Log::error(e.what());
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS; // only exit in response to a signal.
}
