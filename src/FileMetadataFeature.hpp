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

#pragma once

#include "MapFeature.hpp"
#include "PiPedalUI.hpp"
#include "lv2ext/pipedal.lv2/ext/FileMetadataFeature.h"

namespace pipedal
{

    class FileMetadataFeature
    {

    private:
        LV2_Feature feature;
        PIPEDAL_FileMetadata_Interface interface;
        static PIPEDAL_FileMetadata_Status S_setFileMetadata(
            PIPEDAL_FILE_METADATA_Handle handle, 
            const char *absolute_path, 
            const char*key, 
            const char *fileMetadata);

        PIPEDAL_FileMetadata_Status setFileMetadata(
            const char *absolute_path, 
            const char*key, 
            const char *fileMetadata);

        static uint32_t S_getFileMetadata(
            PIPEDAL_FILE_METADATA_Handle handle,
            const char *filePath,
            const char*key,
            char *buffer,
            uint32_t bufferSize);

        uint32_t getFileMetadata(
            const char *absolute_path,
            const char*key,
            char *fileMetadata,
            uint32_t fileMetadataSize);

        static PIPEDAL_FileMetadata_Status S_deleteFileMetadata(
            PIPEDAL_FILE_METADATA_Handle handle, 
            const char *absolute_path, 
            const char*key);

        PIPEDAL_FileMetadata_Status deleteFileMetadata(
            const char *absolute_path, 
            const char*key);


        bool IsValidPath(const std::filesystem::path &path) const;
    public:
        FileMetadataFeature();
        void Prepare(MapFeature &map);
        void SetPluginStoragePath(const std::filesystem::path &storagePath);
        ~FileMetadataFeature();

    public:
        std::filesystem::path tracksPath;
        MapFeature *mapFeature = nullptr;
        const LV2_Feature *GetFeature()
        {
            return &feature;
        }
    };

}