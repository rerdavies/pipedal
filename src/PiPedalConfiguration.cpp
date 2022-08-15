// Copyright (c) 2022 Robin Davies
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

#include "pch.h"
#include "PiPedalConfiguration.hpp"
#include "json.hpp"


using namespace pipedal;

JSON_MAP_BEGIN(PiPedalConfiguration)
JSON_MAP_REFERENCE(PiPedalConfiguration, lv2_path)
JSON_MAP_REFERENCE(PiPedalConfiguration, local_storage_path)
JSON_MAP_REFERENCE(PiPedalConfiguration, mlock)
JSON_MAP_REFERENCE(PiPedalConfiguration, reactServerAddresses)
JSON_MAP_REFERENCE(PiPedalConfiguration, socketServerAddress)
JSON_MAP_REFERENCE(PiPedalConfiguration, threads)
JSON_MAP_REFERENCE(PiPedalConfiguration, logLevel)
JSON_MAP_REFERENCE(PiPedalConfiguration, logHttpRequests)
JSON_MAP_REFERENCE(PiPedalConfiguration, maxUploadSize)
JSON_MAP_REFERENCE(PiPedalConfiguration, accessPointGateway)
JSON_MAP_REFERENCE(PiPedalConfiguration, accessPointServerAddress)
JSON_MAP_REFERENCE(PiPedalConfiguration, isVst3Enabled)
JSON_MAP_END()
