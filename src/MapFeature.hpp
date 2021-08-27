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

#include "lv2.h"

#include "lv2/lv2core.lv2/lv2.h"
#include "lv2/log.lv2/log.h"
#include "lv2/log.lv2/logger.h"
#include "lv2/midi.lv2/midi.h"
#include "lv2/urid.lv2/urid.h"
#include "lv2/atom.lv2/atom.h"
#include <map>
#include <string>
#include <mutex>


namespace pipedal {
	class MapFeature {

	private:
		LV2_URID nextAtom = 0;
		LV2_Feature mapFeature;
		LV2_Feature unmapFeature;
		LV2_URID_Map map;
		LV2_URID_Unmap unmap;
		std::map<std::string, LV2_URID> stdMap;
		std::map<LV2_URID, std::string*> stdUnmap;
		std::mutex mapMutex;


	public:
		MapFeature();
        ~MapFeature();
	public:
		const LV2_Feature* GetMapFeature()
		{
			return &mapFeature;
		}
		const LV2_Feature* GetUnmapFeature()
		{
			return &unmapFeature;
		}
		LV2_URID GetUrid(const char* uri);

        const char*UridToString(LV2_URID urid);

        const LV2_URID_Map *GetMap() const { return &map;}
        LV2_URID_Map *GetMap() { return &map;}

	};
}
