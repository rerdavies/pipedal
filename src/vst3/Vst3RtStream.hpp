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

#pragma once

#include "public.sdk/source/common/memorystream.h"
#include <mutex>
#include "Vst3RtStream.hpp"

namespace pipedal
{
    using namespace Steinberg;

    class IRtStream : public IBStream
    {
    public:
        virtual void QueueForRelease() = 0;
    };

    class RtStreamPool;

    class RtStream : public IRtStream
    {
        friend class RtStreamPool;

        virtual void PLUGIN_API QueueForRelease();
        RtStream(RtStreamPool *pool);

    public:
        virtual ~RtStream();
        //---IBStream---------------------------------------
        tresult PLUGIN_API read(void *buffer, int32 numBytes, int32 *numBytesRead) SMTG_OVERRIDE;
        tresult PLUGIN_API write(void *buffer, int32 numBytes, int32 *numBytesWritten) SMTG_OVERRIDE;
        tresult PLUGIN_API seek(int64 pos, int32 mode, int64 *result) SMTG_OVERRIDE;
        tresult PLUGIN_API tell(int64 *pos) SMTG_OVERRIDE;

        TSize getSize() const;    ///< returns the current memory size
        void setSize(TSize size); ///< set the memory size, a realloc will occur if memory already used
        char *getData() const;    ///< returns the memory pointer
        char *detachData();       ///< returns the memory pointer and give up ownership
        bool truncate();          ///< realloc to the current use memory size if needed
        bool truncateToCursor();  ///< truncate memory at current cursor position

        //------------------------------------------------------------------------
        DECLARE_FUNKNOWN_METHODS
    protected:
        RtStreamPool *pool = nullptr;
        RtStream *next = nullptr;
        char *memory;         // memory block
        TSize memorySize;     // size of the memory block
        TSize size;           // size of the stream
        int64 cursor;         // stream pointer
        bool ownMemory;       // stream has allocated memory itself
        bool allocationError; // stream invalid
    };

    class RtStreamPool
    {
    public:
        ~RtStreamPool()
        {
            ReleaseStreams();
        }
        IRtStream *AllocateBStream()
        {
            ReleaseStreams();
            return new RtStream(this);
        }
        void ReleaseStreams();

    private:
        friend class RtStream;

        RtStream *freeStreamList = nullptr;

        void QueueForRelease(RtStream *pStream)
        {
            pStream->next = freeStreamList;
            freeStreamList = pStream;
        }
    };

  
    inline void RtStream::QueueForRelease()
    {
        pool->QueueForRelease(this);
    }
};