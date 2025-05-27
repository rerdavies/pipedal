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

 

 using namespace pipedal;


static constexpr size_t INVALID_INDEX = std::numeric_limits<size_t>::max();

 namespace fs = std::filesystem;

 namespace {
    class DirectoryList {
    public:
        DirectoryList(const fs::path&path);
    
        const std::vector<std::filesystem::path> &files() {
            return files_;
        }
    private:

        std::vector<std::filesystem::path> files_;
    };
    
    class AudioFileInfoImpl: public AudioFileInfo {
    public:
        virtual ~AudioFileInfoImpl() { }
    protected:
        virtual std::string GetNextAudioFile(const std::string&path) override;
        virtual std::string GetPreviousAudioFile(const std::string&path) override;
    };
    /////////////////////

    DirectoryList::DirectoryList(const fs::path&path)
    {
        for (const auto&dirEntry: fs::directory_iterator(path))
        {
            if (dirEntry.is_regular_file())
            {
                fs::path t = dirEntry.path();
                if (!t.filename().string().starts_with('.'))
                {
                    files_.push_back(std::move(t));
                }
            }
        }
    }
    
    std::string AudioFileInfoImpl::GetNextAudioFile(const std::string&path_) {
        try
        {
            fs::path path {path_};
            if (fs::exists(path) && fs::is_regular_file(path))
            {
                DirectoryList directoryList(path.parent_path());
                size_t index = INVALID_INDEX;
                auto &files = directoryList.files();
                for (size_t i= 0; i < files.size(); ++i)
                {
                    if (files[i] == path)
                    {
                        size_t next = i;
                        if (next == files.size()-1)
                        {
                            next = 0;
                        } else {
                            ++next;
                        }
                        return files[next];
                    }
                }

            } 
        } catch (const std::exception&e)
        {

        }
        return "";

    }
    std::string AudioFileInfoImpl::GetPreviousAudioFile(const std::string&path_) 
    {
        try
        {
            fs::path path {path_};
            if (fs::exists(path) && fs::is_regular_file(path))
            {
                DirectoryList directoryList(path.parent_path());
                size_t index = INVALID_INDEX;
                auto &files = directoryList.files();
                for (size_t i= 0; i < files.size(); ++i)
                {
                    if (files[i] == path)
                    {
                        size_t previous = i;
                        if (previous == 0)
                        {
                            previous = files.size()-1;
                        } else {
                            --previous;
                        }
                        return files[previous];
                    }
                }

            } 
        } catch (const std::exception&e)
        {

        }
        return "";

    }

 }

 static AudioFileInfoImpl instance;

 AudioFileInfo&AudioFileInfo::GetInstance()
 {
    return instance;
 }