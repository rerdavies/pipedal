// Copyright (c) 2023 Robin Davies
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
#include <map>
#include <set>

namespace pipedal {
    class MimeTypes {
    private:
        MimeTypes();
    public:
        static const MimeTypes&instance();

        const std::string& MimeTypeFromExtension(const std::string &extension) const;
        // the best possible extension if we had to pick one.
        const std::string& ExtensionFromMimeType(const std::string &mimeType) const;
        // All possible extensions that other people might have picked.
        const std::set<std::string> &AudioExtensions() const;
        const std::set<std::string> &VideoExtensions() const;
        const std::set<std::string> &MidiExtensions() const;
        const bool IsValidExtension(const std::string&mimeType, const std::string&extension) const;
    private:
        void AddMimeType(const std::string&extension, const std::string&mimeType);
        std::map<std::string,std::set<std::string>> mimeTypeToExtensions;
        std::map<std::string,std::string> mimeTypeToExtension;
        std::map<std::string,std::string> extensionToMimeType;
        std::set<std::string> audioExtensions;
        std::set<std::string> midiExtensions;
        std::set<std::string> videoExtensions;
    };
}