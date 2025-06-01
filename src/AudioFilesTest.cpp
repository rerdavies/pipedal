// Copyright (c) 2025 Robin Davies
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
#include "catch.hpp"
#include <string>
#include <iostream>
#include "AudioFiles.hpp"
#include <filesystem>
#include "SysExec.hpp"
#include "json.hpp"
#include "json_variant.hpp"
#include "MimeTypes.hpp"

using namespace std;
using namespace pipedal;
namespace fs = std::filesystem;

fs::path testDir = fs::temp_directory_path() / "AudioFilesTest" / "files";
fs::path sourceDirectory = fs::current_path() / "artifacts" / "TestTracks";
fs::path tempDir = fs::temp_directory_path() / "AudioFilesTest" / "tmp";
fs::path resourceDir = fs::current_path() / "vite" / "public" / "img";
fs::path textIndexPath = fs::temp_directory_path() / "AudioFilesTest" / "TextIndex.pipedal";

static void CreateTestDirectory()
{
    if (fs::exists(testDir))
    {
        fs::remove_all(testDir);
    }
    fs::remove_all(tempDir);
    fs::create_directories(testDir);
    if (fs::exists(tempDir))
    {
        fs::remove_all(tempDir);
    }
    fs::create_directories(tempDir);
    AudioDirectoryInfo::SetTemporaryDirectory(tempDir);
    AudioDirectoryInfo::SetResourceDirectory(resourceDir);
}

static void CheckForLeakedTemporaryFiles()
{
    bool leakedFiles = false;
    // Check if the temporary directory exists and is not empty
    if (fs::exists(tempDir) && fs::is_directory(tempDir))
    {
        for (const auto &entry : fs::directory_iterator(tempDir))
        {
            if (entry.is_regular_file())
            {
                cout << "Leaked temporary file: " << entry.path() << endl;
                leakedFiles = true;
            }
        }
    }
    if (fs::exists(tempDir) && fs::is_directory(tempDir / "thumbnails"))
    {
        for (const auto &entry : fs::directory_iterator(tempDir / "thumbnails"))
        {
            if (entry.is_regular_file())
            {
                cout << "Leaked temporary file: " << entry.path() << endl;
                leakedFiles = true;
            }
        }
    }
    if (leakedFiles)
    {
        throw std::runtime_error("Leaked temporary files found in: " + tempDir.string());
    }
}

static bool ContainsTrack(const std::vector<AudioFileMetadata> &files, const std::string &trackName)
{
    for (const auto &file : files)
    {
        if (file.title() == trackName)
        {
            return true;
        }
    }
    return false;
}
static bool ContainsFile(const std::vector<AudioFileMetadata> &files, const std::string &fileName)
{
    for (const auto &file : files)
    {
        if (file.fileName() == fileName)
        {
            return true;
        }
    }
    return false;
}

static const AudioFileMetadata *GetFile(const std::vector<AudioFileMetadata> &files, const std::string &fileName)
{
    for (const auto &file : files)
    {
        if (file.fileName() == fileName)
        {
            return &file;
        }
    }
    throw std::logic_error("File not found: " + fileName);
}

static void PrintFiles(const std::vector<AudioFileMetadata> &files)
{
    for (const auto &file : files)
    {
        cout << "File: " << file.fileName() << ", Title: " << file.track() << ". " << file.title() << " Album: " << file.album() << endl;
    }
    cout << endl;
}

void IndexTest()
{
    CreateTestDirectory();

    try
    {
        {
            AudioDirectoryInfo::Ptr audioDir = AudioDirectoryInfo::Create(testDir.string());

        }
        AudioDirectoryInfo::Ptr audioDir = AudioDirectoryInfo::Create(testDir.string());
        REQUIRE(audioDir != nullptr);
        if (fs::exists(testDir))
        {
            fs::remove_all(testDir); // Remove any existing database file
        }
        fs::create_directories(testDir);
        // Copy test files to the test directory
        for (const auto &entry : fs::directory_iterator(sourceDirectory))
        {
            if (entry.is_regular_file())
            {
                fs::copy(entry.path(), testDir / entry.path().filename());
            }
        }

        auto files = audioDir->GetFiles();
        PrintFiles(files);

        REQUIRE(files.size() == 3);

        REQUIRE(ContainsTrack(files, "Track 1"));
        REQUIRE(ContainsTrack(files, "Track 2"));
        REQUIRE(ContainsTrack(files, "Track 3"));

        for (const auto &file : files)
        {
            REQUIRE(!file.fileName().empty());
        }

        // check indexed results.
        files = audioDir->GetFiles();
        REQUIRE(files.size() == 3);
        for (const auto &file : files)
        {
            REQUIRE(!file.fileName().empty());
        }

        fs::remove(testDir / "Track2.mp3");

        files = audioDir->GetFiles();
        REQUIRE(files.size() == 2);
        REQUIRE(!ContainsTrack(files, "Track 2"));
        REQUIRE(ContainsTrack(files, "Track 1"));
        REQUIRE(ContainsTrack(files, "Track 3"));

        PrintFiles(files);
        files = audioDir->GetFiles();
        REQUIRE(files.size() == 2);
        REQUIRE(!ContainsTrack(files, "Track 2"));
        REQUIRE(ContainsTrack(files, "Track 1"));
        REQUIRE(ContainsTrack(files, "Track 3"));

        // check dbinex results.

        fs::copy_file(sourceDirectory / "Track2.mp3", testDir / "Track3.mp3", fs::copy_options::overwrite_existing);
        files = audioDir->GetFiles();
        REQUIRE(files.size() == 2);
        REQUIRE(ContainsTrack(files, "Track 2"));
        REQUIRE(ContainsFile(files, "Track3.mp3"));
        REQUIRE(!ContainsFile(files, "Track2.mp3"));

        fs::copy_file(sourceDirectory / "Track3.mp3", testDir / "Track4.mp3", fs::copy_options::overwrite_existing);
        files = audioDir->GetFiles();
        REQUIRE(files.size() == 3);
        REQUIRE(ContainsTrack(files, "Track 2"));
        REQUIRE(ContainsFile(files, "Track3.mp3"));
        REQUIRE(GetFile(files, "Track4.mp3")->title() == "Track 3");

        {
            auto thumbnail = audioDir->GetThumbnail("Track1.mp3", 200, 200);
            REQUIRE(audioDir->TestGetNumberOfThumbnails() == 1);
        }
        {
            auto thumbnail = audioDir->GetThumbnail("Track1.mp3", 200, 200);
            REQUIRE(audioDir->TestGetNumberOfThumbnails() == 1);
        }
        {
            auto thumbnail = audioDir->GetThumbnail("Track1.mp3", 0, 0);
            REQUIRE(audioDir->TestGetNumberOfThumbnails() == 2);
        }
        CheckForLeakedTemporaryFiles();
    }
    catch (const std::exception &e)
    {
        cout << "Exception: " << e.what() << endl;
        REQUIRE(false);
    }
}

static void ThumbnailTest()
{
    cout << "ThumbnailTest" << endl;
    CreateTestDirectory();

    try
    {
        AudioDirectoryInfo::Ptr audioDir = AudioDirectoryInfo::Create(testDir.string());
        REQUIRE(audioDir != nullptr);
        if (fs::exists(testDir))
        {
            fs::remove_all(testDir); // Remove any existing database file
        }
        fs::create_directories(testDir);
        // Copy test files to the test directory
        for (const auto &entry : fs::directory_iterator(sourceDirectory))
        {
            if (entry.is_regular_file())
            {
                fs::copy(entry.path(), testDir / entry.path().filename());
            }
        }

        auto files = audioDir->GetFiles();

        REQUIRE(files.size() == 3);

        for (const auto &file : files)
        {
            ThumbnailTemporaryFile thumbnail = audioDir->GetThumbnail(file.fileName(), 200, 200);
            REQUIRE(fs::exists(thumbnail.Path()));
            cout << "Thumbnail for " << file.fileName() << ": " << thumbnail.Path() << endl;

            cout << "    " << file.fileName() << "(200,200) size: " << fs::file_size(thumbnail.Path()) << endl;

            thumbnail = audioDir->GetThumbnail(file.fileName(), 0, 0);
            REQUIRE(fs::exists(thumbnail.Path()));
            cout << "Thumbnail for " << file.fileName() << ": " << thumbnail.Path() << endl;

            cout << "    " << file.fileName() << "(0,0) size: " << fs::file_size(thumbnail.Path()) << endl;
        }

        REQUIRE(audioDir->TestGetNumberOfThumbnails() == 2 * 2); // 3 files, one file is missing a thumbnail, 2 per file.

        {
            ThumbnailTemporaryFile thumbnail = audioDir->GetThumbnail("Track1.mp3", 200, 200);
            REQUIRE(audioDir->TestGetNumberOfThumbnails() == 2 * 2); // 3 files, one file is missing a thumbnail, 2 per file.
        }
        fs::remove(testDir / "Track2.mp3");
        files = audioDir->GetFiles();

        REQUIRE(audioDir->TestGetNumberOfThumbnails() == 2);

        // update the last write time, should trigger a thumbnail update.
        REQUIRE(GetFile(files, "Track1.mp3")->thumbnailType() == ThumbnailType::Embedded);

        fs::copy(sourceDirectory / "Track1.mp3", testDir / "Track1.mp3", fs::copy_options::overwrite_existing);
        files = audioDir->GetFiles();
        REQUIRE(GetFile(files, "Track1.mp3")->thumbnailType() == ThumbnailType::Unknown);
        REQUIRE(audioDir->TestGetNumberOfThumbnails() == 0);

        audioDir->GetThumbnail("Track1.mp3", 200, 200);
        REQUIRE(audioDir->TestGetNumberOfThumbnails() == 1);

        CheckForLeakedTemporaryFiles();
        cout << endl;
    }
    catch (const std::exception &e)
    {
        cout << "Exception: " << e.what() << endl;
        REQUIRE(false);
    }
}

void ExecutionTimeTest()
{
    cout << "Timing test" << endl;
    CreateTestDirectory();

    // deploy sample files.
    fs::create_directories(testDir);
    // Copy test files to the test directory
    for (const auto &entry : fs::directory_iterator(sourceDirectory))
    {
        if (entry.is_regular_file())
        {
            fs::copy(entry.path(), testDir / entry.path().filename());
        }
    }

    AudioDirectoryInfo::Ptr audioDir = AudioDirectoryInfo::Create(testDir.string());
    audioDir->GetFiles();

    {
        using clock_t = std::chrono::high_resolution_clock;
        {
            auto start = clock_t::now();
            audioDir->GetThumbnail("Track1.mp3",200,200);
            auto duration = clock_t::now() - start;
            cout << "Track1.mp3(200,200):  " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() <<"ms" << endl;
        }
        {
            auto start = clock_t::now();
            audioDir->GetThumbnail("Track1.mp3",200,200);
            auto duration = clock_t::now() - start;
            cout << "Track1.mp3(200,200):  " << std::chrono::duration_cast<std::chrono::milliseconds>(duration).count() <<"ms" << endl;
        }
    }
}
TEST_CASE("AudioFiles test", "[AudioFiles]")
{

    IndexTest();
    ThumbnailTest();
    ExecutionTimeTest();
}

// from AudioFileMetadata.cpp
std::string shell_escape_filename(const std::string &filename);

static void ScanThumbnails()
{
    const std::string homeDir = getenv("HOME");
    std::filesystem::path musicDir = std::filesystem::path(homeDir) / "Music";

    for (const auto &entry : std::filesystem::recursive_directory_iterator(musicDir))
    {
        if (entry.is_directory())
        {
            continue; // Skip directories
        }

        if (entry.path().extension() == ".mp3" || entry.path().extension() == ".flac")
        {
            try
            {
                // ffprobe -i ~/tmp/test.flac -show_streams -show_entries stream=index,codec_name,disposition

                std::stringstream args;
                args << "-log_level error -i " << shell_escape_filename(entry.path())
                     << " -show_streams -of json -show_entries stream=index,codec_name,disposition 2>/dev/null";

                std::string sargs = args.str();
                auto output = sysExecForOutput("/usr/bin/ffprobe", sargs, true);
                if (output.exitCode != 0)
                {
                    cout << "ffprobe failed for file: " << entry.path() << endl;
                    continue;
                }
                std::stringstream ss(output.output);
                json_reader reader(ss);

                json_variant vt;
                reader.read(&vt);
                auto streams = vt.as_object()->at("streams").as_array();
                struct StreamInfo
                {
                    int index;
                    std::string codec_name;
                    std::string comment;
                };
                std::vector<StreamInfo> thumbnailStreams;
                for (const auto &stream : *streams)
                {
                    auto streamObj = stream.as_object();
                    int index = streamObj->at("index").as_int64();
                    std::string codecName = streamObj->at("codec_name").as_string();
                    std::string comment;
                    if (streamObj->contains("tags"))
                    {
                        auto &tagsObj = streamObj->at("tags");
                        auto &tags = tagsObj.as_object();
                        if (tags->contains("comment"))
                        {
                            comment = tags->at("comment").as_string();
                        }
                    }
                    thumbnailStreams.push_back({index, codecName, comment});
                }
                if (thumbnailStreams.size() > 2)
                {
                    cout << "File: " << entry.path() << endl;
                    for (const auto &stream : thumbnailStreams)
                    {
                        cout << "  Stream Index: " << stream.index
                             << ", Codec: " << stream.codec_name
                             << ", Comment: " << stream.comment << endl;
                    }
                }
            }
            catch (const std::exception &e)
            {
                cout << "Error processing file: " << entry.path() << " - " << e.what() << endl;
            }
        }
    }
}

class TestTimer {
public:
    using clock_t = std::chrono::high_resolution_clock;
    void Start()
    {
        startTime = clock_t::now();
    }
    uint64_t Stop()
    {
        auto elapsed = clock_t::now()-startTime;
        elapsedMs = std::chrono::duration_cast<std::chrono::milliseconds>(elapsed).count();
        return elapsedMs;

    }
    double ElapsedMs()
    {
        return elapsedMs;
    }
private:
    clock_t::time_point startTime;
    uint64_t elapsedMs = 0;
};

static bool isAudioExtension(const std::string &extension)
{
    return MimeTypes::instance().AudioExtensions().contains(extension);
}


static bool ContainsMusic(const std::filesystem::path&path)
{
    for (auto dirEntry: fs::directory_iterator(path)) {
        if (fs::is_regular_file(dirEntry.path())) {
            if (isAudioExtension(dirEntry.path().extension())) {
                return true;
            }
        }
    }
    return false;
}

void ProfileMusicDirectory() {
    const std::string homeDir = getenv("HOME");
    std::filesystem::path musicDir = std::filesystem::path(homeDir) / "Music";

    for (const auto &entry : std::filesystem::recursive_directory_iterator(musicDir))
    {
        if (!entry.is_directory())
        {
            continue; // Skip directories
        }
        if (!ContainsMusic(entry.path())) {
            continue;
        }

        fs::remove(textIndexPath);
        AudioDirectoryInfo::Ptr directoryInfo = AudioDirectoryInfo::Create(entry.path());
        fs::create_directories(textIndexPath.parent_path());
        directoryInfo->TestSetIndexPath(textIndexPath);

        uint64_t firstTime;
        uint64_t dbIndexSize;
        uint64_t entries;
        {
            TestTimer t;
            t.Start();
            AudioDirectoryInfo::Ptr directoryInfo = AudioDirectoryInfo::Create(entry.path());
            directoryInfo->TestSetIndexPath(textIndexPath);
            entries = directoryInfo->GetFiles().size();
            firstTime = t.Stop();
            dbIndexSize = fs::file_size(textIndexPath);
        }
        uint64_t secondTime;
        {
            TestTimer t;
            t.Start();
            AudioDirectoryInfo::Ptr directoryInfo = AudioDirectoryInfo::Create(entry.path());
            directoryInfo->TestSetIndexPath(textIndexPath);            
            directoryInfo->GetFiles();
            secondTime = t.Stop();
        }
        cout << entry.path().string() << endl;
        cout << "    " << "first time: " << firstTime << " second time: " << secondTime << endl;
        cout << "    " << "size: " << dbIndexSize << " entries: " << entries << endl;
    }

}
TEST_CASE("Search for audio files with multple thumbnails", "[ScanThumbnails]")
{
    AudioDirectoryInfo::SetTemporaryDirectory(tempDir);
    AudioDirectoryInfo::SetResourceDirectory(resourceDir);
    ScanThumbnails(); // scan all files in music directory.
    ProfileMusicDirectory(); // Scans all folders in music directory.

    REQUIRE(true); // Just to ensure the test runs without failure
}

TEST_CASE("Thumbnail performance test", "[ProfileThumbnails]" )
{
    AudioDirectoryInfo::SetTemporaryDirectory(tempDir);
    AudioDirectoryInfo::SetResourceDirectory(resourceDir);
    ProfileMusicDirectory(); // Scans all folders in music directory.

    REQUIRE(true); // Just to ensure the test runs without failure
}
