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