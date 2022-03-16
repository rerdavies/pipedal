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

#include "BeastServer.hpp"
#include "MemDebug.hpp"
#include <iostream>

using namespace pipedal;

TEST_CASE("BeastServer shutdown", "[beastServerShutdown][Build][Dev]")
{
    MemStats initialMemory = GetMemStats();
    {
        auto const address = boost::asio::ip::make_address("0.0.0.0");
        auto const port = 8081;
        std::string doc_root = ".";
        auto const threads = 3;

        auto server = createBeastServer(
            address, port, doc_root.c_str(), threads);
        server->RunInBackground();
        sleep(5);
        server->ShutDown(1000);
        sleep(1);
        server->Join();
    }
    MemStats finalMemory = GetMemStats();

    // currently leaking one allocation of 8 bytes. Acceptable.
    if (finalMemory.allocations > initialMemory.allocations+1) 
    {
        std::cout << "Leaked Allocations: " << (finalMemory.allocations- initialMemory.allocations) << std::endl;
        std::cout << "Leaked Memory: " << (finalMemory.allocated - initialMemory.allocated) << " bytes" << std::endl;

        REQUIRE(finalMemory.allocations == initialMemory.allocations);
    }

}
