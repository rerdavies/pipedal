// Copyright (c) 2021 Robin Davies
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

// Diagnostic type used to report calculated type T in an error message.
template <typename T> class TypeDisplay;


#include <mutex>
#include <string>
#include <vector>
#include <string_view>
#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <map>

#include "lv2/core/lv2.h"

/*
#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2_util.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/urid/urid.h"
#include "lv2/log/logger.h"
#include "lv2/uri-map/uri-map.h"
#include "lv2/atom/forge.h"
#include "lv2/worker/worker.h"
#include "lv2/patch/patch.h"
#include "lv2/parameters/parameters.h"
#include "lv2/units/units.h"
#include <lv2/lv2plug.in/ns/ext/worker/worker.h>
*/

#define SS(x) ( ((std::stringstream&)(std::stringstream() << x )).str())
