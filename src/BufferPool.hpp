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