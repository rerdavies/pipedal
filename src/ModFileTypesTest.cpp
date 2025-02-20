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
#include <string>
#include "PiPedalUI.hpp"
#include "ModFileTypes.hpp"

using namespace pipedal;

std::set<std::string> CalculateFileExtensions(const std::string &mod__fileTypes, const std::string &modType)
{
    ModFileTypes modFileTypes(mod__fileTypes);

    auto fileProperty =
        std::make_shared<UiFileProperty>(
            "Test", "urn:test", modFileTypes);

    return fileProperty->GetPermittedFileExtensions(modType);
}

bool checkFile(const std::string &mod__FileTypes,const std::filesystem::path&path)
{
    ModFileTypes modFileTypes(mod__FileTypes);

    auto fileProperty =
        std::make_shared<UiFileProperty>(
            "Test", "urn:test", modFileTypes);

    auto fileModDirectory = fileProperty->getParentModDirectory(path);
    return fileProperty->IsValidExtension(path);
}

TEST_CASE("ModFileTypes Test", "[modFileTypes]")
{

    REQUIRE(checkFile("audio","shared/audio/Test.wav"));
    REQUIRE(checkFile("audio","shared/audio/xxx/Test.flac"));
    REQUIRE(!checkFile("audio","shared/audio/xxx/Test.nam"));

    REQUIRE(checkFile("audio,wav","shared/audio/Test.wav"));
    REQUIRE(!checkFile("audio,wav","shared/audio/xxx/Test.flac"));
    REQUIRE(!checkFile("audio","shared/audio/xxx/Test.nam"));

    REQUIRE(checkFile("cabsim,wav","CabIR/Test.wav"));
    REQUIRE(!checkFile("cabsim,wav","CabIR/xxx/Test.flac"));
    REQUIRE(!checkFile("cabsim","CabIR/xxx/Test.nam"));



    REQUIRE(checkFile("audio,wav","shared/audio/Test.wav"));
    REQUIRE(!checkFile("audio,wav","shared/audio/xxx/Test.flac"));
    REQUIRE(!checkFile("audio","shared/audio/xxx/Test.nam"));


    REQUIRE(checkFile("wav","shared/audio/Test.wav"));
    REQUIRE(!checkFile("wav","CabIR/Test.flac")); // only one audio directory for this case.
    REQUIRE(!checkFile("wav","shared/audio/xxx/Test.nam"));
    REQUIRE(!checkFile("wav","shared/midiSongs/xxx/Test.mid"));
    REQUIRE(!checkFile("wav","shared/other/xxx/Test.mid"));


    REQUIRE(checkFile("mid","shared/midiClips/Test.mid"));
    REQUIRE(checkFile("mid","shared/midiSongs/Test.mid"));
    REQUIRE(!checkFile("mid","CabIR/Test.flac")); // only one audio directory for this case.


    REQUIRE(checkFile("midisong","shared/midiSongs/Test.mid"));
    REQUIRE(checkFile("midisong","shared/midiSongs/xxx/Test.midi"));
    REQUIRE(!checkFile("midisong","shared/midiClips/xxx/Test.nam"));


    REQUIRE(checkFile("","shared/audio/Test.wav"));
    REQUIRE(checkFile("","shared/audio/xxx/Test.flac"));
    REQUIRE(!checkFile("","shared/audio/xxx/Test.nam"));
    REQUIRE(!checkFile("","ToobMlModels/Test.nam"));
    REQUIRE(checkFile("","ToobMlModels/Test.json"));
    REQUIRE(checkFile("","ToobMlModels/a/b/c/Test.json"));
    REQUIRE(checkFile("","shared/other/xxx/Test.xwav"));
    REQUIRE(checkFile("","shared/other/xxx/Test"));




    {
        std::set<std::string> result =
            CalculateFileExtensions("audio", "audio");

        REQUIRE(result.contains(".wav"));
        REQUIRE(result.contains(".flac"));

    }

    {
        std::set<std::string> result =
            CalculateFileExtensions("wav", "audio");

        REQUIRE(result.size() == 1);
        REQUIRE(result.contains(".wav"));
        REQUIRE(!result.contains(".flac"));
    }
    {
        std::set<std::string> result =
            CalculateFileExtensions("nam", "audio");

        REQUIRE(result.size() == 0);
        REQUIRE(!result.contains(".flac"));
        REQUIRE(!result.contains(".nam"));
    }
    {
        // extensions specified, but no modDirectory.
        ModFileTypes modFileTypes("mid,midi");

        auto fileProperty =
            std::make_shared<UiFileProperty>(
                "Test", "urn:test", modFileTypes);

        REQUIRE(fileProperty->modDirectories().size() == 2);
        for (const auto&modDirectory: fileProperty->modDirectories())
        {
            auto fileExtensions = fileProperty->GetPermittedFileExtensions(modDirectory);
            REQUIRE(fileExtensions.size() == 2);
            REQUIRE(fileExtensions.contains(".mid"));
            REQUIRE(fileExtensions.contains(".midi"));
        }
    }
    {
        // Custom directory, custom extensions.
        // actually simulating a PiPedalUI::UiFileProperty load from ttl.
        ModFileTypes modFileTypes("wavx");

        modFileTypes.rootDirectories().push_back("CustomDirectory"); 

        auto fileProperty =
            std::make_shared<UiFileProperty>(
                "Test", "urn:test", modFileTypes);

        REQUIRE(fileProperty->modDirectories().size() == 1);
        auto fileExtensions = fileProperty->GetPermittedFileExtensions("CustomDirectory");

        REQUIRE(fileProperty->modDirectories().size() == 1);
        REQUIRE(fileExtensions.size() == 1);
        REQUIRE(fileExtensions.contains(".wavx"));
        
    }
}
