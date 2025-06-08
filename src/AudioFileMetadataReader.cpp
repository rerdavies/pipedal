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

#include "AudioFileMetadataReader.hpp"
#include <json.hpp>
#include <chrono>
#include <filesystem>
#include <json_variant.hpp>
#include "LRUCache.hpp"
#include <sstream>
#include <stdexcept>
#include "TemporaryFile.hpp"

using namespace pipedal;
namespace fs = std::filesystem;
namespace chrono = std::chrono;

// Safely encode a filename for use in a shell command
std::string shell_escape_filename(const std::string &filename)
{
    std::string escaped;
    escaped.reserve(filename.size() + 2); // Reserve space for quotes and content

    // Wrap the filename in single quotes
    escaped += '\'';

    for (char c : filename)
    {
        // Escape single quotes by closing the quote, adding escaped quote, and reopening
        if (c == '\'')
        {
            escaped += "'\\''";
        }
        else
        {
            escaped += c;
        }
    }

    escaped += '\'';
    return escaped;
}

static std::string readPipeToString(FILE *file)
{
    std::string content;
    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0)
    {
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

    std::string command = ss.str();
    FILE *output = popen(command.c_str(), "r");
    if (output == nullptr)
    {
        throw std::runtime_error("Failed to process file.");
    }
    std::string result;
    try
    {
        result = readPipeToString(output);
    }
    catch (const std::exception &e)
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

static float MetadataFloat(const json_variant &vt, float defaultValue = 0)
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

static std::string MetadataString(const json_variant &o, const std::vector<std::string> &names)
{
    for (const auto &name : names)
    {
        auto result = (*o.as_object())[name];
        if (result.is_string())
        {
            return result.as_string();
        }
    }
    return "";
}

static int32_t MetadataTrackToInt(const std::string &track, const std::string &disc)
{
    if (track.empty())
    {
        return -1;
    }
    try
    {
        int trackNumber = std::stoi(track);
        if (!disc.empty())
        {
            if (disc != "" && disc != "1/1" && disc != "1")
            {
                int discNumber = std::stoi(disc);
                trackNumber += discNumber*1000+ trackNumber; // e.g. disc 1 track 2 becomes 1002
            }

        }
        return trackNumber;
    }
    catch (const std::exception &)
    {
        return -1; // Invalid track or disc number
    }
}

AudioFileMetadata::AudioFileMetadata(const std::filesystem::path &file)
{
    try
    {
        const std::string json = GetJsonMetadata(file);

        json_variant vt;

        std::stringstream ss(json);
        json_reader reader(ss);

        reader.read(&vt);

        auto top = vt.as_object();
        auto format = top->at("format").as_object();

        this->duration_ = MetadataFloat(format->at("duration"), 0);

        auto tags = (*format)["tags"];
        if (tags.is_object())
        {
            // not all of this data gets used (e.g. DATE, YEAR, ALBUM_ARTIST,TOTALTRACKS). But ... write once.
            this->album_ = MetadataString(tags, {"ALBUM", "album"});
            this->artist_ = MetadataString(tags, {"ARTIST", "artist"});
            this->albumArtist_ = MetadataString(tags, {"ALBUM ARTIST", "album_artist", "album artist"});
            this->title_ = MetadataString(tags, {"TITLE", "title"});
            //this->date_ = MetadataString(tags, {"DATE", "date"});
            //this->year_ = MetadataString(tags, {"YEAR", "year"});
            this->track_ =
                MetadataTrackToInt(
                    MetadataString(tags, {"track","TRACK"}),
                    MetadataString(tags, {"disc", "DISC"})
                );
            //this->totalTracks_ = MetadataString(tags, {"TOTALTRACKS"});

            if (title_ == "")
            {
                this->title_ = file.stem();
            }
        }
    }
    catch (const std::exception &e)
    {
        std::string title = file.stem();
        this->title_ = title;
    }
}

namespace
{
    class AudioCacheKey
    {
    public:
        AudioCacheKey(const std::filesystem::path &path)
        {
            this->path = path.string();
            this->lastWrite = std::filesystem::last_write_time(path);
        }
        // Equality operator
        bool operator==(const AudioCacheKey &other) const
        {
            return path == other.path && lastWrite == other.lastWrite;
        }

    public:
        std::string path;
        fs::file_time_type lastWrite;
    };
}

namespace std
{
    template <>
    struct hash<AudioCacheKey>
    {

        size_t operator()(const AudioCacheKey &key) const
        {
            size_t path_hash = hash<std::string>{}(key.path);
            size_t time_hash = hash<uintmax_t>{}(
                duration_cast<std::chrono::nanoseconds>(key.lastWrite.time_since_epoch()).count());
            // Combine hashes (boost::hash_combine approach)
            return path_hash ^ (time_hash + 0x9e3779b9 + (path_hash << 6) + (path_hash >> 2));
        }
    };
}

static LRUCache<AudioCacheKey, std::string> metadataCache{500};
static std::mutex metadataCacheMutex;

std::string pipedal::GetAudioFileMetadataString(const std::filesystem::path &path)
{
    std::lock_guard lock{metadataCacheMutex};
    AudioCacheKey key{path};
    std::string result;
    if (metadataCache.get(key, result))
    {
        return result;
    }
    auto metadata = AudioFileMetadata(path);

    std::stringstream ss;
    json_writer writer(ss);
    writer.write(metadata);
    result = ss.str();

    metadataCache.put(key, result);
    return result;
}

TemporaryFile pipedal::GetAudioFileThumbnail(const std::filesystem::path &path, const std::filesystem::path &outputPath)
{
    return GetAudioFileThumbnail(path, 0, 0, outputPath);
}

std::mutex thumbnailMutex;

TemporaryFile pipedal::GetAudioFileThumbnail(const std::filesystem::path &path, int32_t width, int32_t height, const std::filesystem::path &tempDirectory)
{
    std::lock_guard<std::mutex> lock(thumbnailMutex);
    fs::path thumbnailDirectory = tempDirectory / "thumbnails";
    if (!fs::exists(thumbnailDirectory))
    {
        fs::create_directories(thumbnailDirectory);
    } else {
        // Clean up old thumbnails
        for (const auto &entry : fs::directory_iterator(thumbnailDirectory))
        {
            if (entry.is_regular_file()) 
            {
                fs::remove(entry.path());
            }
        }
    }
    fs::create_directories(tempDirectory);

    TemporaryFile tempFile(tempDirectory);
    std::filesystem::path outputPath = thumbnailDirectory / "thumbnail-%03d.jpg";
    // ffmpeg -loglevel error -i "test2.mp3"  -vf scale=200:200  -frames:v 1 thumb-%03.jpg -y

    std::stringstream ss;
    ss << "/usr/bin/ffmpeg -loglevel error -i " << shell_escape_filename(path.string());
    if (width > 0 && height > 0)
    {
        ss << " -vf \"scale=" << width << ":" << height << 
            ":force_original_aspect_ratio=increase,crop=" << width << ":" << height << "\"";
    } else {
        // -vf "crop=min(iw\,ih):min(iw\,ih)"
        ss << " -vf \"crop=min(iw\\,ih):min(iw\\,ih)\"";
    }
    ss << " " << shell_escape_filename(outputPath.string());
    ss  << " -y 2>/dev/null 1>/dev/null";
    std::string command = ss.str();
    int rc = system(command.c_str());
    if (rc < 0) {
        throw std::runtime_error("Failed to execute ffmpeg command: " + command);
    }
    int exitCode = WEXITSTATUS(rc);
    if (exitCode != EXIT_SUCCESS)
    {        throw std::runtime_error("Thumbnail not foud.");
    }
    fs::remove(tempFile.Path());
    if (fs::exists(thumbnailDirectory / "thumbnail-001.jpg"))
    {
        // Move the thumbnail to the temporary file.
        fs::rename(thumbnailDirectory / "thumbnail-001.jpg", tempFile.Path());
    }
    else
    {
        throw std::runtime_error("Failed to create thumbnail.");
    }
    return tempFile;
}

