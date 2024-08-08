#include <type_traits>

#define ENABLE_ENUM_FLAGS(enum_type)\
template<> struct enum_traits<enum_type> :\
    enum_traits<>::allow_bitops {};\


template<class T = void> struct enum_traits {};

template<> struct enum_traits<void> {
    struct _allow_bitops {
        static constexpr bool allow_bitops = true;
    };
    using allow_bitops = _allow_bitops;

    template<class T, class R = T>
    using t = typename std::enable_if<std::is_enum<T>::value and
        enum_traits<T>::allow_bitops, R>::type;

    template<class T>
    using u = typename std::underlying_type<T>::type;
};

template<class T>
constexpr enum_traits<>::t<T> operator~(T a) {
    return static_cast<T>(~static_cast<enum_traits<>::u<T>>(a));
}
template<class T>
constexpr enum_traits<>::t<T> operator|(T a, T b) {
    return static_cast<T>(
        static_cast<enum_traits<>::u<T>>(a) |
        static_cast<enum_traits<>::u<T>>(b));
}
template<class T>
constexpr enum_traits<>::t<T> operator&(T a, T b) {
    return static_cast<T>(
        static_cast<enum_traits<>::u<T>>(a) &
        static_cast<enum_traits<>::u<T>>(b));
}
template<class T>
constexpr enum_traits<>::t<T> operator^(T a, T b) {
    return static_cast<T>(
        static_cast<enum_traits<>::u<T>>(a) ^
        static_cast<enum_traits<>::u<T>>(b));
}
template<class T>
constexpr enum_traits<>::t<T, T&> operator|=(T& a, T b) {
    a = a | b;
    return a;
}
template<class T>
constexpr enum_traits<>::t<T, T&> operator&=(T& a, T b) {
    a = a & b;
    return a;
}
template<class T>
constexpr enum_traits<>::t<T, T&> operator^=(T& a, T b) {
    a = a ^ b;
    return a;
}