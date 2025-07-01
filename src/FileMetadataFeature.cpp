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
#include "FileMetadataFeature.hpp"
#include "AudioFiles.hpp"
#include "Lv2Log.hpp"
#include "ss.hpp"
#include "util.hpp"
#include "json.hpp"
#include "json_variant.hpp"

#include <iostream>
#include <filesystem>

#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
using namespace pipedal;
namespace fs = std::filesystem;

FileMetadataFeature::FileMetadataFeature()
{
    feature.URI = PIPEDAL__FILE_METADATA_FEATURE;
    feature.data = &interface;
    interface.handle = (void *)this;
    interface.setFileMetadata = &FileMetadataFeature::S_setFileMetadata;
    interface.getFileMetadata = &FileMetadataFeature::S_getFileMetadata;
    interface.deleteFileMetadata = &FileMetadataFeature::S_deleteFileMetadata;
}

FileMetadataFeature::~FileMetadataFeature()
{
}

void FileMetadataFeature::SetPluginStoragePath(const std::filesystem::path &storagePath)
{
    this->tracksPath = storagePath / "shared" / "audio" / "Tracks";
}

// Check if the path is within the tracks directory
bool FileMetadataFeature::IsValidPath(const std::filesystem::path &path) const
{
    if (tracksPath.empty())
    {
        return false;
    }
    if (HasDotDot(path))
    {
        return false;
    }
    // is a subdirectory of track path
    // check for directory seperator
    if (!IsSubdirectory(path, tracksPath))
    {
        return false; // Valid subdirectory
    }
    return true;
}
void FileMetadataFeature::Prepare(MapFeature &map)
{
    this->mapFeature = &map;
}

PIPEDAL_FileMetadata_Status FileMetadataFeature::setFileMetadata(
    const char *absolute_path,
    const char *key,
    const char *value)
{
    if (!IsValidPath(absolute_path))
    {
        return PIPEDAL_FILE_METADATA_INVALID_PATH; // Cannot set metadata for files in the tracks directory
    }

    std::filesystem::path path{SS(absolute_path << ".mdata")};

    json_variant jsonData;
    if (fs::exists(path))
    {
        try
        {
            std::ifstream f(path);
            json_reader reader(f);
            reader.read(&jsonData);
        }
        catch (const std::exception &e)
        {
            jsonData = json_variant::make_object();
        }
    }
    else
    {
        jsonData = json_variant::make_object();
    }

    (*jsonData.as_object())[key] = json_variant(value);

    {
        std::ofstream f(path);
        if (!f.is_open())
        {
            Lv2Log::error(SS("Failed to write metadata file " << path));
            return PIPEDAL_FILE_METADATA_PERMISSION_DENIED; // Failed to open existing metadata
        }
        json_writer writer(f);
        writer.write(jsonData);
    }
    Lv2Log::debug(SS("Set file metadata for " << absolute_path << " key: " << key << " value: " << value));
    return PIPEDAL_FILE_METADATA_SUCCESS;
}

uint32_t FileMetadataFeature::getFileMetadata(
    const char *absolute_path,
    const char *key,
    char *fileMetadata,
    uint32_t fileMetadataSize)
{
    if (fileMetadata)
        fileMetadata[0] = '\0'; // Ensure the buffer is null-terminated

    if (!IsValidPath(absolute_path))
    {
        return 0; // Cannot set metadata for files in the tracks directory
    }

    std::filesystem::path path{SS(absolute_path << ".mdata")};

    json_variant jsonData;
    std::string result;

    try
    {
        if (fs::exists(path))
        {
            std::ifstream f(path);
            json_reader reader(f);
            reader.read(&jsonData);
            result = jsonData.as_object()->at(std::string(key)).as_string();
        }
        else
        {
            Lv2Log::debug(SS("No metadata file found for " << absolute_path << " key: " << key));
            return 0;
        }
    }
    catch (const std::exception &e)
    {
            Lv2Log::debug(SS("Permission denied for " << absolute_path << " key: " << key));
        return 0;
    }

    size_t len = result.length() + 1; // +1 for null terminator
    if (len > fileMetadataSize || fileMetadata == nullptr)
    {
        // Not enough space in the buffer, return the size needed.
        return len;
    }
    memcpy(fileMetadata, result.c_str(), len);
    Lv2Log::debug(SS("Get file metadata for " << absolute_path << " key: " << key << " value: " << result));
    return len;
}
PIPEDAL_FileMetadata_Status FileMetadataFeature::deleteFileMetadata(
    const char *absolute_path,
    const char *key)
{
    if (!IsValidPath(absolute_path))
    {
        return PIPEDAL_FILE_METADATA_INVALID_PATH; // Cannot set metadata for files in the tracks directory
    }
    Lv2Log::debug(SS("Delete file metadata for " << absolute_path << " key: " << key));

    try {
        std::filesystem::path path{SS(absolute_path << ".mdata")};

        if (!fs::exists(path))
        {
            return PIPEDAL_FILE_METADATA_NOT_FOUND; // Metadata file does not exist
        }

        json_variant jsonData;
        try
        {
            std::ifstream f(path);
            json_reader reader(f);
            reader.read(&jsonData);
        }
        catch (const std::exception &e)
        {
            return PIPEDAL_FILE_METADATA_NOT_FOUND; // Failed to read existing metadata
        }

        auto obj = jsonData.as_object();
        auto it = obj->find(std::string(key));
        if (it == obj->end())
        {
            return PIPEDAL_FILE_METADATA_NOT_FOUND; // Key not found
        }

        obj->erase(it);

        if (obj->begin() == obj->end())
        {
            // If the object is empty, delete the metadata file
            fs::remove(path);
            return PIPEDAL_FILE_METADATA_SUCCESS; // Metadata deleted successfully
        }

        std::ofstream f(path);
        if (!f.is_open())
        {
            Lv2Log::error(SS("Failed to write metadata file " << path));
            return PIPEDAL_FILE_METADATA_PERMISSION_DENIED; // Failed to open existing metadata
        }

        json_writer writer(f);
        writer.write(jsonData);

        return PIPEDAL_FILE_METADATA_SUCCESS;
    } catch (const std::exception &e) {
        Lv2Log::error(SS("Exception while deleting metadata: " << e.what()));
        return PIPEDAL_FILE_METADATA_PERMISSION_DENIED; // Failed to open existing metadata
    }
}

PIPEDAL_FileMetadata_Status FileMetadataFeature::S_setFileMetadata(
    PIPEDAL_FILE_METADATA_Handle handle,
    const char *absolute_path,
    const char *key,
    const char *fileMetadata)
{
    return ((FileMetadataFeature *)handle)->setFileMetadata(absolute_path, key, fileMetadata);
}
uint32_t FileMetadataFeature::S_getFileMetadata(
    PIPEDAL_FILE_METADATA_Handle handle,
    const char *absolute_path,
    const char *key,
    char *buffer,
    uint32_t bufferSize)
{
    return ((FileMetadataFeature *)handle)->getFileMetadata(absolute_path, key, buffer, bufferSize);
}

PIPEDAL_FileMetadata_Status FileMetadataFeature::S_deleteFileMetadata(
    PIPEDAL_FILE_METADATA_Handle handle,
    const char *absolute_path,
    const char *key)
{
    return ((FileMetadataFeature *)handle)->deleteFileMetadata(absolute_path, key);
}
