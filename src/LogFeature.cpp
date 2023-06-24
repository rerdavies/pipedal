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

	int result = 0;
	if (this->logMessageListener)
	{
		const char* prefix = "";
		char buffer[1024];
		strcpy(buffer,messagePrefix.c_str());
		char *p = buffer+messagePrefix.length();


		result = vsnprintf(p, sizeof(buffer)-messagePrefix.length(), fmt, va);
		buffer[sizeof(buffer)-1] = '\0';
		
		// strip trailing \n
		size_t len = strlen(buffer);
		if (len != 0 && buffer[len-1] == '\n')
		{
			buffer[len-1] = '\0';
		}

		if (type == uris.ridError)
		{
			logMessageListener->OnLogError(buffer);
		}
		else if (type == uris.ridWarning)
		{
			logMessageListener->OnLogWarning(buffer);
		}
		else if (type == uris.ridNote)
		{
			logMessageListener->OnLogInfo(buffer);
		}
		else if (type == uris.ridTrace)
		{
			logMessageListener->OnLogDebug(buffer);
		}
		else {
			logMessageListener->OnLogInfo(buffer);
		}
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
void LogFeature::Prepare(MapFeature*map, const std::string &messagePrefix, LogMessageListener*listener)
{
	uris.Map(map);
	this->messagePrefix = messagePrefix;
	this->logMessageListener = listener;

}


void LogFeature::LogError(const char*fmt,...)
{
	va_list va;
	va_start(va, fmt);

	vprintf(uris.ridError,fmt,va);

}
void LogFeature::LogWarning(const char*fmt,...)
{
	va_list va;
	va_start(va, fmt);

	vprintf(uris.ridWarning,fmt,va);

}
void LogFeature::LogNote(const char*fmt,...)
{
	va_list va;
	va_start(va, fmt);

	vprintf(uris.ridNote,fmt,va);

}
void LogFeature::LogTrace(const char*fmt,...)
{
{
	va_list va;
	va_start(va, fmt);

	vprintf(uris.ridNote,fmt,va);

}

}
