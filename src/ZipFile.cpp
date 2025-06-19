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

#include "ZipFile.hpp"
#include "zip.h"
#include <stdexcept>
#include <map>
#include "ss.hpp"
#include "Finally.hpp"
#include <cstring>

using namespace pipedal;

ZipFileReader::ZipFileReader()
{
}
ZipFileReader::~ZipFileReader()
{
}
ZipFileWriter::ZipFileWriter()
{
}
ZipFileWriter::~ZipFileWriter()
{
}

class ZipFileImpl : public ZipFileReader
{
public:
    ZipFileImpl(const std::filesystem::path &path)
        : path(path)
    {
        int errorOp = 0;
        zipFile = zip_open(path.c_str(), ZIP_RDONLY, &errorOp);
        if (zipFile == nullptr)
        {
            throw std::runtime_error("Can't open zip file.");
        }
        zip_int64_t nEntries = zip_get_num_entries(zipFile, 0);
        for (zip_int64_t i = 0; i < nEntries; ++i)
        {
            const char *name = zip_get_name(zipFile, i, ZIP_FL_ENC_STRICT);
            if (name)
            {
                files.push_back(name);
                nameMap[name] = i;
            }
        }
    }
    virtual ~ZipFileImpl();
    virtual const std::vector<std::string>& GetFiles() override;
    virtual bool CompareFiles(const std::string &zipName, const std::filesystem::path& path) override;
    virtual void ExtractTo(const std::string &zipName, const std::filesystem::path &path) override;
    virtual zip_file_input_stream GetFileInputStream(const std::string& filename,size_t bufferSize = 16*1024) override;
    virtual size_t GetFileSize(const std::string&filename) override;
    virtual bool FileExists(const std::string &zipName) const override;

private:
    std::vector<std::string> files;
    std::map<std::string, zip_int64_t> nameMap; // avoid o(2) extraction operations.
    const std::filesystem::path path;
    zip_t *zipFile = nullptr;
};

ZipFileReader::ptr ZipFileReader::Create(const std::filesystem::path &path)
{
    return std::shared_ptr<ZipFileReader>(new ZipFileImpl(path));
}
ZipFileImpl::~ZipFileImpl()
{
    if (zipFile)
    {
        zip_close(zipFile);
        zipFile = nullptr;
    }
}

const std::vector<std::string>& ZipFileImpl::GetFiles()
{
    return files;
}


bool ZipFileImpl::CompareFiles(const std::string &zipName, const std::filesystem::path &path)
{
    auto fi = nameMap.find(zipName);
    if (fi == nameMap.end())
    {
        // must call GetFiles() firest.
        throw std::runtime_error("Zip content file not found.");
    }
    zip_int64_t fileIndex = fi->second;

    zip_stat_t zip_stat;
    if (zip_stat_index(zipFile, fileIndex, 0, &zip_stat) < 0) {
        throw std::runtime_error(SS("Failed to get file stats: " << zip_strerror(zipFile)));
    }    

    auto targetSize = std::filesystem::file_size(path);
    if (targetSize != zip_stat.size)
    {
        return false;
    }

    zip_file_t *fIn = zip_fopen_index(this->zipFile, fileIndex, 0);
    if (fIn == nullptr)
    {
        zip_error_t *error = zip_get_error(this->zipFile);
        const char *strError = zip_error_strerror(error);

        throw std::runtime_error(SS("Failed to read zip content file. " << strError));
    }
    Finally t{[fIn]() mutable
              {
                  zip_fclose(fIn);
              }};

    FILE*fTarget = fopen(path.c_str(),"r");
    if (fTarget == nullptr) 
    {
        throw std::runtime_error(SS("Failed to read  file " << path << "."));
    }
    Finally ffTarget{[fTarget]() mutable {
       fclose(fTarget); 
    }};

    constexpr int BUFFER_SIZE = 16 * 1024;
    std::vector<char> vBuff(BUFFER_SIZE);
    std::vector<char> vBuff2(BUFFER_SIZE);
    char *pBuff = (char *)&(vBuff[0]);
    char *pBuff2 = (char *)&(vBuff2[0]);
    while (true)
    {
        zip_int64_t nRead = zip_fread(fIn, pBuff, BUFFER_SIZE);
        if (nRead == -1)
        {
            zip_error_t *error = zip_file_get_error(fIn);
            const char *strError = zip_error_strerror(error);
            throw std::runtime_error(SS("Error reading zip content file." << strError));
        }
        auto nTargetRead = fread(pBuff2,1,BUFFER_SIZE,fTarget);

        if ((zip_int64_t)nTargetRead != nRead)
        {
            return false;
        }
        if (nRead == 0) 
        {
            return true;
        }
        for (zip_int64_t i = 0; i < nRead; ++i)
        {
            if (pBuff2[i] != pBuff[i])
            {
                return false;
            }
        }
    }
}


void ZipFileImpl::ExtractTo(const std::string &zipName, const std::filesystem::path &path)
{
    auto fi = nameMap.find(zipName);

    if (fi == nameMap.end())
    {
        // must call GetFiles() firest.
        throw std::runtime_error("Zip content file not found.");
    }
    zip_int64_t fileIndex = fi->second;

    zip_file_t *fIn = zip_fopen_index(this->zipFile, fileIndex, 0);
    if (fIn == nullptr)
    {
        zip_error_t *error = zip_get_error(this->zipFile);
        const char *strError = zip_error_strerror(error);

        throw std::runtime_error(SS("Failed to read zip content file. " << strError));
    }
    Finally t{[fIn]() mutable
              {
                  zip_fclose(fIn);
              }};

    std::filesystem::create_directories(path.parent_path());
    std::ofstream fo{path, std::ios_base::out | std::ios_base::trunc | std::ios_base::binary};
    if (!fo.is_open())
    {
        throw std::runtime_error(SS("Unable to open " << path));
    }

    constexpr int BUFFER_SIZE = 64 * 1024;
    std::vector<char> vBuff(BUFFER_SIZE);
    char *pBuff = (char *)&(vBuff[0]);
    while (true)
    {
        zip_int64_t nRead = zip_fread(fIn, pBuff, BUFFER_SIZE);
        if (nRead == 0)
            break;
        if (nRead == -1)
        {
            zip_error_t *error = zip_file_get_error(fIn);
            const char *strError = zip_error_strerror(error);
            throw std::runtime_error(SS("Error reading zip content file." << strError));
        }
        fo.write(pBuff, (std::streamsize)nRead);
        if (!fo)
        {
            throw std::runtime_error(SS("Unable to write to " << path));
        }
    }
}

zip_file_input_stream_buf::zip_file_input_stream_buf(zip_file_t *file, size_t buffer_size)
    : file(file), buffer(buffer_size + putback_size)
{
    char *end = buffer.data() + buffer.size();
    setg(end, end, end);
}
zip_file_input_stream_buf::~zip_file_input_stream_buf()
{
    zip_fclose(file);
}

std::streamsize zip_file_input_stream_buf::xsgetn(char *s, std::streamsize n)
{
    std::streamsize num_copied = 0;
    while (n > 0)
    {
        if (gptr() >= egptr())
        {
            if (underflow() == traits_type::eof())
            {
                break;
            }
        }
        std::streamsize chunk = std::min(n, egptr() - gptr());
        std::memcpy(s, gptr(), chunk);
        s += chunk;
        n -= chunk;
        num_copied += chunk;
        gbump(chunk);
    }
    return num_copied;
}
zip_file_input_stream_buf::int_type zip_file_input_stream_buf::underflow()
{
    if (gptr() < egptr())
    {
        return traits_type::to_int_type(*gptr());
    }

    char *base = buffer.data();
    char *start = base;

    if (eback() != base)
    {
        std::memmove(base, egptr() - putback_size, putback_size);
        start += putback_size;
    }

    zip_int64_t n = zip_fread(file, start, (zip_uint64_t)(buffer.size() - (start - base)));
    if (n <= 0)
    {
        return traits_type::eof();
    }

    setg(base, start, start + n);
    return traits_type::to_int_type(*gptr());
}

zip_file_input_stream ZipFileImpl::GetFileInputStream(const std::string& filename, size_t bufferSize)
{
    zip_file_t *f = zip_fopen(zipFile,filename.c_str(),0);
    if (!f) {
        throw std::runtime_error(SS("Failed to open zip content file " << filename));
    }
    return zip_file_input_stream(f,bufferSize);
}

bool  ZipFileImpl::FileExists(const std::string&fileName) const
{
    return nameMap.find(fileName) != nameMap.end();
}
size_t ZipFileImpl::GetFileSize(const std::string&filename)
{
    zip_stat_t stat;
    if (zip_stat(zipFile,filename.c_str(),0,&stat) < 0)
    {
        throw std::runtime_error("File not found.");
    }
    if ((stat.valid & ZIP_STAT_SIZE) == 0)
    {
        throw std::runtime_error("Failed to get file size.");
    }
    return stat.size;
}

class ZipFileWriterImpl : public ZipFileWriter
{
public:
    ZipFileWriterImpl(const std::filesystem::path &path)
        : path(path)
    {
        int errorOp = 0;
        zipFile = zip_open(path.c_str(), ZIP_CREATE | ZIP_TRUNCATE, &errorOp);
        if (zipFile == nullptr)
        {
            throw std::runtime_error("Can't open zip file.");
        }
    }
    virtual ~ZipFileWriterImpl();

    virtual void Close() override; 

    virtual void WriteFile(const std::string &zipFilename,  const std::filesystem::path&sourceFilePath) override;
    virtual void WriteFile(const std::string&filename, const void*buffer, size_t length) override;
private:
    std::map<std::string, zip_int64_t> nameMap; // avoid o(2) extraction operations.
    const std::filesystem::path path;
    zip_t *zipFile = nullptr;
};

ZipFileWriter::ptr ZipFileWriter::Create(const std::filesystem::path &path)
{
    return std::make_unique<ZipFileWriterImpl>(path);
}

void ZipFileWriterImpl::Close() {
    if (zipFile) {
        zip_close(zipFile);
        zipFile = nullptr;
    }
}
ZipFileWriterImpl::~ZipFileWriterImpl()
{
    Close();
}

void ZipFileWriterImpl::WriteFile(const std::string &filename, const std::filesystem::path &path)
{
    zip_error_t error;
    zip_source_t *source = zip_source_file_create(path.c_str(),0,-1,&error);
    if (!source) {
        throw std::runtime_error(SS("Unable to create zip source for file " << path));
    }
    if (zip_file_add(zipFile,filename.c_str(),source,ZIP_FL_ENC_UTF_8) < 0)
    {
        zip_source_free(source);
        throw std::runtime_error(SS("Unable to create add file  " << path));
    }
}
void ZipFileWriterImpl::WriteFile(const std::string&filename, const void*buffer, size_t length) 
{
    zip_source_t *source = zip_source_buffer(zipFile,buffer,length,0);

    if (zip_file_add(zipFile,filename.c_str(),source,ZIP_FL_ENC_UTF_8) < 0)
    {
        zip_source_free(source);
        throw std::runtime_error(SS("Failed to add file to zip: " << zip_strerror(zipFile)));
    }
}
