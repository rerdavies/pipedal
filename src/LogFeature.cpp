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

