//-----------------------------------------------------------------------------
// Project     : SDK Core
//
// Category    : Common Classes
// Filename    : public.sdk/source/common/memorystream.cpp
// Created by  : Steinberg, 03/2008
// Description : IBStream Implementation for memory blocks
//
//-----------------------------------------------------------------------------
// LICENSE
// (c) 2022, Steinberg Media Technologies GmbH, All Rights Reserved
//-----------------------------------------------------------------------------
// Redistribution and use in source and binary forms, with or without modification,
// are permitted provided that the following conditions are met:
// 
//   * Redistributions of source code must retain the above copyright notice, 
//     this list of conditions and the following disclaimer.
//   * Redistributions in binary form must reproduce the above copyright notice,
//     this list of conditions and the following disclaimer in the documentation 
//     and/or other materials provided with the distribution.
//   * Neither the name of the Steinberg Media Technologies nor the names of its
//     contributors may be used to endorse or promote products derived from this 
//     software without specific prior written permission.
// 
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
// ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED 
// WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. 
// IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, 
// INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, 
// BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, 
// DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF 
// LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE 
// OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE  OF THIS SOFTWARE, EVEN IF ADVISED
// OF THE POSSIBILITY OF SUCH DAMAGE.
//-----------------------------------------------------------------------------

#include "vst3/Vst3RtStream.hpp"
#include "pluginterfaces/base/futils.h"
#include <stdlib.h>

namespace pipedal {

//-----------------------------------------------------------------------------
IMPLEMENT_FUNKNOWN_METHODS (RtStream, IBStream, IBStream::iid)
static const TSize kMemGrowAmount = 4096;

//-----------------------------------------------------------------------------

//-----------------------------------------------------------------------------
RtStream::RtStream(RtStreamPool*pool)
: pool(pool)
, memory (nullptr)
, memorySize (0)
, size (0)
, cursor (0)
, ownMemory (true)
, allocationError (false)
{
	FUNKNOWN_CTOR
}

//-----------------------------------------------------------------------------
RtStream::~RtStream () 
{ 
	if (ownMemory && memory)
		::free (memory);

	FUNKNOWN_DTOR 
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API RtStream::read (void* data, int32 numBytes, int32* numBytesRead)
{
	if (memory == nullptr)
	{
		if (allocationError)
			return kOutOfMemory;
		numBytes = 0;
	}
	else
	{		
		// Does read exceed size ?
		if (cursor + numBytes > size)
		{
			int32 maxBytes = int32 (size - cursor);

			// Has length become zero or negative ?
			if (maxBytes <= 0) 
			{
				cursor = size;
				numBytes = 0;
			}
			else
				numBytes = maxBytes;
		}
		
		if (numBytes)
		{
			memcpy (data, &memory[cursor], static_cast<size_t> (numBytes));
			cursor += numBytes;
		}
	}

	if (numBytesRead)
		*numBytesRead = numBytes;

	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API RtStream::write (void* buffer, int32 numBytes, int32* numBytesWritten)
{
	if (allocationError)
		return kOutOfMemory;
	if (buffer == nullptr)
		return kInvalidArgument;

	// Does write exceed size ?
	TSize requiredSize = cursor + numBytes;
	if (requiredSize > size) 
	{		
		if (requiredSize > memorySize)
			setSize (requiredSize);
		else
			size = requiredSize;
	}
	
	// Copy data
	if (memory && cursor >= 0 && numBytes > 0)
	{
		memcpy (&memory[cursor], buffer, static_cast<size_t> (numBytes));
		// Update cursor
		cursor += numBytes;
	}
	else
		numBytes = 0;

	if (numBytesWritten)
		*numBytesWritten = numBytes;

	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API RtStream::seek (int64 pos, int32 mode, int64* result)
{
	switch (mode) 
	{
		case kIBSeekSet:
			cursor = pos;
			break;
		case kIBSeekCur:
			cursor = cursor + pos;
			break;
		case kIBSeekEnd:
			cursor = size + pos;
			break;
	}

	if (ownMemory == false)
		if (cursor > memorySize)
			cursor = memorySize;

	if (result)
		*result = cursor;

	return kResultTrue;
}

//-----------------------------------------------------------------------------
tresult PLUGIN_API RtStream::tell  (int64* pos)
{
	if (!pos)
		return kInvalidArgument;

	*pos = cursor;
	return kResultTrue;
}

//------------------------------------------------------------------------
TSize RtStream::getSize () const
{
	return size;
}

//------------------------------------------------------------------------
void RtStream::setSize (TSize s)
{
	if (s <= 0)
	{
		if (ownMemory && memory)
			free (memory);

		memory = nullptr;
		memorySize = 0;
		size = 0;
		cursor = 0;
		return;
	}

	TSize newMemorySize = (((Max (memorySize, s) - 1) / kMemGrowAmount) + 1) * kMemGrowAmount;
	if (newMemorySize == memorySize)
	{
		size = s;
		return;
	}

	if (memory && ownMemory == false)
	{
		allocationError = true;
		return;	
	}

	ownMemory = true;
	char* newMemory = nullptr;

	if (memory)
	{
		newMemory = (char*)realloc (memory, (size_t)newMemorySize);
		if (newMemory == nullptr && newMemorySize > 0)
		{
			newMemory = (char*)malloc ((size_t)newMemorySize);
			if (newMemory)
			{
				memcpy (newMemory, memory, (size_t)Min (newMemorySize, memorySize));           
				free (memory);
			}		
		}
	}
	else
		newMemory = (char*)malloc ((size_t)newMemorySize);

	if (newMemory == nullptr)
	{
		if (newMemorySize > 0)
			allocationError = true;

		memory = nullptr;
		memorySize = 0;
		size = 0;
		cursor = 0;
	}
	else
	{
		memory = newMemory;
		memorySize = newMemorySize;
		size = s;
	}
}

//------------------------------------------------------------------------
char* RtStream::getData () const
{
	return memory;
}

//------------------------------------------------------------------------
char* RtStream::detachData ()
{
	if (ownMemory)
	{
		char* result = memory;
		memory = nullptr;
		memorySize = 0;
		size = 0;
		cursor = 0;
		return result;
	}
	return nullptr;
}

//------------------------------------------------------------------------
bool RtStream::truncate ()
{
	if (ownMemory == false)
		return false;

	if (memorySize == size)
		return true;

	memorySize = size;
	
	if (memorySize == 0)
	{
		if (memory)
		{
			free (memory);
			memory = nullptr;
		}
	}
	else
	{
		if (memory)
		{
			char* newMemory = (char*)realloc (memory, (size_t)memorySize);
			if (newMemory)
				memory = newMemory;
		}
	}
	return true;
}

//------------------------------------------------------------------------
bool RtStream::truncateToCursor ()
{
	size = cursor;
	return truncate ();
}

} 

// namespace
/*
 * MIT License
 * 
 * Copyright (c) 2022 Robin E. R. Davies
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
 * of the Software, and to permit persons to whom the Software is furnished to do
 * so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "vst3/Vst3RtStream.hpp"

using namespace Steinberg;
using namespace pipedal;



void RtStreamPool::ReleaseStreams()
{
    RtStream *freeStreamList;
    {
        freeStreamList = this->freeStreamList;
        this->freeStreamList = nullptr;
    }
    while (freeStreamList)
    {
        RtStream *next = freeStreamList->next;

        freeStreamList->release();
        freeStreamList = next;
    }
}