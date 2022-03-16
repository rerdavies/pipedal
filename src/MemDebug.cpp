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

#include "stdlib.h"
#include  <mutex>
#include "MemDebug.hpp"
#include <exception>

using namespace pipedal;

static std::mutex memMutex;



static MemStats memStats;

const MemStats&pipedal::GetMemStats()
{
    return memStats;
}

const size_t MEM_GUARD = (size_t)0xBAADF00DBAADF00D;

struct MemHeader {
    size_t size = 0;
    MemHeader*pNext = nullptr;
    MemHeader *pPrev = nullptr;
    size_t mem_guard = MEM_GUARD;

};

static MemHeader memHead;

void *operator new(size_t size)
{
    char *p = (char*)malloc(size+32+4);
    { 
        std::lock_guard lock(memMutex);
        MemHeader *pHead = (MemHeader*)p;
        pHead->size = size;
        pHead->pNext = nullptr;
        pHead->pPrev = nullptr;
        pHead->mem_guard = MEM_GUARD;
        memStats.allocated += size;
        memStats.allocations++;

        // link block into allocation chain
        if (memHead.pNext == nullptr)
        {
            memHead.pNext = memHead.pPrev = pHead;
            pHead->pNext = pHead->pPrev = &memHead;
        } else {
            pHead->pNext = memHead.pNext;
            pHead->pNext->pPrev = pHead;
            pHead->pPrev = &memHead;
            memHead.pNext = pHead;
        }

        p = p+32;
        char *endGuard = p + size;
        *endGuard++ = (char)0xBA;
        *endGuard++ = (char)0xAD;
        *endGuard++ = (char)0xF0;
        *endGuard++ = (char)0x0D;


    }
    return p;
}

static void bad_alloc();

void operator delete(void*mem)
{
    if (mem == nullptr) return;

    char *p = ((char*)mem);

    {
        MemHeader *pHeader = (MemHeader*)(p-32);
        std::lock_guard lock(memMutex);
        memStats.allocated -= pHeader->size;
        memStats.allocations--;

        // check the memory guards.
        if (pHeader->mem_guard != MEM_GUARD)
        {
            throw std::bad_alloc();
        }
        char *pTail = p + pHeader->size;
        if (*pTail++ != (char)0xBA) {
            throw std::bad_alloc();
        }
        if (*pTail++ != (char)0xAD) {
            throw std::bad_alloc();
        }
        if (*pTail++ != (char)0xF0) {
            throw std::bad_alloc();
        }
        if (*pTail++ != (char)0x0D) {
            throw std::bad_alloc();
        }
        // invalidate the memory guards (double free, eg).
        pHeader->mem_guard = (size_t)0xDDDDDDDD;
        pTail = p + pHeader->size;
        *pTail++ = (char)0xDD;
        *pTail++ = (char)0xDD;
        *pTail++ = (char)0xDD;
        *pTail++ = (char)0xDD;

        //  unlink the block from the allocation chain.
        pHeader->pNext->pPrev = pHeader->pPrev;
        pHeader->pPrev->pNext = pHeader->pNext;
        pHeader->pNext = nullptr;
        pHeader->pPrev = nullptr;

    }
    free(p-32);


}

void* operator new[](size_t size)
{
    return operator new(size);
}

void operator delete[](void*mem)
{
    return operator delete(mem);
}

