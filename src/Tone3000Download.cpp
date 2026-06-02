/*
 *   Copyright (c) Robin E.R. Davies
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

#include "ss.hpp"
#include "Tone3000Download.hpp"
#include "Tone3000Throttler.hpp"
#include "TemporaryFile.hpp"
#include <array>
#include <ctime>
#include "util.hpp"
#include "Tone3000DownloadProgress.hpp"
#include <thread>
#include "HtmlHelper.hpp"
#include "Curl.hpp"
#include "PiPedalModel.hpp"

using namespace pipedal;

Tone3000Download::Tone3000Download(const std::string &json)
{
    std::istringstream ss(json);
    json_reader reader(ss);
    reader.read(this);
}

JSON_MAP_BEGIN(Tone3000Model)
JSON_MAP_REFERENCE(Tone3000Model, id)
JSON_MAP_REFERENCE(Tone3000Model, name)
JSON_MAP_REFERENCE(Tone3000Model, model_url)
JSON_MAP_REFERENCE(Tone3000Model, created_at)
JSON_MAP_REFERENCE(Tone3000Model, size)
JSON_MAP_REFERENCE(Tone3000Model, user_id)
JSON_MAP_END()

JSON_MAP_BEGIN(Tone3000User)
JSON_MAP_REFERENCE(Tone3000User, id)
JSON_MAP_REFERENCE(Tone3000User, username)
JSON_MAP_REFERENCE(Tone3000User, avatar_url)
JSON_MAP_REFERENCE(Tone3000User, url)
JSON_MAP_END()

JSON_MAP_BEGIN(Tone3000Download)
JSON_MAP_REFERENCE(Tone3000Download, id)
JSON_MAP_REFERENCE(Tone3000Download, user_id)
JSON_MAP_REFERENCE(Tone3000Download, title)
JSON_MAP_REFERENCE(Tone3000Download, description)
JSON_MAP_REFERENCE(Tone3000Download, created_at)
JSON_MAP_REFERENCE(Tone3000Download, updated_at)
JSON_MAP_REFERENCE(Tone3000Download, gear)
JSON_MAP_REFERENCE(Tone3000Download, images)
JSON_MAP_REFERENCE(Tone3000Download, is_public)
JSON_MAP_REFERENCE(Tone3000Download, links)
JSON_MAP_REFERENCE(Tone3000Download, platform)
JSON_MAP_REFERENCE(Tone3000Download, models_count)
JSON_MAP_REFERENCE(Tone3000Download, favorites_count)
JSON_MAP_REFERENCE(Tone3000Download, downloads_count)
JSON_MAP_REFERENCE(Tone3000Download, license)
JSON_MAP_REFERENCE(Tone3000Download, sizes)
JSON_MAP_REFERENCE(Tone3000Download, a1_models_count)
JSON_MAP_REFERENCE(Tone3000Download, a2_models_count)
JSON_MAP_REFERENCE(Tone3000Download, irs_count)
JSON_MAP_REFERENCE(Tone3000Download, custom_models_count)
JSON_MAP_REFERENCE(Tone3000Download, user)
JSON_MAP_REFERENCE(Tone3000Download, url)
JSON_MAP_REFERENCE(Tone3000Download, user)
JSON_MAP_REFERENCE(Tone3000Download, models)
JSON_MAP_END();
