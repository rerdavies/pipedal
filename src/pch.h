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

#ifndef SUPPORT_MIDI  // currently, only whether midi plugins can be loaded. No routing or handling implemented (yet).
#define SUPPORT_MIDI 0
#endif


#include <mutex>
#include <string>
#include <vector>
#include <string_view>
#include <stdexcept>
#include <sstream>
#include <cstdlib>
#include <map>

/*
#include <lv2/lv2core.lv2/lv2.h>

#include "lv2/atom.lv2/atom.h"
#include "lv2/atom.lv2/util.h"
#include "lv2/log.lv2/log.h"
#include "lv2/log.lv2/logger.h"
#include "lv2/midi.lv2/midi.h"
#include "lv2/urid.lv2/urid.h"
#include "lv2/log.lv2/logger.h"
#include "lv2/uri-map.lv2/uri-map.h"
#include "lv2/atom.lv2/forge.h"
#include "lv2/worker.lv2/worker.h"
#include "lv2/patch.lv2/patch.h"
#include "lv2/parameters.lv2/parameters.h"
#include "lv2/units.lv2/units.h"
*/

#define SS(x) ( ((std::stringstream&)(std::stringstream() << x )).str())
