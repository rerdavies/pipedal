/*
 * MIT License
 *
 * Copyright (c) 2022 Robin E. R. Davies
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "CommandLineParser.hpp"
#include "ss.hpp"
#include "PrettyPrinter.hpp"
#include <iostream>
#include "PiPedalAlsa.hpp"
#include "Lv2Log.hpp"
#include <mutex>
#include "AlsaDriver.hpp"
#include <iomanip>
#include <chrono>
#include <thread>
#include <sched.h>

using namespace pipedal;

constexpr int E_NOSIGNAL = 1;
constexpr int E_OPEN_FAILURE = 2;
constexpr int E_XRUN = 3;

constexpr uint64_t NO_SIGNAL_VALUE = 0x7000000;

struct TestResult
{
    int error = 0;
    uint64_t latency = 0;
    float cpuOverhead = 0;
};
void PrintHelp()
{
    PrettyPrinter pp;
    pp.width(78);

    pp << "PiPedal Latency Tester\n";
    pp << "Copyright (c) 2022 Robin Davies\n";
    pp << "\n";
    pp << Indent(0) << "Syntax\n\n";
    pp << Indent(2) << "pipedal_latency_test [<options>] <input-device> [<output-device>]\n\n";
    pp << "where <input-device> is the name of an ALSA capture device and <output-device> is the name of a playback device. "
          "If <output-device> is omitted, the input device will be used for both capture and playback. Typically the device names start with 'hw:'.\n\n";
    pp << Indent(0) << "Options\n\n";
    pp << Indent(15);

    pp << HangingIndent() << "  -l --list\t"
       << "List available devices.\n\n";
    pp << HangingIndent() << "  -r --rate\t"
       << "Sample rate (default 48000).\n\n";
    pp << HangingIndent() << "  -i --in_channels\t"
       << "Input channels. Command-seperated list. e.g.: 0,3. Default: all channels.\n\n";
    pp << HangingIndent() << "  -o --out_channels\t"
       << "Output channels. Command-seperated list. e.g.: 0,1,4. Default: all channels.\n\n";

    pp << HangingIndent() << "  -h --help\t"
       << "Display this message.\n\n";

    pp << Indent(0) << "Remarks\n\n";
    pp << Indent(2);

    pp << "PiPedal Latency Tester measures actual audio latency from output to input of an ALSA device.\n\n"
       << "To run a latency test, you must connect an audio cable from left (first) output of the device "
          "under test to the left (first) input of the device under test.\n\n"

       << "PiPedal Latency Tester  measures internal buffer delays as well as operating system and  "
       << "signal delays in hardware peripherals. Latency figures will therefore be somewhat higher than  "
       << "most reported latency figures which typically only include internal buffer delays.\n\n";

    pp
        << "The tests run over a variety of buffer sizes. A nominal compute load is provided in order to put some "
           "stress on the audio system.\n\n"

        << "You may need to stop the pipedald audio service in order to access the ALSA device:\n\n"
        << Indent(6) << "sudo systemctl stop pipedald\n\n";

    pp << Indent(0) << "Examples\n\n";
    pp << Indent(2) << "pipedal_latency_test --list\n\n";
    pp << Indent(2) << "pipedal_latency_test hw:M2 hw:M2\n";
    pp << Indent(2) << "pipedal_latency_test hw:M2 hw:Device2\n\n";
}

void ListDevices()
{
    PiPedalAlsaDevices alsaDevices;
    auto devices = alsaDevices.GetAlsaDevices();

    PrettyPrinter pp;
    if (devices.size() == 0)
    {
        pp << "No devices found.\n";
    }
    else
    {
        pp << Indent(0);
        pp << "Alsa Devices\n";
        pp << Indent(15);

        for (auto &device : devices)
        {
            pp << HangingIndent() << (device.id_) << "\t"
               << device.longName_ << "\n";
        }
    }
}

using ChannelsT = std::vector<int>;

class AlsaTester : private AudioDriverHost
{
public:
    enum class TestType
    {
        Oscillator,
        LatencyMonitor,
        NullTest
    };

private:
    AudioDriver *audioDriver = nullptr;

    const std::string &inputDeviceId;
    const std::string &outputDeviceId;
    ChannelsT inputChannels;
    ChannelsT outputChannels;
    uint32_t sampleRate;
    int bufferSize;
    int buffers;

public:
    AlsaTester(
        const std::string &inputDeviceId,
        const std::string &outputDeviceId,
        const ChannelsT &inputChannels,
        const ChannelsT &outputChannels,
        uint32_t sampleRate, int bufferSize, int buffers)
        : inputDeviceId(inputDeviceId),
          outputDeviceId(outputDeviceId),
          sampleRate(sampleRate),
          inputChannels(inputChannels),
          outputChannels(outputChannels),
          bufferSize(bufferSize),
          buffers(buffers)
    {
    }
    ~AlsaTester()
    {
        delete audioDriver;
        delete[] inputBuffers;
        delete[] outputBuffers;
    }
    std::vector<std::string> SelectChannels(const std::vector<std::string> &available, const std::vector<int> &selection)
    {
        if (selection.size() == 0)
            return available;

        std::vector<std::string> result;
        for (int sel : selection)
        {
            if (sel < 0 || sel >= available.size())
            {
                throw PiPedalArgumentException(SS("Invalid channel: " + sel));
            }
            result.push_back(available[sel]);
        }
        return result;
    }

    TestResult Test()
    {
        TestResult result;
        try
        {
            JackServerSettings serverSettings(inputDeviceId, outputDeviceId, sampleRate, bufferSize, buffers);

            JackConfiguration jackConfiguration;
            jackConfiguration.AlsaInitialize(serverSettings);

            auto &availableInputs = jackConfiguration.inputAudioPorts();
            auto &availableOutputs = jackConfiguration.outputAudioPorts();

            std::vector<std::string> inputAudioPorts, outputAudioPorts;

            inputAudioPorts = SelectChannels(availableInputs, this->inputChannels);
            outputAudioPorts = SelectChannels(availableOutputs, this->outputChannels);

            JackChannelSelection channelSelection(
                inputAudioPorts, outputAudioPorts,
                std::vector<AlsaMidiDeviceInfo>());

            audioDriver = CreateAlsaDriver(this);

            latencyMonitor.Init(jackConfiguration.sampleRate());
            audioDriver->Open(serverSettings, channelSelection);

            inputBuffers = new float *[channelSelection.GetInputAudioPorts().size()];
            outputBuffers = new float *[channelSelection.GetOutputAudioPorts().size()];

            audioDriver->Activate();

            std::this_thread::sleep_for(std::chrono::milliseconds(3000));

            this->SetXruns(0);

            std::this_thread::sleep_for(std::chrono::milliseconds(7000));

            audioDriver->Deactivate();
            audioDriver->Close();

            if (this->GetXruns() != 0)
            {
                result.error = E_XRUN;
                return result;
            }
            result.latency = latencyMonitor.GetLatency();
            if (result.latency == NO_SIGNAL_VALUE)
            {
                result.error = E_NOSIGNAL;
            }
            result.cpuOverhead = audioDriver->CpuOverhead();
        }
        catch (const std::exception &e)
        {
            result.error = E_OPEN_FAILURE;
        }
        return result;
    }

    float **inputBuffers = nullptr;
    float **outputBuffers = nullptr;

    class Oscillator
    {
    private:
        double dx = 0;
        double x = 0;
        double dx2 = 0;
        double x2 = 0;

    public:
        void Init(float frequency, size_t sampleRate)
        {
            dx = frequency * 3.141592736 * 2 / sampleRate;
            dx2 = 0.5 * 3.141592736 * 2 / sampleRate;
        }

        float Next()
        {
            float result = (float)std::cos(x);
            float env = (float)std::cos(x2);
            x += dx;
            x2 += dx2;
            return result * env;
        }
    };

    class LatencyMonitor
    {
        enum class State
        {
            Idle,
            Waiting,
        };
        State state = State::Idle;
        uint64_t t;
        uint64_t idle_samples;
        uint64_t waiting_samples;
        size_t current_latency = 0;
        size_t latency = 0;
        std::mutex sync;

    public:
        void Init(uint64_t sampleRate)
        {
            idle_samples = (uint64_t)(sampleRate * 0.5);
            waiting_samples = (uint64_t)(sampleRate * 0.5);

            state = State::Idle;
            t = idle_samples;
            latency = 0;
        }
        void StartTest()
        {
        }

        size_t GetLatency()
        {
            std::lock_guard lock{sync};
            return latency;
        }
        float Next(float input)
        {
            switch (state)
            {
            default:
            case State::Idle:
            {
                if (t-- == 0)
                {
                    state = State::Waiting;
                    current_latency = 0;
                }
                return 0.001;
            }
            break;
            case State::Waiting:
            {
                if (std::abs(input) > 0.1 || current_latency >= 2000)
                {
                    {
                        std::lock_guard lock{sync};
                        if (latency >= 2000)
                        {
                            latency = NO_SIGNAL_VALUE;
                        }
                        else
                        {
                            latency = current_latency;
                        }
                    }
                    state = State::Idle;
                    t = idle_samples;
                }
                else
                {
                    ++current_latency;
                }
                return current_latency < 100 ? 0.25 : 0.0;
            }
            break;
            }
        }
    };

    Oscillator oscillator;
    LatencyMonitor latencyMonitor;

    virtual void OnAlsaDriverStopped()
    {
    }
    virtual void OnAudioTerminated()
    {
    }
    virtual void OnProcess(size_t nFrames)
    {

        size_t inputs = audioDriver->MainInputBufferCount();
        size_t outputs = audioDriver->MainOutputBufferCount();

        for (size_t i = 0; i < inputs; ++i)
        {
            inputBuffers[i] = audioDriver->GetMainInputBuffer(i);
        }
        for (size_t i = 0; i < outputs; ++i)
        {
            outputBuffers[i] = audioDriver->GetMainOutputBuffer(i);
        }

        for (size_t i = 0; i < nFrames; ++i)
        {
            float v = latencyMonitor.Next(inputBuffers[0][i]);
            for (size_t c = 0; c < outputs; ++c)
            {
                outputBuffers[c][i] = v;
            }
        }
    }
    std::mutex sync;
    uint64_t xruns;

    uint64_t GetXruns()
    {
        lock_guard lock{sync};
        return xruns;
    }
    void SetXruns(uint64_t value)
    {
        lock_guard lock{sync};
        xruns = value;
    }
    virtual void OnUnderrun()
    {
        lock_guard lock{sync};
        ++xruns;
    }
};

TestResult RunLatencyTest(
    const std::string inputDeviceId,
    const std::string outputDeviceId,
    const ChannelsT &inputChannels,
    const ChannelsT &outputChannels,
    uint32_t sampleRate, int bufferSize, int buffers)
{
    AlsaTester tester(inputDeviceId, outputDeviceId, inputChannels, outputChannels, sampleRate, bufferSize, buffers);
    return tester.Test();
}

static std::string msDisplay(float value)
{
    std::stringstream s;
    s << fixed;
    s.precision(1);
    s << value << "ms";
    return s.str();
}
static std::string overheadDisplay(float value)
{
    std::stringstream s;
    s << setw(3) << value << "%";
    return s.str();
}

static bool testRealtimePriorityPrivileges()
{

    struct sched_param currentParam;
    int currentPolicy = sched_getscheduler(0);
    if (currentPolicy == -1) {
        return false;
    }
    if (sched_getparam(0, &currentParam) != 0) {
        return false;
    }
    struct sched_param param;
    param.sched_priority = 85;
    if (sched_setscheduler(0, SCHED_RR, &param) != 0)
    {
        return false;
    }
    // Restore normal priority
    sched_setscheduler(0, currentPolicy, &currentParam);
    return true;
}

void RunLatencyTest(
    const std::string &inputDeviceId,
    const std::string &outputDeviceId,
    const ChannelsT &inputChannels,
    const ChannelsT &outputChannels,
    uint32_t sampleRate)
{

    PrettyPrinter pp;

    if (!testRealtimePriorityPrivileges())
    {
        pp << "Unable to enable realtime scheduling. Add your user id to the pipedal_d group to fix this problem:" << "\n\n";
        pp.AddIndent(4);
        pp << "sudo usermod -a -G pipedal_d <your user id>" << "\n\n";
        pp.AddIndent(-4);
        pp << "You will need to log out or reboot your system in order for the change to take effect.\n\n";
        throw std::runtime_error("Unable to set relatime thread priority.");
    }

    pp << "Input: " << inputDeviceId << "  Output: " << outputDeviceId << "  Rate: " << sampleRate << "\n\n";

    const int SIZE_COLUMN_WIDTH = 8;
    const int BUFFERS_COLUMN_WIDTH = 20;

    static int bufferCounts[] = {2, 3, 4};
    static int bufferSizes[] = {16, 24, 32, 48, 64, 128};

    pp << Column(SIZE_COLUMN_WIDTH) << "Buffers\n";
    pp << "Size";

    int column = SIZE_COLUMN_WIDTH;
    for (auto bufferCount : bufferCounts)
    {
        pp << Column(column) << bufferCount;
        column += BUFFERS_COLUMN_WIDTH;
    }
    pp << "\n";

    for (auto bufferSize : bufferSizes)
    {
        pp << bufferSize;

        int column = SIZE_COLUMN_WIDTH;

        for (auto bufferCount : bufferCounts)
        {
            auto result = RunLatencyTest(inputDeviceId, outputDeviceId, inputChannels, outputChannels, sampleRate, bufferSize, bufferCount);

            pp.Column(column);
            column += BUFFERS_COLUMN_WIDTH;
            switch (result.error)
            {
            case E_NOSIGNAL:
                pp << "No signal";
                break;
            case E_OPEN_FAILURE:
                pp << "Failed";
                break;
            case E_XRUN:
                pp << "Xrun";
                break;

            default:
            {
                float ms = 1000.0f * result.latency / sampleRate;
                pp << result.latency << "/" << msDisplay(ms);
                break;
            }
            }
        }
        pp << "\n";
    }
}

ChannelsT ParseChannels(const std::string &channels)
{
    ChannelsT result;
    std::stringstream s(channels);

    while (true)
    {
        int c = s.peek();
        if (c == -1)
            break;

        if (c == ',')
        {
            s.get();
            c = s.peek();
        }
        if (c < '0' || c > '9')
        {
            throw PiPedalArgumentException("Invalid channel selection: " + channels);
        }
        int v = 0;
        while (s.peek() >= '0' && s.peek() <= '9')
        {
            c = s.get();
            v = v * 10 + c - '0';
        }
        result.push_back(v);
    }

    return result;
}

int main(int argc, const char **argv)
{

    Lv2Log::log_level(LogLevel::Warning);

    CommandLineParser parser;

    std::string deviceName;
    bool listDevices = false;
    bool help = false;
    uint32_t sampleRate = 48000;
std:
    string strInputChannels, strOutputChannels;

    ChannelsT inputChannels, outputChannels;

    parser.AddOption("l", "list", &listDevices);
    parser.AddOption("h", "help", &help);
    parser.AddOption("r", "rate", &sampleRate);
    parser.AddOption("i", "in_channels", &strInputChannels);
    parser.AddOption("o", "out_channels", &strOutputChannels);

    try
    {
        parser.Parse(argc, argv);

        if (help)
        {
            PrintHelp();
        }
        else if (listDevices)
        {
            ListDevices();
        }
        else if (parser.Arguments().size() >= 1 && parser.Arguments().size() <= 2)
        {
            inputChannels = ParseChannels(strInputChannels);
            outputChannels = ParseChannels(strOutputChannels);

            std::string inDev = parser.Arguments()[0];
            std::string outDev = parser.Arguments().size() == 2 ? parser.Arguments()[1] : inDev;
            RunLatencyTest(inDev, outDev, inputChannels, outputChannels, sampleRate);
        }
        else
        {
            PrintHelp();
        }
    }
    catch (std::exception &e)
    {
        cout << "Error: " << e.what() << endl;
        return 1;
    }
    return 0;
}