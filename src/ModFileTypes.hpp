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
#include <vector>
#include <filesystem>
#include <set>

namespace pipedal
{
    class ModFileTypes
    {
    public:

        ModFileTypes(const std::string &fileTypes);
        const std::vector<std::string> &rootDirectories() const { return rootDirectories_; }
        std::vector<std::string> &rootDirectories()  { return rootDirectories_; }
        const std::vector<std::string> &modFileTypes() const { return modFileTypes_; }
        const std::vector<std::string> &fileTypes() const { return fileTypes_; }

        static constexpr const char *DEFAULT_FILE_TYPES = "audio,nammodel,mlmodel,sf2,sfz,midisong,midiclip,*"; // (everything)

    private:
        std::vector<std::string> rootDirectories_;
        std::vector<std::string> modFileTypes_; // e.g. "audio", "nammodel", etc.
        std::vector<std::string> fileTypes_;

    public:
        class ModDirectory
        {
        public:
            ModDirectory(
                const std::string modType,
                const std::string pipedalPath,
                const std::string displayName,
                const std::vector<std::string> defaultFileExtensions);
            const std::string modType;
            const std::filesystem::path pipedalPath;
            const std::string displayName;
            const std::vector<std::string> fileTypesX; // mixture of .ext and mime types. Use fileExtensions instead.
            const std::set<std::string> fileExtensions;

        };

        static const std::vector<ModDirectory> &ModDirectories();
        static const ModDirectory *GetModDirectory(const std::string &modType);

        static void CreateDefaultDirectories(const std::filesystem::path &rootDirectory);
    };
}