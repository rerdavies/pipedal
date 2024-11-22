// Copyright (c) 2023 Robin Davies
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
#include "lv2/state/state.h"
#include <filesystem>
#include "MapFeature.hpp"

namespace pipedal
{
    struct ResourceFileMapping {
        std::string resourcePath; // absolute path of the resource directory.
        std::string storagePath;  // absolute path of where resource directory files get placed.
    };
    class MapPathFeature
    {
    public:
        MapPathFeature();
		void Prepare(MapFeature* map);
        void SetPluginStoragePath(const std::string&path) { storagePath = path;}

        void AddResourceFileMapping(const ResourceFileMapping &resourceFileMapping)
        {
            this->resourceFileMappings.push_back(resourceFileMapping);
        }

        const std::vector<ResourceFileMapping> &GetResourceFileMappings() const { return resourceFileMappings; }
        const std::string&GetPluginStoragePath() const { return storagePath; }
        const LV2_Feature*GetMapPathFeature() { return &mapPathFeature;}
        const LV2_Feature*GetMakePathFeature() { return &makePathFeature;}
        const LV2_Feature*GetFreePathFeature() { return &freePathFeature;}
    private:
        char *AbsolutePath(const char *abstract_path);
        static char *FnAbsolutePath(LV2_State_Map_Path_Handle handle,
                                    const char *abstract_path);

        
        LV2_State_Map_Path lv2_state_map_path;

        LV2_State_Make_Path lv2_state_make_path;

        LV2_State_Free_Path lv2_state_free_path;

        void FreePath(char *path);
        static void FnFreePath(LV2_State_Free_Path_Handle handle, char* path);

        LV2_Feature mapPathFeature;
        LV2_Feature makePathFeature;
        LV2_Feature freePathFeature;

        char *AbstractPath(const char *abstract_path);
        static char * FnAbstractPath(
            LV2_State_Map_Path_Handle handle,
            const char *absolute_path);

    private:
        std::string storagePath;
        std::vector<ResourceFileMapping> resourceFileMappings;
    };
}