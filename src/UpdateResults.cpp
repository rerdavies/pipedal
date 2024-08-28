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

#include "UpdateResults.hpp"
#include <filesystem>
#include <fstream>
#include "Lv2Log.hpp"
#include "ss.hpp"



using namespace pipedal;
namespace fs = std::filesystem;

const fs::path UPDATE_RESULT_PATH{"/var/pipedal/config"};
void UpdateResults::Load()
{
    std::ifstream f(UPDATE_RESULT_PATH);
    if (f.is_open())
    {
        json_reader reader(f);
        reader.read(this);
    }
}
void UpdateResults::Save()
{
    std::ofstream f {UPDATE_RESULT_PATH};
    if (f.is_open())
    {
        json_writer writer(f);
        writer.write(this);
    } else {
        Lv2Log::error(SS("Can't write to " << UPDATE_RESULT_PATH));
    }
}


JSON_MAP_BEGIN(UpdateResults)
JSON_MAP_REFERENCE(UpdateResults, updated)
JSON_MAP_REFERENCE(UpdateResults, updateSuccessful)
JSON_MAP_REFERENCE(UpdateResults, updateVersion)
JSON_MAP_REFERENCE(UpdateResults, updateMessage)
JSON_MAP_REFERENCE(UpdateResults, installerLog)
JSON_MAP_END()
