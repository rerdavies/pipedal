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
#include "catch.hpp"
#include <sstream>
#include <cstdint>
#include <string>
#include <iostream>
#include "AlsaSequencer.hpp"
#include <iomanip>

#include "PiPedalAlsa.hpp"

using namespace pipedal;
using namespace std;

static void DiscoveryTest()
{
    cout << "--- Discovery" << endl;
    PiPedalAlsaDevices devices;
    auto result = devices.GetAlsaDevices();
    std::cout << result.size() << " ALSA devices found." << std::endl;

    auto midiInputDevices = GetAlsaMidiInputDevices();
    std::cout << midiInputDevices.size() << " ALSA MIDI input devices found." << std::endl;
    auto midiOutputDevices = GetAlsaMidiOutputDevices();
    std::cout << midiOutputDevices.size() << " ALSA MIDI output devices found." << std::endl;
}

void EnumerateSequencers()
{
    cout << "--- Enumerating ALSA Sequencers" << endl;
    auto sequencers = AlsaSequencer::EnumeratePorts();
    for (const auto &sequencer : sequencers)
    {
        cout << sequencer.client << ": "<< sequencer.port << " - " << sequencer.clientName << "/" << sequencer.name;

        cout << " [" << (sequencer.isKernelDevice ? "Kernel" : "User");
        if (!sequencer.rawMidiDevice.empty())
        {
            cout << " " << sequencer.rawMidiDevice;
        }
        cout << "]" << endl;

        cout << "        Read: " << (sequencer.canRead ? "Yes" : "No")
             << ", Write: " << (sequencer.canWrite ? "Yes" : "No")
             << ", Read Subscribe: " << (sequencer.canReadSubscribe ? "Yes" : "No")
             << ", Write Subscribe: " << (sequencer.canWriteSubscribe ? "Yes" : "No") << std::endl;

        cout
            << "        System Announce : " << (sequencer.isSystemAnnounce ? " Yes " : " No ")
            << "UMP: "
            << (sequencer.isUmp ? "Yes" : "No")
            << "MIDI: "
            << (sequencer.isMidi ? "Yes" : "No")
            << "Application: "
            << (sequencer.isApplication ? "Yes" : "No")
            << " MIDI Synth: "
            << (sequencer.isMidiSynth ? "Yes" : "No")
            << "Synth: "
            << (sequencer.isSynth ? "Yes" : "No")
            << std::endl;
        cout << "       Specific: " << (sequencer.isSpecific ? "Yes" : "No")
             << "Hardware: " << (sequencer.isHardware ? "Yes" : "No")
             << "Software: " << (sequencer.isSoftware ? "Yes" : "No") << std::endl;
    }
}

void ReadFromsequencerTest()
{
    cout << "--- Reading from ALSA Sequencer" << endl;
    AlsaSequencer sequencer;
    sequencer.ConnectPort("V25 V25 In"); 
    
    AlsaMidiMessage message;
    while (true)
    {
        if (sequencer.ReadMessage(message))
        {
            cout << "Received MIDI message: "
                 << " " << hex << setfill('0') << setw(2) << static_cast<int>(message.cc0) 
                 << " " << hex << setfill('0') << setw(2) << static_cast<int>(message.cc1) 
                 << " " << hex << setfill('0') << setw(2) << static_cast<int>(message.cc2) 
                 << " Timestamp: " << setw(8) << dec << message.timestamp
                 << std::endl;
        }
    }
}

TEST_CASE("ALSA Seq Test", "[pipedal_alsa_seq_test][Build][Dev]")
{
    EnumerateSequencers();

    ReadFromsequencerTest();
}

TEST_CASE("ALSA Test", "[pipedal_alsa_test][Build][Dev]")
{

    DiscoveryTest();
}
