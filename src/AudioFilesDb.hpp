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
#include "SQLiteCpp/SQLiteCpp.h"
#include "AudioFileMetadata.hpp"
#include <cstdint>
#include <vector>


namespace pipedal::impl {

    class DatabaseLock;

    class DbFileInfo : public AudioFileMetadata
    {
    private:
        int64_t idFile_ = -1;
        bool present_ = false;
        bool dirty_ = false;
    public:
        int64_t idFile() const { return idFile_;}
        void idFile(int64_t value) { idFile_ = value;}
        bool present() const { return present_; }
        void present(bool value) { present_ = value;}
        bool dirty() const { return dirty_;}
        void dirty(bool value) { dirty_ = value;}

    };


    class ThumbnailInfo {
    public:
        ThumbnailInfo() = default;
    private:
        ThumbnailType thumbnailType_ = ThumbnailType::Unknown;
        std::string thumbnailFile_;
        int64_t thumbnailLastModified_ = 0;
    public:
        ThumbnailType thumbnailType() const { return thumbnailType_; }
        void thumbnailType(ThumbnailType value) { thumbnailType_ = value; }
        const std::string &thumbnailFile() const { return thumbnailFile_; }
        void thumbnailFile(const std::string &value) { thumbnailFile_ = value; }
        int64_t thumbnailLastModified() const { return thumbnailLastModified_; }
        void thumbnailLastModified(int64_t value) { thumbnailLastModified_ = value; }
    };
    class AudioFilesDb {
    public:
        static constexpr int32_t DB_VERSION = 1;
        AudioFilesDb(
            const std::filesystem::path &path,
            const std::filesystem::path &indexPath = "" // if non-empty, forces the location of the ".index.pipedal" file.
        );
    public:
        void DeleteFile(DbFileInfo *dbFile);
        void WriteFile(DbFileInfo *dbFile);

        // test only.
        size_t GetNumberOfThumbnails();
        void UpdateThumbnailInfo(
            int64_t idFile, 
            ThumbnailType thumbnailType, 
            const std::string &thumbnailFile = "", 
            int64_t thumbnailLastModified = 0);
        void UpdateThumbnailInfo(
            const std::string&fileName, 
            ThumbnailType thumbnailType, 
            const std::string &thumbnailFile = "", 
            int64_t thumbnailLastModified = 0);
        std::unique_ptr<SQLite::Transaction> transaction();
        std::vector<DbFileInfo> QueryTracks();
        void DeleteThumbnails(id_t idFile);
        ThumbnailInfo GetThumbnailInfo(const std::string &fileNameOnly);
        std::vector<uint8_t> GetEmbeddedThumbnail(const std::string &fileNameOnly, int32_t width, int32_t height);
        std::vector<uint8_t> GetEmbeddedThumbnail(int64_t idFile, int32_t width, int32_t height);
        void AddThumbnail(
            const std::string &fileName,
            int64_t width,
            int64_t height,
            const std::vector<uint8_t> &thumbnailData);
        void AddThumbnail(
            int64_t idFile,
            int64_t width,
            int64_t height,
            const std::vector<uint8_t> &thumbnailData);
        void UpdateFilePosition(
            int64_t idFile,
            int32_t position);

    private:

        void CreateDb(const std::filesystem::path &dbPathName);
        void UpgradeDb();
        int QueryVersion();

    private:
        std::filesystem::path indexPath;

        // warning: destructor order matters here!
        std::shared_ptr<DatabaseLock> dbLock; // mutex to protect the index file.
        std::unique_ptr<SQLite::Database> db;
        std::unique_ptr<SQLite::Statement> insertFileQuery;
        std::unique_ptr<SQLite::Statement> updateFileQuery;
        std::unique_ptr<SQLite::Statement> deleteFileQuery;
        std::unique_ptr<SQLite::Statement> updateThumbnailInfoQueryByName;
        std::unique_ptr<SQLite::Statement> updateThumbnailInfoQueryById;
        std::unique_ptr<SQLite::Statement> updatePositionQuery;
        std::filesystem::path path;
    };
}

