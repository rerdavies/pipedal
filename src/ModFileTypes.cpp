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

#include "ModFileTypes.hpp"
#include "util.hpp"
#include <stdexcept>
#include <iostream>
#include "ss.hpp"

using namespace pipedal;

const std::vector<ModFileTypes::ModDirectory> ModFileTypes::ModDirectories =
    {
        {"audioloop", "shared/audio/Loops", "Loops", {"audio/*"}}, // Audio Loops, meant to be used for looper-style plugins
        //"audiorecording","shared/audio/Audio Recordings", {"audio/*"}}, : Audio Recordings, triggered by plugins and stored in the unit
        {"audiosample", "shared/audio/Samples", "Samples", {"audio/*"}}, // One-shot Audio Samples, meant to be used for sampler-style plugins
        {"audiotrack", "shared/audio/Tracks", "Tracks", {"audio/*"}},    // Audio Tracks, meant to be used as full-performance/song or backtrack
        {"cabsim", "CabIR", "Cab IRs", {"audio/*"}},                     // Speaker Cabinets, meant as small IR audio files

        /// - h2drumkit: Hydrogen Drumkits, must use h2drumkit file extension
        {"ir", "ReverbImpulseFiles", "Impulse Responses", {"audio/*"}},    // Impulse Responses
        {"midiclip", "shared/midiClips", "MIDI Clips", {".mid", ".midi"}}, // MIDI Clips, to be used in sync with host tempo, must have mid or midi file extension
        {"midisong", "shared/midiSongs", "MIDI Songs", {".mid", ".midi"}}, // MIDI Songs, meant to be used as full-performance/song or backtrack
        {"sf2", "shared/sf2", "Sound Fonts", {"sf2", "sf3"}},              // SF2 Instruments, must have sf2 or sf3 file extension
        {"sfz", "shared/sfz", "Sfz Files", {"sfz"}},                       // SFZ Instruments, must have sfz file extension
        // extensions observed in the field.
        {"audio", "shared/audio", "Audio", {"audio/*"}},                              // all audio files (Ratatoille)
        {"nammodel", "NeuralAmpModels", "Neural Amp Models", {".nam"}},              // Ratatoille, Mike's NAM.
        {"aidadspmodel", "shared/aidaaix", "AIDA IAX Models", {".json", ".aidaiax"}}, // Ratatoille
        {"mlmodel", "ToobMlModels", "ML Models", {".json"}},                         //
        //
};

const ModFileTypes::ModDirectory *ModFileTypes::GetModDirectory(const std::string &type)
{
    for (const ModFileTypes::ModDirectory &modType : ModFileTypes::ModDirectories)
    {
        if (modType.modType == type)
        {
            return &modType;
        }
    }
    return nullptr;
}

ModFileTypes::ModFileTypes(const std::string &fileTypes)
{
    std::vector<std::string> types = split(fileTypes, ',');
    for (const auto &type : types)
    {
        const ModDirectory *wellKnownType = GetModDirectory(type);
        if (wellKnownType)
        {
            rootDirectories_.push_back(type);
        }
        else
        {
            fileTypes_.push_back(type);
        }
    }
    // for Ratatoille.lv2.
    // If rootDirectories contains "nammodel" and fileTypes contains "json", add "mlmodel" too.
    // if (contains(rootDirectories_,"nammodel") && contains(fileTypes_,"json"))
    // {
    //     if (!contains(rootDirectories_,"mlmodel"))
    //     {
    //         rootDirectories_.push_back("mlmodel");
    //     }
    // }
    if (fileTypes_.empty())
    {
        for (const auto &type : types)
        {
            const ModDirectory *wellKnownType = GetModDirectory(type);
            if (wellKnownType)
            {
                for (const std::string &newType : wellKnownType->defaultFileExtensions)
                {
                    bool found = false;
                    for (const std::string &oldType : fileTypes_)
                    {
                        if (newType == oldType)
                        {
                            found = true;
                            break;
                        }
                    }
                    if (!found)
                    {
                        fileTypes_.push_back(newType);
                    }
                }
            }
        }
    }
}

static std::filesystem::path getModDirectoryPath(
    const std::filesystem::path &rootDirectory,
    const std::string &modType)
{
    auto wellKnownType = ModFileTypes::GetModDirectory(modType);
    if (!wellKnownType)
    {
        throw std::runtime_error(SS("Can't find modFileType " << modType));
    }
    return rootDirectory / wellKnownType->pipedalPath;
}
void ModFileTypes::CreateDefaultDirectories(const std::filesystem::path &rootDirectory)
{
    namespace fs = std::filesystem;
    using namespace std;
    try
    {
        for (const auto &modType : ModDirectories)
        {
            fs::path path = rootDirectory / modType.pipedalPath;

            fs::create_directories(path);
        }

        if (!fs::exists(rootDirectory / "shared" / "audio" / "Cab IR Files"))
        {
            fs::create_symlink(
                getModDirectoryPath(rootDirectory, "cabsim"),
                rootDirectory / "shared" / "audio" / "Cab IR Files");
        }
        if (!fs::exists(rootDirectory / "shared" / "audio" / "Impulse Responses"))
        {
            fs::create_symlink(
                getModDirectoryPath(rootDirectory, "ir"),
                rootDirectory / "shared" / "audio" / "Impulse Responses");
        }
    }
    catch (const std::exception &e)
    {
        cout << "Can't create default MOD directories. " << e.what() << endl;
    }
}

