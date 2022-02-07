#include "stdlib.h"
#include  <mutex>
#ifdef DEBUG


static std::mutex memMutex;

struct MemStats {
    size_t allocated = 0;
    size_t allocations = 0;
};

MemStats memStats;

void *operator new(size_t size)
{
    { 
        std::lock_guard lock(memMutex);
        memStats.allocated += size;
        memStats.allocations++;
    }

    char *p = (char*)malloc(size+8);
    ((size_t*)p)[0] = size;
    return p+8;
}

void operator delete(void*mem)
{
    char *p = ((char*)mem);
    size_t size = *(size_t*)(p-8);

    {
        std::lock_guard lock(memMutex);
        memStats.allocated -= size;
        memStats.allocations--;
    }


    free(p-8);
}

void* operator new[](size_t size)
{
    {
        std::lock_guard lock(memMutex);
        memStats.allocated += size;
        memStats.allocations++;
    }

    char *p = (char*)malloc(size+8);
    ((size_t*)p)[0] = size;
    return p+8;
}

void operator delete[](void*mem)
{
    char *p = ((char*)mem);
    size_t size = *(size_t*)(p-8);
    {
        std::lock_guard lock(memMutex);
        memStats.allocated -= size;
        memStats.allocations--;
    }

    free(p-8);
}

#endif