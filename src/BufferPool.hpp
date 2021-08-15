#pragma once



#include <type_traits>
// maintains ownership of allocated buffers.

namespace pipedal {


class BufferPool {
    std::vector<void*> allocatedBuffers;

public: 
    ~BufferPool() {
        Clear();
    }
    template <typename TYPE>
    TYPE *AllocateBuffer(size_t size)
    {
        TYPE *result= new TYPE[size];
        for (int i = 0; i < size; ++i)
        {
            result[i] = 0;
        }
        allocatedBuffers.push_back(result);
        return result;
    }

    void Clear() {
        for (int i = 0; i < allocatedBuffers.size(); ++i)
        {
            delete[] (char*)allocatedBuffers[i];
        }
        allocatedBuffers.resize(0);
    }


};



} // namespace.