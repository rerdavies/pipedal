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

#include "MapPathFeature.hpp"
#include <string.h>
#include <algorithm>
#include "json.hpp"
#include <sstream>


using namespace pipedal;

MapPathFeature::MapPathFeature(const std::filesystem::path &storagePath)
:storagePath(storagePath)
{
    lv2_state_map_path.handle = (LV2_State_Map_Path_Handle *)this;
    lv2_state_map_path.absolute_path = FnAbsolutePath;
    lv2_state_map_path.abstract_path = FnAbstractPath;

    lv2_state_make_path.handle = (LV2_State_Make_Path_Handle*)this;
    lv2_state_make_path.path = FnAbsolutePath;

    lv2_state_free_path.handle = (LV2_State_Free_Path_Handle*)this;
    lv2_state_free_path.free_path = FnFreePath;

    mapPathFeature.URI = LV2_STATE__mapPath;
    mapPathFeature.data = (void*)&lv2_state_map_path;
    makePathFeature.URI = LV2_STATE__makePath;
    makePathFeature.data = (void*)&lv2_state_make_path;
    freePathFeature.URI = LV2_STATE__freePath;
    freePathFeature.data = (void*)&lv2_state_free_path;
}

void MapPathFeature::Prepare(MapFeature* map)
{

}

void MapPathFeature::FreePath(char *path)
{
    free(path);
}

void MapPathFeature::FnFreePath(LV2_State_Free_Path_Handle handle, char* path)
{
    ((MapPathFeature*)handle)->FreePath(path);
}


/*static*/ char *MapPathFeature::FnAbsolutePath(
    LV2_State_Map_Path_Handle handle,
    const char *abstract_path)
{
    return ((MapPathFeature *)handle)->AbsolutePath(abstract_path);
}

char *MapPathFeature::AbsolutePath(const char *abstract_path)
{
    if (strlen(abstract_path) == 0)
    {
        return strdup("");
    }
    std::filesystem::path t (abstract_path);
    if (t.is_absolute()) {
        return strdup(abstract_path);
    }
    std::filesystem::path result = storagePath / t;
    return strdup(result.c_str());
}

/*static*/
char *MapPathFeature::FnAbstractPath(
    LV2_State_Map_Path_Handle handle,
    const char *absolute_path)
{
    return ((MapPathFeature*)handle)->AbstractPath(absolute_path);
}

char *MapPathFeature::AbstractPath(const char *absolute_path)
{
    if (strlen(absolute_path) == 0)
    {
        return strdup("");
    }
    if (strncmp(storagePath.c_str(),absolute_path,storagePath.length()) == 0)
    {
        const char*result = absolute_path + storagePath.length();
        if (*result == '/')
        {
            ++result;
            return strdup(result);
        }
        if (*result == '0') {
            return strdup(result);
        }
    }
    return strdup(absolute_path);
}

