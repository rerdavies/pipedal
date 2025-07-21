// Copyright (c) 2025 Robin Davies
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

#include "pch.h"
#include "catch.hpp"
#include <sstream>
#include <cstdint>
#include <string>
#include <iostream>
#include "PipewireInputStream.hpp"

#include "PiPedalAlsa.hpp"
#include <atomic>
#include <thread>

using namespace pipedal;
using namespace std;


TEST_CASE("PipeWire Input Stream Test", "[pipewire_input_stream]")
{

    auto stream = PipeWireInputStream::Create("PiPedalTest Stream", 48000, 2);
    REQUIRE(stream != nullptr);
    REQUIRE(!stream->IsActive());
    std::atomic<size_t> frameCount = 0;
    std::atomic<size_t> *pFrameCount = &frameCount;
    stream->Activate([pFrameCount] (
        const float*buffer,
        size_t size)
    {
        *pFrameCount += size;
    });

    size_t lastCount = frameCount;
    while (true) {
        size_t t = frameCount;
        if (lastCount != t) {
            lastCount = t;
            cout << "Frame count: " << lastCount << endl;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }
}

