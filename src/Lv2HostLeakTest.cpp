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

#include "pch.h"
#include "catch.hpp"
#include <sstream>
#include <cstdint>
#include <string>


#include "PluginHost.hpp"
#include "MemDebug.hpp"

using namespace pipedal;


TEST_CASE( "PluginHost memory leak", "[lv2host_leak][Build][Dev]" ) {

    MemStats initialMemory = GetMemStats();
    {
        PluginHost host;

        host.Load("/usr/lib/lv2:/usr/local/lib/lv2:/usr/modep/lv2");
    }
    MemStats finalMemory = GetMemStats();

    // Something in lilv leaks a Dublin Core url.
    // Acceptable.
    const int ACCEPTABLE_ALLOCATION_LEAKS = 6;
    const int ACCEPTABLE_MEMORY_LEAK = 400;

    if (finalMemory.allocations > initialMemory.allocations + ACCEPTABLE_ALLOCATION_LEAKS
        || finalMemory.allocated > initialMemory.allocated + ACCEPTABLE_MEMORY_LEAK) 
    {
        std::cout << "Leaked Allocations: " << (finalMemory.allocations- initialMemory.allocations) << std::endl;
        std::cout << "Leaked Memory: " << (finalMemory.allocated - initialMemory.allocated) << " bytes" << std::endl;

        REQUIRE(finalMemory.allocations == initialMemory.allocations);
    }
}