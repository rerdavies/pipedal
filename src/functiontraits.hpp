#pragma once


template<typename R, typename... Args>
struct FunctionTraitsBase
{
   using RetType = R;
   using ArgTypes = std::tuple<Args...>;
   static constexpr std::size_t ArgCount = sizeof...(Args);
};

template<typename F> struct FunctionTraits;

template<typename R, typename... Args>
struct FunctionTraits<R(*)(Args...)>
    : FunctionTraitsBase<R, Args...>
{
  using Pointer = R(*)(Args...);
};

template<typename R>
struct FunctionTraits<R(*)()>
    : FunctionTraitsBase<R>
{
  using Pointer = R(*)();
};