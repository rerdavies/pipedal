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

#include "lv2/lv2plug.in/ns/ext/buf-size/buf-size.h"
using namespace pipedal;

FileMetadataFeature::FileMetadataFeature()
{
    feature.URI = PIPEDAL__FILE_METADATA_FEATURE;
    feature.data = &interface;
    interface.handle = (void *)this;
    interface.setFileMetadata = &FileMetadataFeature::S_setFileMetadata;
    interface.getFileMetadata = &FileMetadataFeature::S_getFileMetadata;

}

FileMetadataFeature::~FileMetadataFeature()
{
}
void FileMetadataFeature::Prepare(MapFeature &map)
{
    this->mapFeature = &map;
}

PIPEDAL_FileMetadata_Status FileMetadataFeature::setFileMetadata(
    const char *absolute_path,
    LV2_URID key,
    const char *fileMetadata)
{
    const char *strKey = mapFeature->UridToString(key);
    if (strKey == nullptr)
    {
        return PIPEDAL_FILE_METADATA_INVALID_KEY;
    }
    std::filesystem::path path{absolute_path};

    if (!std::filesystem::exists(path))
    {
        return PIPEDAL_FILE_METADATA_INVALID_PATH; // File does not exist
    }
    if (!std::filesystem::is_regular_file(path))
    {
        return PIPEDAL_FILE_METADATA_INVALID_PATH; // Not a regular file
    }
    try {
        AudioDirectoryInfo::Ptr audioDirectoryInfo = AudioDirectoryInfo::Create(path.parent_path());
        audioDirectoryInfo->SetFileMetadata(path.filename(), strKey, fileMetadata);
    } catch (const std::exception&e) {
        Lv2Log::error(SS("Failed to create AudioDirectoryInfo for " << path << ": " << e.what()));
        return PIPEDAL_FILE_METADATA_INVALID_PATH; // Failed to create AudioDirectoryInfo
    }


    return PIPEDAL_FILE_METADATA_SUCCESS; // TODO: Implement this
}

uint32_t FileMetadataFeature::getFileMetadata(
    const char *absolute_path,
    LV2_URID key,
    char *fileMetadata,
    uint32_t fileMetadataSize)
{
    const char *strKey = mapFeature->UridToString(key);
    if (strKey == nullptr)
    {
        return PIPEDAL_FILE_METADATA_INVALID_KEY;
    }

    std::filesystem::path path{absolute_path};

    AudioDirectoryInfo::Ptr audioDirectoryInfo = AudioDirectoryInfo::Create(path.parent_path());

    std::string metadata = audioDirectoryInfo->GetFileMetadata(path.filename(), strKey);
    if (metadata.empty())
    {
        return 0; // No metadata found
    }
    size_t required = metadata.size() + 1;
    if (required >= std::numeric_limits<uint32_t>::max())
    {
        return PIPEDAL_FILE_METADATA_ERR_UNKNOWNM; // Metadata too large
    }
    if (fileMetadata == nullptr || required > fileMetadataSize)
    {
        return static_cast<uint32_t>(required); // Return size needed including null terminator
    }
    if (fileMetadataSize < metadata.length() + 1)
    {
        return static_cast<uint32_t>(metadata.size() + 1); // Return size needed including null terminator
    }
    std::strncpy(fileMetadata, metadata.c_str(), fileMetadataSize);
    return required;
}

PIPEDAL_FileMetadata_Status FileMetadataFeature::S_setFileMetadata(
    PIPEDAL_FILE_METADATA_Handle handle,
    const char *absolute_path,
    LV2_URID key,
    const char *fileMetadata)
{
    return ((FileMetadataFeature *)handle)->setFileMetadata(absolute_path, key, fileMetadata);
}
uint32_t FileMetadataFeature::S_getFileMetadata(
    PIPEDAL_FILE_METADATA_Handle handle, 
    LV2_URID key, 
    const char *absolute_path, 
    char *buffer, 
    uint32_t bufferSize)
{
    return ((FileMetadataFeature *)handle)->getFileMetadata(absolute_path, key, buffer, bufferSize);
}
