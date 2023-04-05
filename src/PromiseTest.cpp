// Copyright (c) 2023 Robin Davies
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

#include "Promise.hpp"
#include "catch.hpp"
#include <future>
#include <thread>
#include <iostream>

using namespace pipedal;

TEST_CASE("Promise", "[promise][Build][Dev]")
{
    {
        // synchronous then->get
        Promise<double> p1 = Promise<double>(
            [](ResolveFunction<double> resolve, RejectFunction reject)
            {
                resolve(1.0);
            });
        Promise<int> p2 = p1
                              .Then<int>([](double value, ResolveFunction<int> resolve, RejectFunction reject)
                                         { resolve((int)(value + 3)); });
        int result = p2.Get();
        REQUIRE(result == 4.0);
    }
    REQUIRE(PromiseInnerBase::allocationCount == 0);

    ////////////////////////
    {
        // synchronous resolve.
        double result = Promise<double>(
                            [](ResolveFunction<double> resolve, RejectFunction reject)
                            {
                                resolve(1.0);
                            })
                            .Get();
        REQUIRE(result == 1.0);
    }
    {
        // async resolve (Get)
        double result = Promise<double>(
                            [](ResolveFunction<double> resolve, RejectFunction reject)
                            {
                                auto _ = std::async(
                                    std::launch::async,
                                    [resolve]()
                                    {
                                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                                        resolve(2.0);
                                    });
                            })
                            .Get();
        REQUIRE(result == 2.0);
    }
    {
        // async resolve (Then)
        std::mutex mutex;
        std::condition_variable cv;
        bool ready = false;

        Promise<double>(
            [](ResolveFunction<double> resolve, RejectFunction reject)
            {
                auto _ = std::async(
                    std::launch::async,
                    [resolve]()
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        resolve(3.0);
                    });
            })
            .Then([&cv, &mutex, &ready](double result)
                  {
            REQUIRE(result == 3.0);  
            {          
                std::lock_guard lock { mutex};
                ready = true;
            }
            cv.notify_all(); })
            .Catch([](const std::string &message)
                   {
                       REQUIRE(true == false); // should not get here.
                   });
        while (true)
        {
            std::unique_lock lock{mutex};
            if (ready)
                break;
            cv.wait(lock);
        }
    }
    REQUIRE(PromiseInnerBase::allocationCount == 0);
    {
        // async resolve (Then, Then<int32_t>)

        std::mutex mutex;
        std::condition_variable cv;
        bool ready = false;

        Promise<double>(
            [](ResolveFunction<double> resolve, RejectFunction reject)
            {
                auto _ = std::async(
                    std::launch::async,
                    [resolve]()
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        resolve(3.0);
                    });
            })
            .Then<int32_t>([](double value, ResolveFunction<int32_t> resolve, RejectFunction reject)
                           {
                               REQUIRE(value == 3.0);
                               auto _ = std::async(
                                   std::launch::async,
                                   [resolve, value]()
                                   {
                                       std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                                       resolve((int32_t)(value + 10));
                                   });
                           })
            .Then([&cv, &mutex, &ready](int32_t result)
                  {
                REQUIRE(result == 13);  
                {          
                    std::lock_guard lock { mutex};
                    ready = true;
                }
                cv.notify_all(); });
        while (true)
        {
            std::unique_lock lock{mutex};
            if (ready)
                break;
            cv.wait(lock);
        }
    }
    REQUIRE(PromiseInnerBase::allocationCount == 0);

    {
        // synchronous then->get
        int result = Promise<double>(
                         [](ResolveFunction<double> resolve, RejectFunction reject)
                         {
                             resolve(1.0);
                         })
                         .Then<int>([](double value, ResolveFunction<int> resolve, RejectFunction reject)
                                    { resolve((int)(value + 3)); })
                         .Get();
        REQUIRE(result == 4.0);
    }
    REQUIRE(PromiseInnerBase::allocationCount == 0);

    {
        // synchronous reject
        Promise<double> promise = Promise<double>(
            [](ResolveFunction<double> resolve, RejectFunction reject)
            {
                reject("Rejected");
            });
        REQUIRE_THROWS(promise.Get());
    }
    REQUIRE(PromiseInnerBase::allocationCount == 0);
    {
        // async reject (Get)
        Promise<double> promise = Promise<double>(
            [](ResolveFunction<double> resolve, RejectFunction reject)
            {
                auto _ = std::async(
                    std::launch::async,
                    [reject]()
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(100));
                        reject("Rejected");
                    });
            });
        REQUIRE_THROWS(promise.Get());
    }
    REQUIRE(PromiseInnerBase::allocationCount == 0);
    {
        // synchronous then then<int>/reject->get
        Promise<int> promise = Promise<double>(
                                   [](ResolveFunction<double> resolve, RejectFunction reject)
                                   {
                                       resolve(1.0);
                                   })
                                   .Then<int>(
                                       [](double value, ResolveFunction<int> resolve, RejectFunction reject)
                                       {
                                           reject("reject");
                                       });
        REQUIRE_THROWS(promise.Get());
    }
    REQUIRE(PromiseInnerBase::allocationCount == 0);
    {
        // synchronous then reject->then<int>->get
        Promise<int> promise = Promise<double>(
                                   [](ResolveFunction<double> resolve, RejectFunction reject)
                                   {
                                       reject("reject");
                                   })
                                   .Then<int>([](double value, ResolveFunction<int> resolve, RejectFunction reject)
                                              { resolve((int)(value + 3)); });
        REQUIRE_THROWS(promise.Get());
    }
    REQUIRE(PromiseInnerBase::allocationCount == 0);

    {
        // async resolve (Catch)
        std::mutex mutex;
        std::condition_variable cv;
        bool ready = false;

        Promise<double>(
            [](ResolveFunction<double> resolve, RejectFunction reject)
            {
                auto _ = std::async(
                    std::launch::async,
                    [reject]()
                    {
                        std::this_thread::sleep_for(std::chrono::milliseconds(200));
                        reject("rejected");
                    });
            })
            .Then([&cv, &mutex, &ready](double result)
                  { REQUIRE(true == false); })
            .Catch([&cv, &mutex, &ready](const std::string &message)
                   {
            {
                std::lock_guard lock(mutex);
                ready = true;
            }
            cv.notify_all(); });
        while (true)
        {
            std::unique_lock lock{mutex};
            if (ready)
                break;
            cv.wait(lock);
        }
    }

    REQUIRE(PromiseInnerBase::allocationCount == 0);
}
