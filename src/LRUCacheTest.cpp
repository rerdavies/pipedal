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
#include "LRUCache.hpp"
#include <string>
#include <iostream>

using namespace std;

TEST_CASE( "LRUCache test", "[lrucache]" ) {

    LRUCache<int,int> lruCache(5);

    for (int i = 0; i < 10; ++i)
    {
        lruCache.put(i,i);
    }
    REQUIRE(lruCache.cache().size() == 5);

    REQUIRE(lruCache.cache().begin()->value == 9);

    int value;
    REQUIRE(lruCache.get(5,value) == true);
    REQUIRE(value == 5);
    REQUIRE(lruCache.cache().begin()->value == 5);

    cout << '[';
    for (auto it = lruCache.cache().begin();it != lruCache.cache().end(); ++it)
    {
        cout << it->value << ',';
    }
    cout << ']' << endl;
    for (auto it = ++lruCache.cache().begin();it != lruCache.cache().end(); ++it)
    {
        REQUIRE(it->value != 5);
    }
    lruCache.put(1,1);
    cout << '[';
    for (auto it = lruCache.cache().begin();it != lruCache.cache().end(); ++it)
    {
        cout << it->value << ',';
    }
    cout << ']' << endl;





}