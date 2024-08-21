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
#include <utility>

#include "json.hpp"
#include "json_variant.hpp"
#include <concepts>
#include <type_traits>
#include "inverting_mutex.hpp"
#include <iostream>
#include <thread>
#include <functional>
#include "util.hpp"

using namespace pipedal;
using namespace std;
using namespace std::chrono;

TEST_CASE("inverting_mutext test", "[inverting_mutex_test]")
{

    inverting_mutex mutex;

    {
        std::thread thread(
            [&mutex]() mutable
            {
                std::ignore = nice(5);
                {
                    std::lock_guard<inverting_mutex> lock(mutex);
                    SetThreadName("imTest");
                    cout << "Thread holds lock" << endl;
                    std::this_thread::sleep_for(3s);

                    std::this_thread::sleep_for(1s);
                    cout << "Thread releases lock" << endl;
                }
            });
        std::this_thread::sleep_for(std::chrono::seconds(1));

        {
            std::lock_guard<inverting_mutex> lock(mutex); // cause the thread to avoid priority-inversion.
            cout << "Main resumed." << endl;
        }
        {
            std::lock_guard<inverting_mutex> lock(mutex);
        }
        thread.join();
    }
}
