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

#include "AudioFiles.hpp"
#include <filesystem>
#include <limits>
#include <stdexcept>
#include "AudioFileMetadataReader.hpp"
#include "Locale.hpp"
#include "MimeTypes.hpp"
#include <stdexcept>
#include "AudioFilesDb.hpp"
#include "Lv2Log.hpp"
#include "ss.hpp"
#include "util.hpp"

#undef _GLIBCXX_DEBUG // Ensure we are not in debug mode, as this file is not compatible with it.
#include "SQLiteCpp/SQLiteCpp.h"

using namespace pipedal;
using namespace pipedal::impl;
namespace fs = std::filesystem;

static constexpr size_t INVALID_INDEX = std::numeric_limits<size_t>::max();


// All the varous cover art files I found on my personal music collection.
// These are used to find the cover art for a directory. Based on
// scanning a large music collection that have been tagged by various tools
// including iTunes, Windows Media Playe, and a variety of taggers.
//
// Files are listed in order of preference.

static std::vector<std::string> COVER_ART_FILES = {
    "Folder.jpg",  // window media player/explorer.
    "Cover.jpg",   // itunes.
    "folder.jpg",  // Linux audio players plex, kodi, Rythmbox, Amorok, Clementine.
    "cover.jpg",   // Linux audio players plex, kodi, Rythmbox, Amorok, Clementine.
    "Artwork.jpg", // itunes.
    "Front.jpg",
    "front.jpg", // linux.
    "AlbumArt.jpg",
    "albumArt.jpg",
    "Frontcover.jpg",
    "AlbumArtSmall.jpg" // windows media player.
};

bool pipedal::isArtworkFileName(const std::string &fileName)
{
    for (const auto &coverArtFile : COVER_ART_FILES)
    {
        if (fileName == coverArtFile)
        {
            return true;
        }
    }
    return false;
}


void AudioDirectoryInfo::SetTemporaryDirectory(const std::filesystem::path &path)
{
    temporaryDirectory = path;
}
void AudioDirectoryInfo::SetResourceDirectory(const std::filesystem::path &path)
{
    resourceDirectory = path;
}
std::filesystem::path AudioDirectoryInfo::GetTemporaryDirectory()
{
    if (temporaryDirectory.empty())
    {
        return "/tmp/pipedal/audiofiles"; // used during testing.
    }
    return temporaryDirectory;
}
std::filesystem::path AudioDirectoryInfo::GetResourceDirectory()
{
    if (resourceDirectory.empty())
    {
        fs::path path = fs::current_path() / "vite/public/img"; // take from the source directory during testing.
        return "/usr/share/pipedal/audiofiles";                 // used during testing.
    }
    return resourceDirectory;
}

std::filesystem::path AudioDirectoryInfo::temporaryDirectory;
std::filesystem::path AudioDirectoryInfo::resourceDirectory;

namespace
{

    static int64_t fileTimeToInt64(const fs::file_time_type &fileTime)
    {
        std::chrono::system_clock::time_point system_time = std::chrono::clock_cast<std::chrono::system_clock>(fileTime);
        auto result = std::chrono::duration_cast<std::chrono::milliseconds>(system_time.time_since_epoch()).count();
        return result;
    }
    static int64_t GetLastWriteTime(const fs::path &file)
    {
        try
        {
            return fileTimeToInt64(fs::last_write_time(file));
        }
        catch (const std::exception &e)
        {
            return 0; // Return 0 if we cannot get the last write time.
        }
    }

    class AudioDirectoryInfoImpl : public AudioDirectoryInfo
    {
    public:
        AudioDirectoryInfoImpl(const std::filesystem::path &path, const std::filesystem::path &indexPath)
            : path(path), indexPath(
                              (indexPath.empty() ? path : indexPath) / ".index.pipedal")
        {
            if (this->indexPath.parent_path() != path)
            {
                fs::create_directories(this->indexPath.parent_path());
            }
        }
        virtual ~AudioDirectoryInfoImpl() {}

        virtual std::vector<AudioFileMetadata> GetFiles() override;
        virtual ThumbnailTemporaryFile GetThumbnail(const std::string &fileNameOnly, int32_t width, int32_t height) override;

        virtual ThumbnailTemporaryFile DefaultThumbnailTemporaryFile() override;

        virtual size_t TestGetNumberOfThumbnails() override; // test use only.
        virtual void TestSetIndexPath(const std::filesystem::path &path) override
        {
            indexPath = path;
        }
        virtual std::string GetNextAudioFile(const std::string &fileNameOnly) override;
        virtual std::string GetPreviousAudioFile(const std::string &fileNameOnly) override;

        virtual void MoveAudioFile(
            const std::string &directory,
            int32_t fromPosition,
            int32_t toPosition) override;

        virtual void SetFileMetadata(
            const std::string &fileNameOnly,
            const std::string &key,
            const std::string &value) override;

        virtual std::string GetFileMetadata(
            const std::string &fileNameOnly,
            const std::string &key) override;


    private:
        std::vector<DbFileInfo> QueryTracks();

        bool opened = false;
        std::filesystem::path indexPath;
        void OpenAudioDb();
        ThumbnailTemporaryFile GetUnindexedThunbnail(const std::string &fileNameOnly, int32_t width, int32_t height);

        std::shared_ptr<AudioFilesDb> audioFilesDb;
        fs::path path;
        using id_t = int64_t;

        std::vector<DbFileInfo> UpdateDbFiles();
        void DbSetThumbnailType(
            int64_t idFile,
            ThumbnailType thumbnailType,
            const std::string &thumbnailFile = "",
            int64_t thumbnailLastModified = 0);
        void DbSetThumbnailType(
            const std::string &fileNameOnly,
            ThumbnailType thumbnailType,
            const std::string &thumbnailFile = "",
            int64_t thumbnailLastModified = 0);

        void DbDeleteFile(DbFileInfo *dbFile);
        void UpdateMetadata(DbFileInfo *dbFile);
        static constexpr int DB_VERSION = 1;
        std::filesystem::path GetFolderFile() const;
    };
}

AudioDirectoryInfo::Ptr AudioDirectoryInfo::Create(const std::filesystem::path &path, const std::filesystem::path &indexPath)
{
    return std::make_shared<AudioDirectoryInfoImpl>(path, indexPath);
}

std::vector<DbFileInfo> AudioDirectoryInfoImpl::QueryTracks()
{
    if (audioFilesDb)
    {
        return audioFilesDb->QueryTracks();
    }
    return std::vector<DbFileInfo>();
}

std::vector<AudioFileMetadata> AudioDirectoryInfoImpl::GetFiles()
{
    OpenAudioDb();
    std::vector<DbFileInfo> dbFiles = UpdateDbFiles();
    std::vector<AudioFileMetadata> metadataResults;
    for (const auto &dbFile : dbFiles)
    {
        AudioFileMetadata metadata{dbFile}; // slice copy of just the metadata.
        metadataResults.push_back(std::move(metadata));
    }
    return metadataResults;
}

static bool isAudioExtension(const std::string &extension)
{
    return MimeTypes::instance().AudioExtensions().contains(extension);
}

static bool HasPositionInfo(const std::vector<DbFileInfo> &dbFiles)
{
    for (const auto &file : dbFiles)
    {
        if (file.position() != -1)
        {
            return true;
        }
    }
    return false;
}
static int DefaultedSortOrder(int track)
{
    if (track < 0)
    {
        return std::numeric_limits<int>::max();
    }
    return track;
}

static bool isFolderArtwork(const std::string &name)
{
    return name.ends_with(".jpg") || name.ends_with(".png");
}

static void SortDbFiles(std::vector<DbFileInfo> &dbFiles, Collator::ptr &collator)
{
    std::sort(
        dbFiles.begin(), dbFiles.end(),
        [collator](const DbFileInfo &a, const DbFileInfo &b)
        {
            if (isFolderArtwork(a.fileName()) != isFolderArtwork(b.fileName()))
            {
                return isFolderArtwork(a.fileName()) < isFolderArtwork(b.fileName());
            }
            if (a.position() != b.position())
            {
                return DefaultedSortOrder(a.position()) < DefaultedSortOrder(b.position());
            }
            if (a.track() != b.track())
            {
                return DefaultedSortOrder(a.track()) < DefaultedSortOrder(b.track());
            }
            if (a.title() != b.title())
            {
                return collator->Compare(a.title(), b.title()) < 0;
            }
            if (a.album() != b.album())
            {
                return collator->Compare(a.title(), b.title()) < 0;
            }
            return false;
            ;
        });
}

std::vector<DbFileInfo> AudioDirectoryInfoImpl::UpdateDbFiles()
{
    std::shared_ptr<SQLite::Transaction> transaction;
    if (audioFilesDb)
    {
        transaction = audioFilesDb->transaction();
    }
    std::vector<DbFileInfo> dbFiles = QueryTracks();
    std::vector<DbFileInfo> newFiles;

    bool updateRequired = false;
    std::map<std::string, DbFileInfo *> nameToDbRecord;
    for (size_t i = 0; i < dbFiles.size(); ++i)
    {
        auto *pFile = &(dbFiles[i]);
        nameToDbRecord[pFile->fileName()] = pFile;
    }

    for (auto dirEntry : fs::directory_iterator(path))
    {
        if (!dirEntry.is_directory())
        {
            auto path = dirEntry.path();
            std::string name = path.filename();
            std::string extension = path.extension();
            if (isAudioExtension(extension) || isArtworkFileName(name))
            {
                // If the file is an audio file or a cover art file, we need to check it.
                if (name.empty() || name[0] == '.')
                {
                    continue; // skip hidden files.
                }
            }
            else
            {
                continue; // not an audio file or cover art file.
            }
            {
                int64_t lastModified = fileTimeToInt64(dirEntry.last_write_time());

                auto f = nameToDbRecord.find(name);
                if (f != nameToDbRecord.end())
                {
                    DbFileInfo *dbFile = f->second;
                    dbFile->present(true);
                    if (dbFile->lastModified() != lastModified)
                    {
                        if (audioFilesDb)
                        {
                            audioFilesDb->DeleteThumbnails(dbFile->idFile());
                        }
                        dbFile->thumbnailType(ThumbnailType::Unknown);
                        dbFile->thumbnailFile("");
                        dbFile->thumbnailLastModified(0);
                        UpdateMetadata(dbFile);
                        dbFile->dirty(true);
                        updateRequired = true;
                    }
                }
                else
                {
                    DbFileInfo newFile;
                    newFile.fileName(name);
                    newFile.idFile(-1);
                    newFile.dirty(true);
                    newFile.present(true);
                    UpdateMetadata(&newFile);
                    newFiles.push_back(std::move(newFile));
                    updateRequired = true;
                }
            }
        }
    }
    for (auto i = dbFiles.begin(); i != dbFiles.end(); ++i)
    {
        if (!i->present())
        {
            DbDeleteFile(&*i);
            dbFiles.erase(i);
            updateRequired = true;
            --i;
        }
    }
    // refresh folder file names.
    auto folderFile = GetFolderFile();
    int64_t folderLastModified = 0;
    std::string folderFileName;
    if (!folderFile.empty() && fs::exists(folderFile))
    {
        folderLastModified = GetLastWriteTime(folderFile);
        folderFileName = folderFile.filename().string();
    }
    for (auto &dbFile : dbFiles)
    {
        if (dbFile.thumbnailType() == ThumbnailType::None)
        {
            if (!folderFileName.empty())
            {
                dbFile.thumbnailType(ThumbnailType::Folder);
                dbFile.thumbnailFile(folderFileName);
                dbFile.thumbnailLastModified(folderLastModified);
                dbFile.dirty(true);
                updateRequired = true;
            }
        }
        else if (dbFile.thumbnailType() == ThumbnailType::Folder)
        {
            // Check if the folder file has changed.
            if (folderFileName.empty())
            {
                // no more folder file. update accordingly.
                dbFile.thumbnailType(ThumbnailType::None);
                dbFile.thumbnailFile("");
                dbFile.thumbnailLastModified(0);
                dbFile.dirty(true);
                updateRequired = true;
            }
            else
            {
                // folder file has changed. update accordingly.
                if (dbFile.thumbnailFile() != folderFileName ||
                    dbFile.thumbnailLastModified() != folderLastModified)
                {
                    dbFile.thumbnailFile(folderFileName);
                    dbFile.thumbnailLastModified(folderLastModified);
                    dbFile.dirty(true);
                    updateRequired = true;
                }
            }
        }
    }

    Locale::ptr locale = Locale::GetInstance();
    Collator::ptr collator = locale->GetCollator();
    if (!HasPositionInfo(dbFiles))
    {
        dbFiles.insert(dbFiles.end(), newFiles.begin(), newFiles.end());
        SortDbFiles(dbFiles, collator);
    }
    else
    {
        // We have position info, so we need to update the position.
        dbFiles.insert(dbFiles.end(), newFiles.begin(), newFiles.end());
        SortDbFiles(dbFiles, collator);

        for (size_t i = 0; i < dbFiles.size(); ++i)
        {
            auto newPosition = static_cast<int32_t>(i);
            if (newPosition != dbFiles[i].position())
            {
                dbFiles[i].dirty(true);
            }
            dbFiles[i].position(newPosition);
        }
    }
    for (auto &dbFile : dbFiles)
    {
        if (dbFile.dirty())
        {
            if (audioFilesDb)
            {
                audioFilesDb->WriteFile(&dbFile);
            }
            dbFile.dirty(false);
        }
    }
    if (transaction)
    {
        transaction->commit();
        transaction = nullptr;
    }
    return dbFiles;
}

void AudioDirectoryInfoImpl::DbDeleteFile(DbFileInfo *dbFile)
{
    if (this->audioFilesDb)
    {
        audioFilesDb->DeleteFile(dbFile);
    }
}
ThumbnailTemporaryFile AudioDirectoryInfoImpl::DefaultThumbnailTemporaryFile()
{
    ThumbnailTemporaryFile tempFile;
    fs::path defaultThumbnail = GetResourceDirectory() / "img/missing_thumbnail.jpg";
    tempFile.SetNonDeletedPath(defaultThumbnail, "image/jpeg");
    return tempFile;
}

ThumbnailTemporaryFile AudioDirectoryInfoImpl::GetUnindexedThunbnail(
    const std::string &fileNameOnly, int32_t width, int32_t height)
{
    fs::path file = this->path / fileNameOnly;
    if (!fs::exists(file))
    {
        return DefaultThumbnailTemporaryFile();
    }
    try
    {
        auto tempFile = pipedal::GetAudioFileThumbnail(
            file, width, height,
            GetTemporaryDirectory());
        auto thumbnailPath = tempFile.Path();
        ThumbnailTemporaryFile result;
        result.Attach(tempFile.Detach(), "image/jpeg");

        // Read the thumbnail data from the temporary file.
        std::ifstream thumbnailStream(thumbnailPath, std::ios::binary);
        if (!thumbnailStream)
        {
            throw std::runtime_error("Failed to open thumbnail file: " + thumbnailPath.string());
        }
        std::vector<uint8_t> thumbnailData(
            (std::istreambuf_iterator<char>(thumbnailStream)),
            std::istreambuf_iterator<char>());

        if (!thumbnailData.empty())
        {
            audioFilesDb->AddThumbnail(
                fileNameOnly,
                width,
                height,
                thumbnailData);

            // Set the MIME type for the thumbnail.
            result.SetMimeType("image/jpeg");
            audioFilesDb->UpdateThumbnailInfo(
                fileNameOnly,
                ThumbnailType::Embedded);
            return result;
        }
        else
        {
            throw std::runtime_error("Thumbnail data is empty.");
        }
    }
    catch (const std::exception &e)
    {
        Lv2Log::error("GetUnindexedThunbnail failed: %s", e.what());
    }
    return DefaultThumbnailTemporaryFile();
}

fs::path AudioDirectoryInfoImpl::GetFolderFile() const
{
    for (const auto &coverArtFile : COVER_ART_FILES)
    {
        fs::path thumbnailPath = path / coverArtFile;
        if (fs::exists(thumbnailPath))
        {
            return thumbnailPath;
        }
    }
    return {};
}

void AudioDirectoryInfoImpl::DbSetThumbnailType(
    int64_t idFile,
    ThumbnailType thumbnailType,
    const std::string &thumbnailFile,
    int64_t thumbnailLastModified)
{
    if (audioFilesDb)
    {
        audioFilesDb->UpdateThumbnailInfo(
            idFile,
            thumbnailType,
            thumbnailFile,
            thumbnailLastModified);
    }
}
void AudioDirectoryInfoImpl::DbSetThumbnailType(
    const std::string &fileNameOnly,
    ThumbnailType thumbnailType,
    const std::string &thumbnailFile,
    int64_t thumbnailLastModified)
{
    if (audioFilesDb)
    {
        audioFilesDb->UpdateThumbnailInfo(
            fileNameOnly,
            thumbnailType,
            thumbnailFile,
            thumbnailLastModified);
    }
}

static ThumbnailTemporaryFile GetFolderTemporaryFile(const fs::path &folderFile)
{
    ThumbnailTemporaryFile result;
    result.SetNonDeletedPath(folderFile, MimeTypes::instance().MimeTypeFromExtension(folderFile.extension().string()));
    return result;
}
ThumbnailTemporaryFile AudioDirectoryInfoImpl::GetThumbnail(const std::string &fileNameOnly, int32_t width, int32_t height)
{
    fs::path file = this->path / fileNameOnly;
    OpenAudioDb();
    if (audioFilesDb)
    {
        try
        {

            ThumbnailInfo thumbnailInfo = audioFilesDb->GetThumbnailInfo(file.filename().string());

            if (thumbnailInfo.thumbnailType() == ThumbnailType::Folder)
            {
                fs::path thumbnailFile = path / thumbnailInfo.thumbnailFile();
                if (!fs::exists(thumbnailFile) ||
                    fileTimeToInt64(fs::last_write_time(thumbnailFile)) !=
                        thumbnailInfo.thumbnailLastModified())
                {
                    // reset the thumbnail type and try again.
                    audioFilesDb->UpdateThumbnailInfo(
                        fileNameOnly,
                        ThumbnailType::Unknown);
                    return GetThumbnail(fileNameOnly, width, height);
                }
                fs::path fullFolderPath = this->path / thumbnailInfo.thumbnailFile();
                return GetFolderTemporaryFile(fullFolderPath);
            }
            if (thumbnailInfo.thumbnailType() == ThumbnailType::Embedded)
            {
                std::vector<uint8_t> blob = audioFilesDb->GetEmbeddedThumbnail(fileNameOnly, width, height);

                if (!blob.empty())
                {
                    ThumbnailTemporaryFile tempFile = ThumbnailTemporaryFile::CreateTemporaryFile(GetTemporaryDirectory(), "image/jpeg");

                    tempFile.SetMimeType("image/jpeg");
                    // write buffer to tempFile.GetPath()
                    std::ofstream outFile(tempFile.Path(), std::ios::binary);
                    if (!outFile)
                    {
                        throw std::runtime_error("Failed to open temporary file for writing: " + tempFile.Path().string());
                    }
                    outFile.write((const char *)blob.data(), blob.size());
                    return tempFile;
                }
                else
                {
                    // fall through and generate the indexed thumbnail.
                }
            }

            if (thumbnailInfo.thumbnailType() == ThumbnailType::None)
            {
                return DefaultThumbnailTemporaryFile();
            }

            try
            {
                auto tempFile = pipedal::GetAudioFileThumbnail(
                    this->path / fileNameOnly,
                    width, height,
                    GetTemporaryDirectory());
                auto thumbnailPath = tempFile.Path();
                ThumbnailTemporaryFile result;
                result.Attach(tempFile.Detach(), "image/jpeg");

                // Read the thumbnail data from the temporary file.
                std::ifstream thumbnailStream(thumbnailPath, std::ios::binary);
                if (!thumbnailStream)
                {
                    throw std::runtime_error("Failed to open thumbnail file: " + thumbnailPath.string());
                }
                std::vector<uint8_t> thumbnailData(
                    (std::istreambuf_iterator<char>(thumbnailStream)),
                    std::istreambuf_iterator<char>());

                if (!thumbnailData.empty())
                {
                    audioFilesDb->AddThumbnail(
                        fileNameOnly,
                        width,
                        height,
                        thumbnailData);

                    // Set the MIME type for the thumbnail.
                    result.SetMimeType("image/jpeg");
                    audioFilesDb->UpdateThumbnailInfo(
                        fileNameOnly,
                        ThumbnailType::Embedded);
                    return result;
                }
                else
                {
                    throw std::runtime_error("Thumbnail data is empty.");
                }
                return result;
            }
            catch (const std::exception &_)
            {
            }
            auto folderFile = GetFolderFile();
            if (!folderFile.empty())
            {
                // We have a folder thumbnail, set the type and return it.
                fs::path t = this->path / folderFile;
                int64_t lastModified = fileTimeToInt64(fs::last_write_time(t));
                DbSetThumbnailType(fileNameOnly, ThumbnailType::Folder, folderFile, lastModified);
                auto fullFolderPath = this->path / folderFile;
                return GetFolderTemporaryFile(fullFolderPath);
            }
            DbSetThumbnailType(
                fileNameOnly,
                ThumbnailType::None); // No thumbnail available, set to None.
            return DefaultThumbnailTemporaryFile();
        }
        catch (const std::exception &e)
        {
        }
    }
    return GetUnindexedThunbnail(fileNameOnly, width, height);
}


void AudioDirectoryInfoImpl::UpdateMetadata(DbFileInfo *dbFile)
{
    fs::path file = this->path / dbFile->fileName();
    AudioFileMetadata metadata(file);
    dbFile->title(metadata.title());
    dbFile->track(metadata.track());
    dbFile->duration(metadata.duration());
    dbFile->album(metadata.album());
    dbFile->artist(metadata.artist());
    dbFile->albumArtist(metadata.albumArtist());
    dbFile->lastModified(GetLastWriteTime(file));
    dbFile->duration(metadata.duration());
}

ThumbnailTemporaryFile::ThumbnailTemporaryFile(const std::filesystem::path &temporaryDirectory)
    : TemporaryFile(temporaryDirectory)
{
    // Set a default MIME type for the thumbnail.
    mimeType = "image/jpeg"; // Default MIME type, can be set later.
}

ThumbnailTemporaryFile ThumbnailTemporaryFile::CreateTemporaryFile(const std::filesystem::path &temporaryDirectory, const std::string &mimeType)
{
    ThumbnailTemporaryFile result(temporaryDirectory);
    result.SetMimeType(mimeType);
    return result;
}

ThumbnailTemporaryFile ThumbnailTemporaryFile::FromFile(const std::filesystem::path &filePath, const std::string &mimeType)
{
    ThumbnailTemporaryFile result;
    result.Attach(filePath, mimeType);
    return result;
}

void ThumbnailTemporaryFile::Attach(const std::filesystem::path &filePath, const std::string &mimeType)
{
    super::Attach(filePath);
    SetMimeType(mimeType);
}

size_t AudioDirectoryInfoImpl::TestGetNumberOfThumbnails()
{
    if (!audioFilesDb)
    {
        throw std::logic_error("DB not open");
    }
    return this->audioFilesDb->GetNumberOfThumbnails();
}

void AudioDirectoryInfoImpl::OpenAudioDb()
{

    if (!this->audioFilesDb)
    {
        try
        {
            this->audioFilesDb = std::make_shared<AudioFilesDb>(this->path, indexPath);
        }
        catch (const SQLite::Exception &e)
        {
            Lv2Log::debug("Can't create .index.pipedal: %s - %s", e.what(), this->path.c_str());
        }
    }
}


std::string AudioDirectoryInfoImpl::GetNextAudioFile(const std::string &fileNameOnly)
{
    auto files = this->GetFiles();
    if (files.empty())
    {
        return "";
    }
    for (size_t i = 0; i < files.size(); ++i)
    {
        if (files[i].fileName() == fileNameOnly)
        {
            if (i + 1 < files.size())
            {
                return files[i + 1].fileName();
            }
            else
            {
                return files[0].fileName(); // wrap around to the first file.
            }
        }
    }
    return "";
}
std::string AudioDirectoryInfoImpl::GetPreviousAudioFile(const std::string &fileNameOnly)
{
    auto files = this->GetFiles();
    if (files.empty())
    {
        return "";
    }
    for (size_t i = 0; i < files.size(); ++i)
    {
        if (files[i].fileName() == fileNameOnly)
        {
            if (i > 0)
            {
                return files[i - 1].fileName();
            }
            else
            {
                return files[files.size() - 1].fileName(); // wrap around to the last file.
            }
        }
    }
    return "";
}

void AudioDirectoryInfoImpl::MoveAudioFile(
    const std::string &directory,
    int32_t fromPosition,
    int32_t toPosition)
{
    OpenAudioDb();
    if (!audioFilesDb)
    {
        throw std::runtime_error("Directory is not writable.");
    }
    auto files = this->UpdateDbFiles();
    if (directory.empty() || fromPosition < 0 || toPosition < 0 ||
        fromPosition >= static_cast<int32_t>(files.size()) ||
        toPosition > static_cast<int32_t>(files.size()))
    {
        throw std::invalid_argument("Invalid arguments for MoveAudioFile");
    }
    std::shared_ptr<SQLite::Transaction> transaction;
    if (audioFilesDb)
    {
        transaction = audioFilesDb->transaction();
    }

    if (fromPosition == toPosition)
    {
        return; // No change needed.
    }
    if (fromPosition > toPosition)
    {
        auto t = files[fromPosition];
        files.erase(files.begin() + fromPosition);
        files.insert(files.begin() + toPosition, t);
    }
    else
    {
        // fromPosition < toPosition
        auto t = files[fromPosition];
        files.erase(files.begin() + fromPosition);
        files.insert(files.begin() + toPosition, t); // insert before the toPosition.
    }
    for (size_t i = 0; i < files.size(); ++i)
    {
        auto &file = files[i];
        if (file.position() != static_cast<int32_t>(i))
        {
            file.position(static_cast<int32_t>(i));
            if (this->audioFilesDb)
            {
                audioFilesDb->UpdateFilePosition(file.idFile(), file.position());
            }
            else
            {
                throw std::runtime_error("Directory is not writable.");
            }
        }
    }
    transaction->commit();
}

namespace pipedal
{
    std::filesystem::path IndexPathToShadowIndexPath(const std::filesystem::path &audioRootDirectory, const std::filesystem::path &path)
    {
        std::filesystem::path relativePath = MakeRelativePath(path, audioRootDirectory);
        if (relativePath.is_absolute())
        {
            throw std::runtime_error(SS("Can't write to path " << path << "."));
        };
        return audioRootDirectory / "shadow_indexes" / relativePath;
    }

    // recover the original path fromthe shadow index path.
    std::filesystem::path ShadowIndexPathToIndexPath(const std::filesystem::path &audioRootDirectory, const std::filesystem::path &path)
    {
        auto shadowIndexesPath = audioRootDirectory / "shadow_indexes";
        std::filesystem::path relativePath = MakeRelativePath(path, shadowIndexesPath);
        if (relativePath.is_absolute())
        {
            throw std::runtime_error(SS("Path must start with " << shadowIndexesPath));
        }
        return audioRootDirectory / relativePath; // recover the original path.
    }

    std::filesystem::path GetShadowIndexDirectory(const std::filesystem::path &audioRootDirectory, const std::filesystem::path &path)
    {
        if (HasWritePermissions(path))
        {
            return ""; // use default index path.
        }
        return IndexPathToShadowIndexPath(audioRootDirectory, path);
    }
}

void AudioDirectoryInfoImpl::SetFileMetadata(
    const std::string &fileNameOnly,
    const std::string &key,
    const std::string &value)
{
    OpenAudioDb();
    if (!audioFilesDb)
    {
        throw std::runtime_error("Directory is not writable.");
    }
    this->UpdateDbFiles();

    audioFilesDb->SetFileExtraMetadata(fileNameOnly, key, value);
}

std::string AudioDirectoryInfoImpl::GetFileMetadata(
            const std::string &fileNameOnly,
            const std::string &key) 
{
    OpenAudioDb();
    if (!audioFilesDb)
    {
        throw std::runtime_error("Directory is not writable.");
    }
    this->UpdateDbFiles(); // check for deletions.

    return audioFilesDb->GetFileExtraMetadata(fileNameOnly, key);       
}


