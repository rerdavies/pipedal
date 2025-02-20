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

#pragma once

#include <string>
#include <filesystem>

namespace pipedal {
    class PiPedalModel;

    class PresetBundleWriter {
    public:
        using self = PresetBundleWriter;
        using ptr = std::unique_ptr<self>;

        virtual ~PresetBundleWriter() noexcept = 0;

        virtual void WriteToFile(const std::filesystem::path&filePath) = 0;


        static ptr CreatePresetsFile(PiPedalModel&model,const std::string&presetJson);
        static ptr CreatePluginPresetsFile(PiPedalModel&model,const std::string&presetJson);
    };

    class PresetBundleReader {
    public:
        using self = PresetBundleReader;
        using ptr = std::unique_ptr<self>;

        virtual ~PresetBundleReader() noexcept = 0;

        virtual void ExtractMediaFiles() = 0;
        virtual std::string GetPresetJson() = 0;
        virtual std::string GetPluginPresetsJson() = 0;
        static ptr LoadPresetsFile(PiPedalModel&model,const std::filesystem::path &path);
        static ptr LoadPluginPresetsFile(PiPedalModel&model,const std::filesystem::path &path);
    };

}
