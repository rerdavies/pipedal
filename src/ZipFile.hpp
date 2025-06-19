// Copyright (c) 2024 Robin Davies
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
#include <memory>
#include <string>
#include <vector>
#include <filesystem>
#include <iostream>
#include <zip.h>


namespace pipedal {

    class ZipFileFileReader {
    public:
    private:
        zip_file_t*file = nullptr;
    };
    
    class zip_file_input_stream_buf : public std::streambuf {
    private:
        zip_file_t* file;
        std::vector<char> buffer;
        static const size_t putback_size = 8;
    public:
        zip_file_input_stream_buf(zip_file_t* file, size_t buffer_size = 16384);
        ~zip_file_input_stream_buf();
    protected:
        virtual int_type underflow() override;
        virtual std::streamsize xsgetn(char* s, std::streamsize n) override;
    };
    class  zip_file_input_stream : public std::istream {
    private:
        zip_file_input_stream_buf buf;
    public:
        zip_file_input_stream(zip_file_t *file, size_t buff_size = 16384)
        : std::istream(nullptr), buf(file,buff_size)
        {
            rdbuf(&buf);

        }
    };
    class ZipFileReader {
    protected:
        ZipFileReader();

    public:
        using ptr = std::shared_ptr<ZipFileReader>;
        static ptr Create(const std::filesystem::path &path);
        ZipFileReader(const ZipFileReader&) = delete;
        ZipFileReader&operator=(const ZipFileReader&) = delete;
        virtual ~ZipFileReader();

        virtual const std::vector<std::string>& GetFiles() = 0;
        virtual void ExtractTo(const std::string &zipName, const std::filesystem::path& path) = 0;
        virtual bool CompareFiles(const std::string &zipName, const std::filesystem::path& path) = 0;
        virtual zip_file_input_stream GetFileInputStream(const std::string& filename,size_t bufferSize = 16*1024) = 0;
        virtual size_t GetFileSize(const std::string&filename) = 0;
        virtual bool FileExists(const std::string&fileName) const = 0;

    };

    class ZipFileWriter;

    
    class ZipFileWriter {
    protected:
        ZipFileWriter();

    public:
        using self = ZipFileWriter();
        using ptr = std::shared_ptr<ZipFileWriter>;
        static ptr Create(const std::filesystem::path &path);

        ZipFileWriter(const ZipFileReader&) = delete;
        ZipFileWriter&operator=(const ZipFileWriter&) = delete;
        virtual ~ZipFileWriter();

        virtual void Close() = 0;

        virtual void WriteFile(const std::string &filename, const std::filesystem::path&path) =  0;
        virtual void WriteFile(const std::string&filename, const void*buffer, size_t length) = 0;
    };
}