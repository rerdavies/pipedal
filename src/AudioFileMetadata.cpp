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

#include "AudioFileMetadata.hpp"

using namespace pipedal;

JSON_MAP_BEGIN(AudioFileMetadata)
JSON_MAP_REFERENCE(AudioFileMetadata, duration)
JSON_MAP_REFERENCE(AudioFileMetadata, fileName)
JSON_MAP_REFERENCE(AudioFileMetadata, lastModified)

JSON_MAP_REFERENCE(AudioFileMetadata, title)
JSON_MAP_REFERENCE(AudioFileMetadata, track)
JSON_MAP_REFERENCE(AudioFileMetadata, album)

JSON_MAP_REFERENCE(AudioFileMetadata, thumbnailType)
JSON_MAP_REFERENCE(AudioFileMetadata, thumbnailFile)
JSON_MAP_REFERENCE(AudioFileMetadata, thumbnailLastModified)
JSON_MAP_REFERENCE(AudioFileMetadata, position)

JSON_MAP_END()
