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