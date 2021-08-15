#include "LogFeature.hpp"
#include <mutex>
#include <cstdio>
#include "Lv2Log.hpp"


using namespace pipedal;
using namespace std;



int LogFeature::printfFn(LV2_Log_Handle handle, LV2_URID type, const char* fmt, ...)
{
	va_list va;
	va_start(va, fmt);

	LogFeature* logFeature = (LogFeature*)handle;
	return logFeature->vprintf(type, fmt, va);
}

int LogFeature::vprintfFn(LV2_Log_Handle handle,
	LV2_URID       type,
	const char* fmt,
	va_list        ap)
{
	LogFeature* logFeature = (LogFeature*)handle;
	return logFeature->vprintf(type, fmt, ap);
}

int LogFeature::vprintf(LV2_URID type,const char*fmt, va_list va)
{
	std::lock_guard<std::mutex> guard(logMutex);

	const char* prefix = "";
	char buffer[1024];
	int result = vsnprintf(buffer, sizeof(buffer), fmt, va);

	if (type == uris.ridError)
	{
		Lv2Log::error(buffer);
	}
	else if (type == uris.ridWarning)
	{
		Lv2Log::warning(buffer);
	}
	else if (type == uris.ridNote)
	{
		Lv2Log::info(buffer);
	}
	else if (type == uris.ridTrace)
	{
        Lv2Log::debug(buffer);
	}
    else {
        Lv2Log::info(buffer);
    }
    return result;
}


LogFeature::LogFeature()
{
	feature.URI = LV2_LOG__log;
	feature.data = &log;
	log.handle = (void*)this;
	log.printf = printfFn;
	log.vprintf = vprintfFn;
}
void LogFeature::Prepare(MapFeature*map)
{
	uris.Map(map);

}

