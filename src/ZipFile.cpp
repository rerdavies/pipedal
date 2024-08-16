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

using namespace pipedal;

ZipFile::ZipFile()
{

}
ZipFile::~ZipFile()
{

}

class ZipFileImpl: public ZipFile {
public:
    ZipFileImpl(const std::filesystem::path&path) 
    :path(path)
    {
        int errorOp = 0;
        zipFile = zip_open(path.c_str(),ZIP_RDONLY,&errorOp);
        if (zipFile == nullptr)
        {
            throw std::runtime_error("Can't open zip file.");
        }
    }
    virtual ~ZipFileImpl();
    virtual std::vector<std::string> GetFiles() override;
    virtual void ExtractTo(const std::string &zipName, const std::filesystem::path& path) override;

private:
    std::map<std::string,zip_int64_t> nameMap; // avoid o(2) extraction operations.
    const std::filesystem::path path;
    zip_t*zipFile = nullptr;
};

ZipFile::ptr ZipFile::Create(const std::filesystem::path &path)
{
    return std::shared_ptr<ZipFile>(new ZipFileImpl(path));
}
ZipFileImpl::~ZipFileImpl()
{
    if (zipFile)
    {
        zip_close(zipFile);
        zipFile = nullptr;
    }
}

std::vector<std::string> ZipFileImpl::GetFiles() 
{
    std::vector<std::string> result;
    zip_int64_t nEntries = zip_get_num_entries(zipFile,0);
    for (zip_int64_t i = 0; i < nEntries; ++i)
    {
        const char*name = zip_get_name(zipFile,i,ZIP_FL_ENC_STRICT);
        if (name)
        {
            result.push_back(name);
            nameMap[name] = i;
        }
    }
    return result;
}

void ZipFileImpl::ExtractTo(const std::string &zipName, const std::filesystem::path& path) {
    auto fi = nameMap.find(zipName);
    
    if (fi == nameMap.end())
    {
        // must call GetFiles() firest.
        throw std::runtime_error("Zip content file not found.");
    }
    zip_int64_t fileIndex = fi->second;

    zip_file_t*fIn = zip_fopen_index(this->zipFile,fileIndex,0);
    if (fIn == nullptr)
    {
        zip_error_t *error  = zip_get_error(this->zipFile);
        const char *strError = zip_error_strerror(error);

        throw std::runtime_error(SS("Failed to read zip content file. " << strError));
    }
    Finally t{[fIn] () mutable{
        zip_fclose(fIn);
    }};

    std::ofstream fo {path,std::ios_base::out |std::ios_base::trunc | std::ios_base::binary};

    constexpr int BUFFER_SIZE = 64*1024;
    std::vector<char> vBuff(BUFFER_SIZE);
    char*pBuff = (char*)&(vBuff[0]);
    while (true)
    {
        zip_int64_t nRead = zip_fread(fIn,pBuff, BUFFER_SIZE);
        if (nRead = 0) break;
        if (nRead == -1) {
            zip_error_t*error = zip_file_get_error(fIn);
            const char *strError = zip_error_strerror(error);
            throw std::runtime_error(SS("Error reading zip content file." << strError));
        }
        fo.write(pBuff,(std::streamsize)nRead);
        if (!fo) {
            throw std::runtime_error(SS("Unable to write to " << path));
        }
    }



}
