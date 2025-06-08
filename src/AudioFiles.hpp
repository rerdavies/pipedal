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
#include <memory>
#include <cstdint>
#include <vector>
#include <filesystem>
#include "AudioFileMetadata.hpp"
#include "TemporaryFile.hpp"

namespace pipedal
{

    class ThumbnailTemporaryFile : public TemporaryFile
    {
    private:
        // Private constructor to enforce the use of CreateTemporaryFile or the other constructor.
        explicit ThumbnailTemporaryFile(const std::filesystem::path &temporaryDirectory);

    public:
        using super = TemporaryFile;
        ThumbnailTemporaryFile() = default;
        ThumbnailTemporaryFile(const ThumbnailTemporaryFile &) = delete;
        ThumbnailTemporaryFile(ThumbnailTemporaryFile &&) = default;
        ThumbnailTemporaryFile &operator=(const ThumbnailTemporaryFile &) = delete;
        ThumbnailTemporaryFile &operator=(ThumbnailTemporaryFile &&) = default;

    public:
        static ThumbnailTemporaryFile CreateTemporaryFile(const std::filesystem::path &tempDirectory, const std::string &mimeType);
        static ThumbnailTemporaryFile FromFile(const std::filesystem::path &filePath, const std::string &mimeType);

    public:
        void SetNonDeletedPath(const std::filesystem::path &path, const std::string &mimeType = "audio/mpeg")
        {
            TemporaryFile::SetNonDeletedPath(path);
            // Set the MIME type for audio files.
            this->mimeType = mimeType; // Default MIME type, can be set later.
        }
        void Attach(const std::filesystem::path &path, const std::string &mimeType);

        std::string GetMimeType()
        {
            return mimeType;
        }
        void SetMimeType(const std::string &mimeType)
        {
            this->mimeType = mimeType;
        }

    private:
        std::string mimeType = "image/jpeg"; // Default MIME type, can be set later.
    };

    class AudioDirectoryInfo
    {
    protected:
        AudioDirectoryInfo() {}
        virtual ~AudioDirectoryInfo() {}

    public:
        using self = AudioDirectoryInfo;
        using Ptr = std::shared_ptr<self>;

        static Ptr Create(
            const std::filesystem::path &path,
            const std::filesystem::path &indexPath = "");

        virtual std::vector<AudioFileMetadata> GetFiles() = 0;

        virtual ThumbnailTemporaryFile GetThumbnail(const std::string &fileNameOnly, int32_t width, int32_t height) = 0;

        virtual std::string GetNextAudioFile(const std::string &fileNameOnly) = 0;
        virtual std::string GetPreviousAudioFile(const std::string &fileNameOnly) = 0;

        virtual void MoveAudioFile(
            const std::string &directory,
            int32_t fromPosition,
            int32_t toPosition) = 0;
        virtual ThumbnailTemporaryFile DefaultThumbnailTemporaryFile() = 0;

        static void SetTemporaryDirectory(const std::filesystem::path &path);
        static void SetResourceDirectory(const std::filesystem::path &path);

        virtual size_t TestGetNumberOfThumbnails() = 0; // test use only.
        virtual void TestSetIndexPath(const std::filesystem::path &path) = 0;

    public:
        static std::filesystem::path GetTemporaryDirectory();
        static std::filesystem::path GetResourceDirectory();

    private:
        static std::filesystem::path temporaryDirectory;
        static std::filesystem::path resourceDirectory;
        ;
    };



    std::filesystem::path IndexPathToShadowIndexPath(const std::filesystem::path &audioRootDirectory, const std::filesystem::path &path);
    // recover the original path fromthe shadow index path.
    std::filesystem::path ShadowIndexPathToIndexPath(const std::filesystem::path &audioRootDirectory,  const std::filesystem::path &path);
    std::filesystem::path GetShadowIndexDirectory(const std::filesystem::path &audioRootDirectory, const std::filesystem::path &path);

}