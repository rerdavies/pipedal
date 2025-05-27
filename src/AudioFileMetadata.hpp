/*
 *   Copyright (c) 2025 Robin E. R. Davies
 *   All rights reserved.

 *   Permission is hereby granted, free of charge, to any person obtaining a copy
 *   of this software and associated documentation files (the "Software"), to deal
 *   in the Software without restriction, including without limitation the rights
 *   to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 *   copies of the Software, and to permit persons to whom the Software is
 *   furnished to do so, subject to the following conditions:

 *   The above copyright notice and this permission notice shall be included in all
 *   copies or substantial portions of the Software.

 *   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 *   IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 *   AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *   OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 *   SOFTWARE.
 */

#pragma once

#include <string>
#include <filesystem>
#include "json.hpp"


namespace pipedal
{

#define GETTER_SETTER_REF(name)                               \
    const decltype(name##_) &name() const { return name##_; } \
    void name(const decltype(name##_) &value) { name##_ = value; }

#define GETTER_SETTER_VEC(name)                   \
    decltype(name##_) &name() { return name##_; } \
    const decltype(name##_) &name() const { return name##_; }

    // void name(decltype(const name##_) &value) { name##_ =  value; }
    // void name(decltype(const name##_) &&value) { name##_ =  std::move(value); }

#define GETTER_SETTER(name)                            \
    decltype(name##_) name() const { return name##_; } \
    void name(decltype(name##_) value) { name##_ = value; }

    class AudioFileMetadata
    {
    private:
        float duration_ = 0;
        std::string title_;
        std::string track_;
        std::string album_;
        std::string disc_;
        std::string artist_;
        std::string albumArtist_;
        std::string totalTracks_;
        std::string date_;
        std::string year_;


    public:
        AudioFileMetadata(const std::filesystem::path &file);

        GETTER_SETTER(duration);
        GETTER_SETTER_REF(title);
        GETTER_SETTER_REF(track);
        GETTER_SETTER_REF(album);
        GETTER_SETTER_REF(disc);
        GETTER_SETTER_REF(artist);
        GETTER_SETTER_REF(albumArtist);

    public:
        DECLARE_JSON_MAP(AudioFileMetadata);

    };

    std::string GetAudioFileMetadataString(const std::filesystem::path &path);
    
#undef GETTER_SETTER
#undef GETTER_SETTER_VEC
#undef GETTER_SETTER_REF

}
