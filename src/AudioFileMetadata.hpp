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

#include <filesystem>
#include <string>
#include <cstdint>
#include <vector>
#include "json.hpp"


namespace pipedal {
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


    enum ThumbnailType
    {
        Unknown = 0,
        Embedded = 1, // embedded in the file
        Folder = 2,   // from a albumArt.jpg or similar
        None = 3      // Use default thumbnail.
    };

    class AudioFileMetadata
    {
    private:
        std::string fileName_;
        int64_t lastModified_ = 0;
        std::string title_;
        int32_t track_ = -1; // track number, 0-based, -1 if not set
        std::string album_;
        std::string artist_;
        std::string albumArtist_;
        float duration_ = 0;
        int32_t thumbnailType_ = 0; // 0 = unknown, 1 = embedded, 3 = folder, 4 = none
        std::string thumbnailFile_; // only when thumbnailType is 3= folder.
        int64_t thumbnailLastModified_ = 0;
        int32_t position_ = -1;


    public:

        AudioFileMetadata() = default;
        AudioFileMetadata(const std::filesystem::path &file);

        GETTER_SETTER(fileName)
        GETTER_SETTER(lastModified)
        GETTER_SETTER_REF(title)
        GETTER_SETTER_REF(track)
        GETTER_SETTER_REF(album)
        GETTER_SETTER_REF(albumArtist)
        GETTER_SETTER_REF(artist)
        GETTER_SETTER(duration)
        ThumbnailType thumbnailType() const { return (ThumbnailType)thumbnailType_;}
        void thumbnailType(ThumbnailType value) { thumbnailType_ = (int32_t)value; }
        GETTER_SETTER(thumbnailFile)
        GETTER_SETTER(thumbnailLastModified)
        GETTER_SETTER(position)


    public:
        DECLARE_JSON_MAP(AudioFileMetadata);

    };

    #undef GETTER_SETTER
    #undef GETTER_SETTER_VEC
    #undef GETTER_SETTER_REF


}