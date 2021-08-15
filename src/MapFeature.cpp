#include "MapFeature.hpp"
#include <mutex>

using namespace pipedal;

static LV2_URID mapFn(LV2_URID_Map_Handle handle, const char* uri)
{
	MapFeature* feature = (MapFeature*)(void*)handle;
	return feature->GetUrid(uri);
}
static const char* unmapFn(LV2_URID_Map_Handle handle, LV2_URID urid)
{
	MapFeature* feature = (MapFeature*)(void*)handle;
	return feature->UridToString(urid);
}

MapFeature::MapFeature()
{
	mapFeature.URI = LV2_URID__map;
	mapFeature.data = &map;
	map.handle = (void*)this;
	map.map = &mapFn;

    unmapFeature.URI = LV2_URID__unmap;
    unmapFeature.data = &unmap;
    unmap.handle = (void*)this;
    unmap.unmap = &unmapFn;
}

MapFeature::~MapFeature()
{
    for (auto i = stdUnmap.begin(); i != stdUnmap.end(); ++i)
    {
        delete i->second;
    }
}

LV2_URID MapFeature::GetUrid(const char* uri)
{

	std::lock_guard<std::mutex> guard(mapMutex);

	LV2_URID result = stdMap[uri];
	if (result == 0)
	{
		stdMap[uri] = ++nextAtom;
		result = nextAtom;
        std::string *stringRef = new std::string(uri);
        stdUnmap[result] = stringRef;
	}
	return result;
}

const char*MapFeature::UridToString(LV2_URID urid)
{
	std::lock_guard<std::mutex> guard(mapMutex);

    std::string*pResult = stdUnmap[urid];
    if (pResult == nullptr) return nullptr;

    return pResult->c_str();

}
