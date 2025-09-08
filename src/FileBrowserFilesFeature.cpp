/*
 * MIT License
 *
 * Copyright (c) 2023 Robin E. R. Davies
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */
#include "FileBrowserFilesFeature.hpp"
#include <vector>
#include <fstream>
#include <string>
#include <unordered_set>
#include <string.h>
#include <stdexcept>
#include "util.hpp"
#include "ofstream_synced.hpp"
#include "ModFileTypes.hpp"

using namespace pipedal;
using namespace pipedal::implementation;
namespace fs = std::filesystem;



uint32_t BrowserFilesVersionInfo::Version() const { return version; }
void BrowserFilesVersionInfo::Version(uint32_t value) { version = value; }

const std::vector<std::string> &BrowserFilesVersionInfo::InstalledFiles() const {
    return installedFiles;
}
bool BrowserFilesVersionInfo::IsInstalled(const std::string &file) const
{
    return installedFileMap.contains(file);
}
void BrowserFilesVersionInfo::AddFile(const std::string &file)
{
    if (!IsInstalled(file))
    {
        installedFileMap.insert(file);
        installedFiles.push_back(file);
    }
}

bool BrowserFilesVersionInfo::Load(const std::filesystem::path &path)
{
    std::ifstream f{path};
    if (!f.is_open())
        return false;
    f >> version;
    std::string str;
    std::getline(f, str);
    if (f.fail())
        return false;

    while (!f.eof())
    {
        std::getline(f, str);
        if (f.fail() || str.length() == 0)
            break;
        AddFile(str);
    }
    return true;
}

FileBrowserFilesFeature::FileBrowserFilesFeature()
{
    lv2_log_logger_init(&lv2Logger,nullptr,nullptr);
    feature.URI = LV2_FILEBROWSER__files;
    feature.data = &(this->featureData);
}
void BrowserFilesVersionInfo::Save(const std::filesystem::path &path)
{
    pipedal::ofstream_synced f{path};
    if (!f.is_open())
        return;
    f << version << std::endl;

    for (const std::string &file : installedFiles)
    {
        f << file << std::endl;
    }
}

void FileBrowserFilesFeature::LogError(const char*message)
{
    lv2_log_error(&lv2Logger,"FileBrowserFiles: %s\n",message);
}


void FileBrowserFilesFeature::Initialize(
    const LV2_URID_Map *lv2Map,
    const LV2_Log_Log *lv2Log,
    const std::string &bundleDirectory, 
    const std::string &browserDirectory)
{
    lv2_log_logger_init(&this->lv2Logger,const_cast<LV2_URID_Map *>(lv2Map),const_cast<LV2_Log_Log *>(lv2Log));

    this->bundleDirectory = bundleDirectory;
    this->browserRootDirectory = browserDirectory;
    this->privateDirectory = browserDirectory / this->bundleDirectory.filename();
    MakeDirectoryMap(browserDirectory);


    featureData.handle = (void *)this;
    featureData.free_path = &FN_free_path;
    featureData.get_upload_path = &FN_get_upload_path;
    featureData.map_path = &FN_map_path;
    featureData.publish_resource_files = &FN_publish_resource_files;
}

char *FileBrowserFilesFeature::FN_get_upload_path(LV2_FileBrowser_Files_Handle handle, const char *fileBrowserPath)
{
    return ((FileBrowserFilesFeature *)handle)->GetUploadPath(fileBrowserPath);
}
char *FileBrowserFilesFeature::FN_map_path(LV2_FileBrowser_Files_Handle handle, const char*path, const char*resourcePathBase,const char *fileBrowserPath)
{
    return ((FileBrowserFilesFeature *)handle)->MapPath(path,resourcePathBase,fileBrowserPath);
}
void FileBrowserFilesFeature::FN_free_path(LV2_FileBrowser_Files_Handle handle, char *path)
{
    ((FileBrowserFilesFeature *)handle)->FreePath(path);
}
LV2_FileBrowser_Status FileBrowserFilesFeature::FN_publish_resource_files(LV2_FileBrowser_Files_Handle handle, uint32_t version, const char *resourcePath, const char *uploadDirectory)
{
    return ((FileBrowserFilesFeature *)handle)->PublishResourceFiles(version, resourcePath, uploadDirectory);
}


const char*FileBrowserFilesFeature::GetWellKnownDirectory(const std::string&directory)
{
    if (strcmp(directory.c_str(), "~") == 0) {
        return privateDirectory.c_str();
    }
    const auto &f = g_wellKnownDirectoryMap->find(directory);
    if (f != g_wellKnownDirectoryMap->end())
    {
        return f->second.c_str();
    }
    return nullptr;
}


using WellKnownDirectoryMap = std::map<std::string,std::string>;
std::mutex FileBrowserFilesFeature::g_DirectoryMap_mutex;

std::unique_ptr<WellKnownDirectoryMap> FileBrowserFilesFeature::g_wellKnownDirectoryMap;

void FileBrowserFilesFeature::MakeDirectoryMap(const std::filesystem::path&rootBrowserDirectory)
{
    std::lock_guard lock { g_DirectoryMap_mutex};

    if (!g_wellKnownDirectoryMap)
    {
        g_wellKnownDirectoryMap = std::make_unique<WellKnownDirectoryMap>();
        for (const auto&modDirectory: ModFileTypes::ModDirectories())
        {
            (*g_wellKnownDirectoryMap)[modDirectory.modType] =
                (rootBrowserDirectory / modDirectory.pipedalPath).string();
        }

    }
}



char *FileBrowserFilesFeature::GetUploadPath(const char *fileBrowserPath)
{
    if (fileBrowserPath == nullptr | fileBrowserPath[0] == '\0')
    {
        return strdup("");
    }
    std::filesystem::path path {fileBrowserPath};

    auto iter = path.begin();
    if (iter == path.end()) 
    {
        return strdup("");
    }
    std::string firstSegment = iter->string();

    const auto f = (g_wellKnownDirectoryMap)->find(firstSegment);
    if (f == g_wellKnownDirectoryMap->end())
    {
        return nullptr;
    }

    fs::path result = f->second;

    ++iter;
    while (iter != path.end())
    {
        result /= (*iter);
        ++iter;
    }
    return strdup(result.c_str());
}
void FileBrowserFilesFeature::FreePath(char *path)
{
    free(path);
}

static std::string RelativePath(const std::filesystem::path&root, const std::filesystem::path&path)
{
    std::string strRoot = root.string();
    std::string strPath = path.string();
    if (!strPath.starts_with(strRoot))
    {
        throw std::logic_error("Path is not relative to root.");
    }
    size_t pos = strRoot.length();

    // if there's no separator, we need to skip the separator in the path.
    if (pos != 0) {
        if (pos < strPath.length()+1)
        {
            char c = strPath[pos];
            if (c == std::filesystem::path::preferred_separator)
            {
                ++pos;
            }
        }
    }
    return strPath.substr(pos);
}
static std::string GetPluginTag(const std::filesystem::path bundlePath)
{
    std::string directoryName = bundlePath.filename();
    if (directoryName.length() != 0) 
    {
        return directoryName;
    }
    return bundlePath.parent_path().filename();
}


char *FileBrowserFilesFeature::MapPath(const char *path,const char*resourcePathBase,const char*fileBrowserPath)
{
    try {
        if (strncmp(path,this->bundleDirectory.c_str(),strlen(this->bundleDirectory.c_str()) != 0))
        {
            return strdup(path);
        }
        std::filesystem::path resourceRoot { resourcePathBase};

        if (resourceRoot.is_relative())
        {
            resourceRoot = this->bundleDirectory / resourceRoot;
        }
        if (strncmp(path,resourceRoot.c_str(),strlen(resourceRoot.c_str())) != 0 ) {
            return strdup(path);
        }
        auto relativePath = RelativePath(resourceRoot,path);

        std::filesystem::path targetDirectory;
        if (fileBrowserPath == nullptr || fileBrowserPath[0] == '\0')
        {
            targetDirectory = GetPluginTag(this->bundleDirectory);
        } else {
            targetDirectory = fileBrowserPath;
        }
        targetDirectory = this->browserRootDirectory / targetDirectory;
        std::filesystem::create_directories(targetDirectory);

        std::filesystem::path targetPath = targetDirectory / relativePath;

        if (!std::filesystem::exists(targetPath))
        {
            std::filesystem::create_symlink(path,targetPath);
        }
        return strdup(targetPath.c_str());

    } catch (const std::exception&e)
    {
        LogError(e.what());
        return strdup("");
    }

}




// throws on error.
void FileBrowserFilesFeature::PublishRecursive(
    BrowserFilesVersionInfo &versionInfo,
    const std::filesystem::path &rootResourcePath,
    const std::filesystem::path &resourcePath,
    const std::filesystem::path &browserPath)
{
    for (const std::filesystem::directory_entry directoryEntry : std::filesystem::directory_iterator{resourcePath})
    {
        std::string fileName = directoryEntry.path().filename();
        if (directoryEntry.is_directory())
        {
            std::filesystem::path targetDirectory = browserPath / fileName;
            std::filesystem::create_directories(targetDirectory);
            PublishRecursive(
                versionInfo,
                rootResourcePath,
                directoryEntry.path(),
                targetDirectory);
        }
        else
        {
            std::filesystem::path sourceFile =
                resourcePath / fileName;
            std::filesystem::path targetFile =
                browserPath / fileName;
            std::string relativePath = RelativePath(rootResourcePath, sourceFile);
            if (!std::filesystem::exists(targetFile)) // only create link if there isn't one.
            {

                // If the file has been previously installed but no longer exists, the file
                // has been deleted by a user, so don't reinstall it.
                if (!versionInfo.IsInstalled(relativePath))
                {
                    std::filesystem::create_directories(browserPath);
                    std::filesystem::create_symlink(
                        sourceFile, targetFile);
                }
            }
            versionInfo.AddFile(relativePath);
        }
    }
}

LV2_FileBrowser_Status FileBrowserFilesFeature::PublishResourceFiles(uint32_t version, const char *resourcePath, const char *uploadDirectory)
{
    BrowserFilesVersionInfo versionInfo;

    static std::string VERSION_FILENAME_PREFIX = ".versionInfo.";

    // generate a filename that will allow multiple plugins to install files into the same directory.
    std::filesystem::path versionFileName = this->browserRootDirectory / uploadDirectory / 
        (VERSION_FILENAME_PREFIX + GetPluginTag(this->bundleDirectory)); 

    bool loaded = versionInfo.Load(versionFileName);
    if (loaded && versionInfo.Version() >= version)
    {
        return LV2_FileBrowser_Status::LV2_FileBrowser_Status_Success;
    }
    try {
        versionInfo.Version(version);

        std::filesystem::path sourcePath = resourcePath;
        if (sourcePath.is_relative())
        {
            sourcePath = this->bundleDirectory / resourcePath;
        }
        std::filesystem::path targetDirectory;
        if (uploadDirectory == nullptr || uploadDirectory[0] == '\0')
        {
            targetDirectory = GetPluginTag(this->bundleDirectory);
        } else {
            targetDirectory = uploadDirectory;
        }
        if (!targetDirectory.is_relative())
        {
            LogError("uploadDirectory must be relative.");
            return LV2_FileBrowser_Status::LV2_FileBrowser_Status_Err_InvalidParameter;
        }
        targetDirectory = this->browserRootDirectory / targetDirectory;
        PublishRecursive(versionInfo, sourcePath,sourcePath, this->browserRootDirectory / uploadDirectory);
        versionInfo.Save(versionFileName);  
    } catch (const std::exception & e)
    {
        LogError(e.what());
        return LV2_FileBrowser_Status::LV2_FileBrowser_Status_Err_Filesystem;
    }
    return LV2_FileBrowser_Status::LV2_FileBrowser_Status_Success;
}
