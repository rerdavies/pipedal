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

#include "AudioFilesDb.hpp"
#include <stdexcept>
#include <atomic>
#include <mutex>
#include <map>

using namespace pipedal;
using namespace pipedal::impl;
namespace fs = std::filesystem;

namespace pipedal::impl
{
    struct DatabaseLockEntry
    {
        virtual ~DatabaseLockEntry() = default;

        using ptr = std::shared_ptr<DatabaseLockEntry>;

        std::mutex mutex;
    };
    static std::map<std::filesystem::path, std::shared_ptr<DatabaseLockEntry>> databaseLocks;

    std::mutex databaseLockMutex;

    class DatabaseLock
    {
    private:
        fs::path indexFile;
        std::shared_ptr<DatabaseLockEntry> lockEntry;

        // Disable copy and move constructors
        DatabaseLock(const DatabaseLock &) = delete;
        DatabaseLock &operator=(const DatabaseLock &) = delete;
        DatabaseLock(DatabaseLock &&) = delete;
        DatabaseLock &operator=(DatabaseLock &&) = delete;

    public:
        explicit DatabaseLock(const fs::path &lockFilePath)
            : indexFile(lockFilePath)
        {
            {
                std::lock_guard<std::mutex> lock(databaseLockMutex);

                auto it = databaseLocks.find(indexFile);
                if (it == databaseLocks.end())
                {
                    // Create a new lock entry
                    DatabaseLockEntry::ptr entry = std::make_shared<DatabaseLockEntry>();
                    databaseLocks[indexFile] = entry;
                    lockEntry = entry;
                }
                else
                {
                    lockEntry = it->second;
                }
            }
            lockEntry->mutex.lock(); // we now have exclusive access to the database.
        }

        ~DatabaseLock()
        {
            lockEntry->mutex.unlock(); // release the lock
            {
                std::lock_guard<std::mutex> lock(databaseLockMutex);
                auto use_count = lockEntry.use_count();

                if (use_count == 2) // one for us, one for the Lockentry
                {
                    // Remove the entry from the map if no other locks are held.
                    databaseLocks.erase(indexFile);
                }
            }
        }
    };
}

static std::unique_ptr<DatabaseLock> getDatabaseLock(const std::filesystem::path &path)
{
    // Create a lock file in the same directory as the database.
    return std::make_unique<DatabaseLock>(path);
}

AudioFilesDb::AudioFilesDb(
    const std::filesystem::path &path,
    const std::filesystem::path &indexPath)
    : path(path)
{
    fs::path dbPathName = this->path / ".pipedal.index";
    if (!indexPath.empty())
    {
        dbPathName = indexPath;
    }

    dbLock = getDatabaseLock(dbPathName);

    if (!fs::exists(dbPathName))
    {
        CreateDb(dbPathName);
    }
    else
    {
        this->db = std::make_unique<SQLite::Database>(dbPathName, SQLite::OPEN_READWRITE);
        UpgradeDb();
    }
}

void AudioFilesDb::UpgradeDb()
{
    int version = QueryVersion();
}

int AudioFilesDb::QueryVersion()
{
    SQLite::Statement query(*db, "SELECT version FROM am_dbInfo LIMIT 1");

    // Fetch the result
    if (query.executeStep())
    {
        int version = query.getColumn(0).getInt();
        return version;
    }
    else
    {
        throw std::runtime_error("QueryVersion failed.");
    }
}

void AudioFilesDb::CreateDb(const std::filesystem::path &dbPathName)
{
    try
    {

        this->db = std::make_unique<SQLite::Database>(dbPathName, SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);

        try
        {
            db->exec("CREATE TABLE am_dbInfo ("
                     "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "version INTEGER NOT NULL)");

            {
                SQLite::Statement query(*db, "INSERT INTO am_dbInfo (version) VALUES (?)");
                query.bind(1, DB_VERSION);
                query.exec();
            }

            db->exec(
                "CREATE TABLE IF NOT EXISTS files ("
                "idFile INTEGER PRIMARY KEY AUTOINCREMENT, "
                "fileName TEXT NOT NULL, "
                "lastModified INT64 NOT NULL,"
                "title TEXT NOT NULL,"
                "track NUMBER NOT NULL,"
                "album TEXT NOT NULL, "
                "artist TEXT NOT NULL, "
                "albumArtist TEXT NOT NULL, "
                "duration REAL NOT NULL DEFAULT 0.0,"

                "thumbnailType INTEGER NOT NULL DEFAULT 0,"
                "position INTEGER NOT NULL DEFAULT -1,"
                "thumbnailFile TEXT NOT NULL DEFAULT \"\","
                "thumbnailLastModified INT64 NOT NULL DEFAULT 0"
                ")");

            db->exec("CREATE TABLE IF NOT EXISTS thumbnails ("
                     "idThumbnail INTEGER PRIMARY KEY AUTOINCREMENT, "
                     "idFile INT64, "
                     "thumbnail BLOB, "
                     "width INTEGER, "
                     "height INTEGER)");
        }
        catch (const SQLite::Exception &e)
        {
            throw std::runtime_error("Failed to create database tables: " + std::string(e.what()));
        }
    }
    catch (const SQLite::Exception &e)
    {
        throw std::runtime_error("Failed to create database: " + std::string(e.what()));
    }
}

void AudioFilesDb::DeleteFile(DbFileInfo *dbFile)
{
    DeleteThumbnails(dbFile->idFile());
    if (!deleteFileQuery)
    {
        deleteFileQuery = std::make_unique<SQLite::Statement>(
            *db,
            "DELETE FROM files WHERE idFile = ?");
    }
    deleteFileQuery->tryReset();
    deleteFileQuery->bind(1, dbFile->idFile());
    deleteFileQuery->exec();
}

void AudioFilesDb::DeleteThumbnails(id_t idFile)
{
    if (!updateThumbnailInfoQueryByName)
    {
        updateThumbnailInfoQueryByName = std::make_unique<SQLite::Statement>(
            *db,
            "DELETE FROM thumbnails WHERE idFile = ?");
    }
    updateThumbnailInfoQueryByName->tryReset();
    updateThumbnailInfoQueryByName->bind(1, idFile);
    updateThumbnailInfoQueryByName->exec();
}

size_t AudioFilesDb::GetNumberOfThumbnails()
{
    SQLite::Statement query(*db, "SELECT COUNT(*) FROM thumbnails");
    if (query.executeStep())
    {
        return query.getColumn(0).getInt64();
    }
    return 0;
}

void AudioFilesDb::UpdateThumbnailInfo(
    const std::string &fileName,
    ThumbnailType thumbnailType,
    const std::string &thumbnailFile,
    int64_t thumbnailLastModified)
{
    if (!updateThumbnailInfoQueryByName)
    {
        updateThumbnailInfoQueryByName = std::make_unique<SQLite::Statement>(
            *db,
            "UPDATE files SET thumbnailType = ?,thumbnailFile = ?, thumbnailLastModified = ? WHERE fileName = ?");
    }
    updateThumbnailInfoQueryByName->tryReset();
    updateThumbnailInfoQueryByName->bind(1, (int32_t)thumbnailType);
    updateThumbnailInfoQueryByName->bind(2, thumbnailFile);
    updateThumbnailInfoQueryByName->bind(3, thumbnailLastModified);
    updateThumbnailInfoQueryByName->bind(4, fileName);
    updateThumbnailInfoQueryByName->exec();
}

void AudioFilesDb::UpdateThumbnailInfo(
    int64_t idFile,
    ThumbnailType thumbnailType,
    const std::string &thumbnailFile,
    int64_t thumbnailLastModified)
{
    if (!updateThumbnailInfoQueryById)
    {
        updateThumbnailInfoQueryById = std::make_unique<SQLite::Statement>(
            *db,
            "UPDATE files SET thumbnailType = ?,thumbnailFile = ?, thumbnailLastModified = ? WHERE idFile = ?");
    }
    updateThumbnailInfoQueryById->bind(1, (int32_t)thumbnailType);
    updateThumbnailInfoQueryById->bind(2, thumbnailFile);
    updateThumbnailInfoQueryById->bind(3, thumbnailLastModified);
    updateThumbnailInfoQueryById->bind(4, idFile);
    updateThumbnailInfoQueryById->exec();
}

std::unique_ptr<SQLite::Transaction> AudioFilesDb::transaction()
{
    return std::make_unique<SQLite::Transaction>(*db);
}

std::vector<DbFileInfo> AudioFilesDb::QueryTracks()
{
    std::vector<DbFileInfo> result;
    // get tracks fro the databse.
    SQLite::Statement query(
        *db,
        "SELECT idFile, fileName, "
        "lastModified, title, track, album, artist,albumArtist,duration, "
        "thumbnailType, position, "
        "thumbnailFile, thumbnailLastModified "
        "FROM files ");

    while (query.executeStep())
    {
        DbFileInfo row;
        row.idFile(query.getColumn(0).getInt64());
        row.fileName(query.getColumn(1).getText());
        row.lastModified(query.getColumn(2).getInt64());
        row.title(query.getColumn(3).getText());
        row.track(query.getColumn(4).getInt());
        row.album(query.getColumn(5).getText());
        row.artist(query.getColumn(6).getText());
        row.albumArtist(query.getColumn(7).getText());
        row.duration(query.getColumn(8).getDouble());
        row.thumbnailType((ThumbnailType)query.getColumn(9).getInt());
        row.position(query.getColumn(10).getInt());
        row.thumbnailFile(query.getColumn(11).getText());
        row.thumbnailLastModified(query.getColumn(12).getInt64());
        result.push_back(std::move(row));
    }
    return result;
}

void AudioFilesDb::WriteFile(DbFileInfo *dbFile)
{
    if (dbFile->idFile() == -1)
    {
        if (!insertFileQuery)
        {
            insertFileQuery = std::make_unique<SQLite::Statement>(
                *db,
                "INSERT INTO files ("
                "fileName, lastModified,"
                "title,track,album,artist, albumArtist, "
                "duration, thumbnailType,thumbnailFile, thumbnailLastModified, position "
                ") VALUES (?, ?, ?, ?, ?,?,?,?,?,?,?,?)");
        }
        insertFileQuery->tryReset();
        insertFileQuery->bind(1, dbFile->fileName());
        insertFileQuery->bind(2, dbFile->lastModified());
        insertFileQuery->bind(3, dbFile->title());
        insertFileQuery->bind(4, dbFile->track());
        insertFileQuery->bind(5, dbFile->album());
        insertFileQuery->bind(6, dbFile->artist());
        insertFileQuery->bind(7, dbFile->albumArtist());
        insertFileQuery->bind(8, dbFile->duration());
        insertFileQuery->bind(9, (int32_t)dbFile->thumbnailType());
        insertFileQuery->bind(10, dbFile->thumbnailFile());
        insertFileQuery->bind(11, dbFile->thumbnailLastModified());
        insertFileQuery->bind(12, dbFile->position());
        insertFileQuery->exec();
        dbFile->idFile(db->getLastInsertRowid());
    }
    else
    {
        if (!updateFileQuery)
        {
            updateFileQuery = std::make_unique<SQLite::Statement>(
                *db,
                "UPDATE files SET "
                "fileName = ?, lastModified = ?, "
                "title = ?, track = ?, album = ?, artist = ? , albumArtist = ?, "
                "duration = ?, thumbnailType = ?, "
                "thumbnailFile = ?, thumbnailLastModified = ?, position = ? "
                " WHERE idFile = ?");
        }
        updateFileQuery->tryReset();
        updateFileQuery->bind(1, dbFile->fileName());
        updateFileQuery->bind(2, dbFile->lastModified());
        updateFileQuery->bind(3, dbFile->title());
        updateFileQuery->bind(4, dbFile->track());
        updateFileQuery->bind(5, dbFile->album());
        updateFileQuery->bind(6, dbFile->artist());
        updateFileQuery->bind(7, dbFile->albumArtist());
        updateFileQuery->bind(8, dbFile->duration());
        updateFileQuery->bind(9, (int32_t)dbFile->thumbnailType());
        updateFileQuery->bind(10, dbFile->thumbnailFile());
        updateFileQuery->bind(11, dbFile->thumbnailLastModified());
        updateFileQuery->bind(12, dbFile->position());

        updateFileQuery->bind(13, dbFile->idFile());

        updateFileQuery->exec();
    }
}

ThumbnailInfo AudioFilesDb::GetThumbnailInfo(const std::string &fileNameOnly)
{
    ThumbnailInfo result;
    SQLite::Statement query(
        *db,
        "SELECT thumbnailType, thumbnailFile, thumbnailLastModified FROM files WHERE fileName = ?");
    query.bind(1, fileNameOnly);
    if (query.executeStep())
    {
        result.thumbnailType((ThumbnailType)query.getColumn(0).getInt());
        result.thumbnailFile(query.getColumn(1).getText());
        result.thumbnailLastModified(query.getColumn(2).getInt64());
        return result;
    }
    else
    {
        throw std::runtime_error("No thumbnail info found for file.");
    }
    return result;
}

std::vector<uint8_t> AudioFilesDb::GetEmbeddedThumbnail(int64_t idFile, int32_t width, int32_t height)
{
    SQLite::Statement query(*db,
                            "SELECT thumbnail FROM thumbnails "
                            "WHERE idFile = ? AND width = ? AND height = ?");
    query.bind(1, idFile);
    query.bind(2, width);
    query.bind(3, height);

    if (query.executeStep())
    {
        const uint8_t *data = (uint8_t *)query.getColumn(0).getBlob();
        int size = query.getColumn(0).getBytes();
        return std::vector<uint8_t>(data, data + size);
    }
    else
    {
        return {};
    }
}

std::vector<uint8_t> AudioFilesDb::GetEmbeddedThumbnail(const std::string &fileNameOnly, int32_t width, int32_t height)
{
    int64_t idFile;
    SQLite::Statement query(*db, "SELECT idFile FROM files WHERE fileName = ?");
    query.bind(1, fileNameOnly);
    if (query.executeStep())
    {
        idFile = query.getColumn(0).getInt64();
    }
    else
    {
        throw std::runtime_error("File not found in database: " + fileNameOnly);
    }
    return GetEmbeddedThumbnail(idFile, width, height);
}

void AudioFilesDb::AddThumbnail(
    const std::string &fileName,
    int64_t width,
    int64_t height,
    const std::vector<uint8_t> &thumbnailData)
{
    int64_t idFile;
    SQLite::Statement query(*db, "SELECT idFile FROM files WHERE fileName = ?");
    query.bind(1, fileName);
    if (query.executeStep())
    {
        idFile = query.getColumn(0).getInt64();
    }
    else
    {
        throw std::runtime_error("File not found in database: " + fileName);
    }
    AddThumbnail(idFile, width, height, thumbnailData);
}
void AudioFilesDb::AddThumbnail(
    int64_t idFile,
    int64_t width,
    int64_t height,
    const std::vector<uint8_t> &thumbnailData)
{
    if (thumbnailData.empty())
    {
        throw std::runtime_error("Thumbnail data is empty.");
    }

    SQLite::Statement insertQuery(
        *db,
        "INSERT INTO thumbnails (idFile, thumbnail, width, height) VALUES (?, ?, ?, ?)");
    insertQuery.bind(1, idFile);
    insertQuery.bind(2, thumbnailData.data(), thumbnailData.size());
    insertQuery.bind(3, width);
    insertQuery.bind(4, height);

    try
    {
        insertQuery.exec();
    }
    catch (const SQLite::Exception &e)
    {
        throw std::runtime_error("Failed to add thumbnail: " + std::string(e.what()));
    }
}

void AudioFilesDb::UpdateFilePosition(
    int64_t idFile,
    int32_t position)
{
    if (!updateFileQuery)
    {
        updateFileQuery = std::make_unique<SQLite::Statement>(
            *db,
            "UPDATE files SET position = ? WHERE idFile = ?");
    }
    updateFileQuery->tryReset();
    updateFileQuery->bind(1, position);
    updateFileQuery->bind(2, idFile);
    updateFileQuery->exec();
}
