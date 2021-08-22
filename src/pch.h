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
