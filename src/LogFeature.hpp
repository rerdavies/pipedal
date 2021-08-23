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

#include "lv2/core/lv2.h"

#include "lv2/atom/atom.h"
#include "lv2/atom/util.h"
#include "lv2/core/lv2.h"
#include "lv2/log/log.h"
#include "lv2/log/logger.h"
#include "lv2/midi/midi.h"
#include "lv2/urid/urid.h"
#include "lv2/atom/atom.h"
#include "MapFeature.hpp"
#include <map>
#include <string>
#include <mutex>


namespace pipedal {
	class LogFeature {

	private:
		LV2_URID nextAtom = 0;
		LV2_Feature feature;
		LV2_Log_Log log;
		std::mutex logMutex;
		struct Uri {
			void Map(MapFeature* map)
			{
				ridError = map->GetUrid(LV2_LOG__Error);
				ridNote = map->GetUrid(LV2_LOG__Note);
				ridTrace = map->GetUrid(LV2_LOG__Trace);
				ridWarning = map->GetUrid(LV2_LOG__Warning);
			}
			LV2_URID ridError;
			LV2_URID ridWarning;
			LV2_URID ridNote;
			LV2_URID ridTrace;
		};

		Uri uris;

	public:
		LogFeature();
		void Prepare(MapFeature* map);

	public:
		const LV2_Feature* GetFeature()
		{
			return &feature;
		}
		LV2_URID GetUrid(const char* uri);
	private:
		static int printfFn(LV2_Log_Handle handle, LV2_URID type, const char* fmt, ...);

		static int vprintfFn(LV2_Log_Handle handle,
			LV2_URID       type,
			const char* fmt,
			va_list        ap);

		int vprintf(LV2_URID type, const char* fmt, va_list va);


	};
}