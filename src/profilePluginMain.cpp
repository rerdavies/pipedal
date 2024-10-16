#include "PiPedalModel.hpp"
#include "PiPedalConfiguration.hpp"
#include "Pedalboard.hpp"
#include "Lv2Log.hpp"
#include "RingBufferReader.hpp"
#include "CommandLineParser.hpp"
#include <thread>
#include <chrono>
#include "ss.hpp"

// apt install google-perftools
#include <gperftools/profiler.h>
#include "ArmPerformanceCounters.hpp"

using namespace pipedal;
using namespace std;
namespace fs = std::filesystem;

// discards data from the ring buffer without discarding it.

using WriterRingbuffer = RingBuffer<false, true>;

class RingBufferSink
{
public:
    RingBufferSink(WriterRingbuffer &writerRingBuffer)
        : writerRingbuffer(writerRingBuffer)
    {
        thread = std::make_unique<std::thread>(
            [this]()
            {
                ThreadProc();
            });
    }
    void Close()
    {
        if (!closed)
        {
            closed = true;
            writerRingbuffer.close();
            terminateThread = true;
            thread->join();
            thread = nullptr;
        }
    }
    ~RingBufferSink()
    {
        Close();
    }

private:
    void ThreadProc()
    {
        std::vector<uint8_t> dataVector(1024);
        uint8_t *data = dataVector.data();
        while (true)
        {
            RingBufferStatus status = writerRingbuffer.readWait_for(std::chrono::milliseconds(10));
            if (status == RingBufferStatus::Closed)
            {
                break;
            }
            if (status == RingBufferStatus::Ready)
            {
                size_t available = writerRingbuffer.readSpace();
                while (available != 0)
                {
                    size_t thisTime = std::min(dataVector.size(), available);
                    writerRingbuffer.read(thisTime, data);
                    available -= thisTime;
                }
            }
        }
    }
    bool closed = false;
    std::atomic<bool> terminateThread{false};

    std::unique_ptr<std::thread> thread;
    WriterRingbuffer &writerRingbuffer;
};
struct ProfileOptions
{
    std::string presetName;
    std::string presetFileName;
    float benchmark_seconds = 20;
    std::string outputFilename = "/tmp/profilePlugin.perf";
    bool noProfile = false;
    bool waitForWork = false;
    size_t frameSize = 64;
};

void profilePlugin(const ProfileOptions &profileOptions)
{

#ifdef PIPEDAL_AARCH64
    CacheMissCounter cacheMissCounter;
#endif

    size_t nFrames = profileOptions.frameSize;
    Lv2Log::log_level(LogLevel::Info);

    /*** Initialize the model */
    PiPedalModel model;
    Lv2Log::log_level(LogLevel::Error);
    fs::path doc_root = "/etc/pipedal/config";
    PiPedalConfiguration configuration;
    try
    {
        configuration.Load(doc_root, "");
    }
    catch (const std::exception &e)
    {
        std::stringstream s;
        s << "Unable to read configuration from '" << (doc_root / "config.json") << "'. (" << e.what() << ")";
        throw std::runtime_error(s.str());
    }

    model.Init(configuration);

    model.LoadLv2PluginInfo();

    // model.Load(); don't start audio.

    /* *** Load the preset.  */

    bool presetValid = false;

    if (profileOptions.presetName.length() != 0)
    {
        PresetIndex presetIndex;
        model.GetPresets(&presetIndex);

        int64_t presetIndexId = 0;
        bool presetIndexFound = false;
        for (const auto &preset : presetIndex.presets())
        {
            if (preset.name() == profileOptions.presetName)
            {
                presetIndexId = preset.instanceId();
                presetIndexFound = true;
                break;
            }
        }
        if (!presetIndexFound)
        {
            throw std::runtime_error("Preset not found.");
        }
        model.LoadPreset(-1, presetIndexId);
    }
    else if (profileOptions.presetFileName.length() != 0)
    {
        std::ifstream f(profileOptions.presetFileName);
        if (!f.is_open())
        {
            throw std::runtime_error(SS("Unable to load preset file " << profileOptions.presetFileName << "."));
        }
        try
        {
            json_reader reader(f);
            BankFile bankFile;
            reader.read(&bankFile);
            if (bankFile.presets().size() != 1)
            {
                throw new std::runtime_error(SS("Invalid preset file. Expection one preset, but " << bankFile.presets().size() << " presets were found."));
            }
            Pedalboard pedalboard = (bankFile.presets()[0]->preset());
            model.SetPedalboard(-1,pedalboard);
        }
        catch (const std::exception &e)
        {
            throw std::runtime_error(SS("Invalid file format:  " << profileOptions.presetFileName << ". " << e.what() ));
        }
    }
    else
    {
        throw std::runtime_error("You must specify either a preset name or a preset file.");
    }

    auto pedalboard = model.GetCurrentPedalboardCopy();
    cout << "uri: " << pedalboard.items()[0].uri() << endl;
    /* *** Get and prepare the audio thread pedalboard. */
    auto lv2Pedalboard = model.GetLv2Pedalboard();

    lv2Pedalboard->Activate();

    std::vector<float> inputBufferVector;
    std::vector<float> outputBufferVector;
    inputBufferVector.resize(nFrames);
    outputBufferVector.resize(nFrames);

    float *inputBuffers[2]{inputBufferVector.data(), nullptr};
    float *outputBuffers[2] = {inputBufferVector.data(), nullptr};

    WriterRingbuffer writerRingbuffer;
    RealtimeRingBufferWriter ringBufferWriter(&writerRingbuffer);

    RingBufferSink ringBufferSink(writerRingbuffer);

    // run once to get memory allocations in NAM and ML out of the way.
    lv2Pedalboard->Run(inputBuffers, outputBuffers, nFrames, &ringBufferWriter);

    /* *** Pump the plugin for a bit if it is expected to do work on the scheduler  thread when initializing */
    if (profileOptions.waitForWork)
    {
        using clock = std::chrono::steady_clock;

        // idle, pumping the plugin occasionally to allow inital scheduler work to complete.
        auto waitStart = clock::now();
        auto waitDuration = std::chrono::duration_cast<clock::duration>(std::chrono::seconds(3));

        while (true)
        {
            lv2Pedalboard->Run(inputBuffers, outputBuffers, nFrames, &ringBufferWriter);

            auto elapsed = clock::now() - waitStart;
            if (elapsed >= waitDuration)
            {
                break;
            }
            std::this_thread::sleep_for(std::chrono::milliseconds(100)); // the scheduler thread runs.
        }
    }

    /* *** Run the benchmark */
    if (!profileOptions.noProfile)
    {
        ProfilerStart(profileOptions.outputFilename.c_str());
    }
#ifdef PIPEDAL_AARCH64
    cacheMissCounter.start();
#endif

    auto startTime = std::chrono::system_clock::now();

    size_t repetitions = static_cast<size_t>(48000 * profileOptions.benchmark_seconds / nFrames);
    for (size_t i = 0; i < repetitions; ++i)
    {
        lv2Pedalboard->Run(inputBuffers, outputBuffers, nFrames, &ringBufferWriter);
    }

    auto elapsedTime = std::chrono::high_resolution_clock::now() - startTime;

#ifdef PIPEDAL_AARCH64
    cacheMissCounter.stop();
#endif
    if (!profileOptions.noProfile)
    {
        ProfilerStop();
    }
    std::chrono::milliseconds us = std::chrono::duration_cast<chrono::milliseconds>(elapsedTime);
    std::cout << "Ellapsed time: " << (us.count() / 1000.0) << "s" << endl;
#ifdef PIPEDAL_AARCH64
    std::cout << "L1 Cache Misses: " << cacheMissCounter.get_l1_misses() << endl;
    // non-functional on Raspberry PI OS.
    // std::cout << "L2 Cache Misses: " << cacheMissCounter.get_l2_misses()  << endl;
#endif

    ringBufferSink.Close();

    lv2Pedalboard->Deactivate();
}

int main(int argc, char **argv)
{
    try
    {
        ProfileOptions profileOptions;
        bool help = false;
        CommandLineParser commandLineParser;
        std::string presetFileName;
        commandLineParser.AddOption("w", "wait-for-work", &profileOptions.waitForWork);
        commandLineParser.AddOption("", "no-profile", &profileOptions.noProfile);
        commandLineParser.AddOption("p", "preset-file", &presetFileName);
        commandLineParser.AddOption("o", "output", &profileOptions.outputFilename);
        commandLineParser.AddOption("s", "seconds", &profileOptions.benchmark_seconds);
        commandLineParser.AddOption("h", "help", &help);

        commandLineParser.Parse(argc, (const char **)argv);

        bool argumentError = false;
        if (commandLineParser.Arguments().size() == 1 && presetFileName.length() == 0)
        {
            cerr << "Error: You must supply either a preset name, or a preset file name" << endl;
            argumentError = true;
        }

        if (argumentError || help)
        {
            cout << "profilePlugin - Generate profiling data for LV2 Plugins" << endl;
            cout << "Copyright (c) 2024 Robin E. R. Davies" << endl;
            cout << endl;
            cout << "Syntax:  profilePlugin [preset_name] [options...]" << endl;
            cout << "         where preset_name is the name of a PiPedal preset." << endl;
            cout << endl;
            cout << "          A google-perf profile capture will be written to " << endl;
            cout << "          /tmp/profilePlugin.perf" << endl;
            cout << endl;
            cout << "Options:" << endl;
            cout << "    --no-profile:" << endl;
            cout << "          do NOT generate a perf file." << endl;
            cout << "    -p, --preset-file filename:" << endl;
            cout << "          Load the specified preset file. " << endl;
            cout << "    -o, --output filename:" << endl;
            cout << "          The file to which profiler data is written. " << endl;
            cout << "          Defaults to /tmp/profilePlugin.perf" << endl;
            cout << "    -w, --wait-for-work: " << endl;
            cout << "          Assume that the plugin will load data on the LV2 scheduler thread." << endl;
            cout << "    -s, --seconds time_in_seconds: " << endl;
            cout << "          The number of seconds of audio to process." << endl;
            cout << "    -h, --help:  display this message." << endl;
            cout << endl;
            return help ? EXIT_SUCCESS : EXIT_FAILURE;
        }

        if (commandLineParser.Arguments().size() != 0)
        {
            profileOptions.presetName = commandLineParser.Arguments()[0];
        }
        profileOptions.presetFileName = presetFileName;

        profilePlugin(profileOptions);
    }
    catch (const std::exception &e)
    {
        cerr << "Error: " << e.what() << endl;
        return EXIT_FAILURE;
    }
    return EXIT_SUCCESS;
}