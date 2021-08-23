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

#include "pch.h"


class TargetClass
{
public:
    void read(int*t) { }
    void read(double*t) { }
    void read2(double*t) { }
};

#ifdef JUNK

template <typename T>
class HasJsonRead {


    template <typename TYPE, typename ARG>
    static std::true_type test (decltype(TargetClass().read((ARG*)nullptr))*v) { return std::true_type();};

    template <typename TYPE, typename ARG>
    static std::false_type test (...);

public:
    static constexpr bool value = decltype(test<TargetClass,T>(nullptr))::value;
};

class X{
    static_assert(HasJsonRead<float>::value,"TEST FAILED.");
};

#endif