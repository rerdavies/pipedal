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

#pragma once

#include "lv2ext/pipedal.lv2/ext/fileBrowser.h"
#include <filesystem>
#include <lv2/core/lv2.h>
#include <lv2/log/logger.h>
#include <mutex>
#include <filesystem>
#include <vector>
#include <string>
#include <unordered_set>

namespace pipedal
{
    // Private forward declaration
    namespace implementation
    {
        class BrowserFilesVersionInfo
        {
        public:
            bool Load(const std::filesystem::path &path);
            void Save(const std::filesystem::path &path);
            const std::vector<std::string> &InstalledFiles() const;
            bool IsInstalled(const std::string &file) const;
            void AddFile(const std::string &file);
            uint32_t Version() const;
            void Version(uint32_t value);

        private:
            uint32_t version = 0;
            std::vector<std::string> installedFiles;
            std::unordered_set<std::string> installedFileMap;
        };
    };

    class FileBrowserFilesFeature
    {
    public:
        FileBrowserFilesFeature();
        void Initialize(
            const LV2_URID_Map *lv2Map,
            const LV2_Log_Log *lv2Log,
            const std::string &bundleDirectory,
            const std::string &browserDirectory);
        const LV2_Feature *GetFeature() { return &feature; }

    private:
        static void MakeDirectoryMap(const std::filesystem::path &rootBrowserDirectory);

        using WellKnownDirectoryMap = std::map<std::string, std::string>;
        static std::mutex g_DirectoryMap_mutex;
        static std::unique_ptr<WellKnownDirectoryMap> g_wellKnownDirectoryMap;

        LV2_Feature feature;
        const LV2_URID_Map *lv2Map = nullptr;
        LV2_Log_Logger lv2Logger = {nullptr, 0, 0, 0, 0};

        std::filesystem::path bundleDirectory;
        std::filesystem::path browserRootDirectory;
        std::filesystem::path privateDirectory;
        LV2_FileBrowser_Files featureData;

        std::map<std::string, std::string> wellKnownPaths;

        static char *FN_get_upload_path(LV2_FileBrowser_Files_Handle handle, const char *fileBrowserDirectory);
        static char *FN_map_path(LV2_FileBrowser_Files_Handle handle, const char *path, const char *resourcePathBase, const char *fileBrowserDirectory);
        static void FN_free_path(LV2_FileBrowser_Files_Handle handle, char *path);
        static LV2_FileBrowser_Status FN_publish_resource_files(LV2_FileBrowser_Files_Handle handle, uint32_t version, const char *resourcePath, const char *uploadDirectory);

        const char *GetWellKnownDirectory(const std::string &directory);

        void LogError(const char *message);
        char *GetUploadPath(const char *fileBrowserDirectory);
        char *MapPath(const char *path, const char *resourcePathBase, const char *fileBrowserDirectory);
        void FreePath(char *path);
        LV2_FileBrowser_Status PublishResourceFiles(uint32_t version, const char *resourcePath, const char *uploadDirectory);

        void PublishRecursive(
            implementation::BrowserFilesVersionInfo &versionInfo,
            const std::filesystem::path &rootResourcePath,
            const std::filesystem::path &resourcePath,
            const std::filesystem::path &browserPath);
    };
}