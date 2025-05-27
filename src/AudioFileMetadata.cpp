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

#include "AudioFileMetadata.hpp"
#include <json.hpp>
#include <chrono>
#include <filesystem>
#include <json_variant.hpp>
#include "LRUCache.hpp"
#include <sstream>

using namespace pipedal;
namespace fs = std::filesystem;
namespace chrono = std::chrono;


// Safely encode a filename for use in a shell command
static std::string shell_escape_filename(const std::string& filename) {
    std::string escaped;
    escaped.reserve(filename.size() + 2); // Reserve space for quotes and content

    // Wrap the filename in single quotes
    escaped += '\'';
    
    for (char c : filename) {
        // Escape single quotes by closing the quote, adding escaped quote, and reopening
        if (c == '\'') {
            escaped += "'\\''";
        } else {
            escaped += c;
        }
    }
    
    escaped += '\'';
    return escaped;
}


static std::string readPipeToString(FILE* file) {
    std::string content;
    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        content.append(buffer, bytesRead);
    }
    return content;
}


static std::string GetJsonMetadata(const std::filesystem::path &path)
{
    /* ffprobe -loglevel error -show_streams -show_format -print_format stream_tags -of json 
           ~/filename.flac */

    std::stringstream ss;
    ss << "/usr/bin/ffprobe -loglevel error -show_streams -show_format -print_format stream_tags -of json  "
           << shell_escape_filename(path.string())
           << " 2>/dev/null";

    FILE *output = popen(ss.str().c_str(),"r");
    if(output == nullptr) 
    {
        throw std::runtime_error("Failed to process file.");
    }
    std::string result;
    try {
        result = readPipeToString(output);
    } catch (const std::exception&e)
    {
        pclose(output);
        throw std::runtime_error(e.what());
    }
    int retcode = pclose(output);
    if (retcode != 0)
    {
        throw std::runtime_error(result);
    }
    return result;
}

static float MetadataFloat(const json_variant&vt, float defaultValue = 0) 
{
    if (vt.is_string()) 
    {
        std::string s = vt.as_string();
        std::stringstream ss(s);
        float result = defaultValue;
        ss >> result;
        return result;
    }
    return defaultValue;
}

static std::string MetadataString(const json_variant&o,const std::vector<std::string>& names) {
    for (const auto&name: names)
    {
        auto result = (*o.as_object())[name];
        if (result.is_string()) 
        {
            return result.as_string();
        }
    }
    return "";
}

AudioFileMetadata::AudioFileMetadata(const std::filesystem::path &file)
{
    try {
        const std::string json = GetJsonMetadata(file);

        json_variant vt; 

        std::stringstream ss(json);
        json_reader reader(ss);

        reader.read(&vt);

        auto top = vt.as_object();
        auto format = top->at("format").as_object();
        
        this->duration_ = MetadataFloat(format->at("duration"),0);

        auto tags = (*format)["tags"];
        if (tags.is_object())
        {
            this->album_ = MetadataString(tags,{"ALBUM","album"});
            this->artist_ = MetadataString(tags,{"ARTIST","artist"});
            this->albumArtist_ = MetadataString(tags,{"ALBUM ARTIST","album_artist","album artist"});
            this->title_ = MetadataString(tags,{"TITLE","title"});
            this->date_ = MetadataString(tags,{"DATE","date"});
            this->year_ = MetadataString(tags,{"YEAR","year"});
            this->track_ = MetadataString(tags,{"track","TRACK",});
            this->disc_ = MetadataString(tags,{"disc","DISC"});

            if (title_ == "") {
                this->title_ = file.stem();
            }
            this->totalTracks_ = MetadataString(tags,{"TOTALTRACKS"});
        }
    } catch (const std::exception &e)
    {
        std::string title = file.stem();
        this->title_ = title;
    }
}

namespace {
    class AudioCacheKey {
    public:
        AudioCacheKey(const std::filesystem::path &path) {
            this->path = path.string();
            this->lastWrite = std::filesystem::last_write_time(path);

        }
        // Equality operator 
        bool operator==(const AudioCacheKey& other) const {
            return path == other.path && lastWrite == other.lastWrite;
        }

    public:
        std::string path;
        fs::file_time_type lastWrite;
    };
}

namespace std {
    template <>
    struct hash<AudioCacheKey> {

        
        size_t operator()(const AudioCacheKey& key) const {
            size_t path_hash = hash<std::string>{}(key.path);
            size_t time_hash = hash<uintmax_t>{}(
                duration_cast<std::chrono::nanoseconds>(key.lastWrite.time_since_epoch()).count()
            );
            // Combine hashes (boost::hash_combine approach)
            return path_hash ^ (time_hash + 0x9e3779b9 + (path_hash << 6) + (path_hash >> 2));
        }

       
    };
}


static LRUCache<AudioCacheKey,std::string> metadataCache {500};
static std::mutex metadataCacheMutex;

std::string pipedal::GetAudioFileMetadataString(const std::filesystem::path &path)
{
    std::lock_guard lock { metadataCacheMutex};
    AudioCacheKey key { path};
    std::string result;
    if (metadataCache.get(key,result)) 
    {
        return result;
    }
    auto metadata = AudioFileMetadata(path);

    std::stringstream ss;
    json_writer writer(ss);
    writer.write(metadata);
    result = ss.str();

    metadataCache.put(key,result);
    return result;
}


JSON_MAP_BEGIN(AudioFileMetadata)
JSON_MAP_REFERENCE(AudioFileMetadata, duration)
JSON_MAP_REFERENCE(AudioFileMetadata, title)
JSON_MAP_REFERENCE(AudioFileMetadata, track)
JSON_MAP_REFERENCE(AudioFileMetadata, album)
JSON_MAP_REFERENCE(AudioFileMetadata, disc)
JSON_MAP_REFERENCE(AudioFileMetadata, artist)
JSON_MAP_REFERENCE(AudioFileMetadata, albumArtist)
JSON_MAP_REFERENCE(AudioFileMetadata, totalTracks)
JSON_MAP_REFERENCE(AudioFileMetadata, date)
JSON_MAP_REFERENCE(AudioFileMetadata, year)

JSON_MAP_END()
