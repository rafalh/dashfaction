#pragma once

// Based on:
// https://www.justsoftwaresolutions.co.uk/cplusplus/using-enum-classes-as-bitfields.html
// http://blog.bitwigglers.org/using-enum-classes-as-type-safe-bitmasks/

#include <type_traits>

template<typename Enum>
struct EnableEnumBitwiseOperators : std::false_type {};

template<typename Enum>
typename std::enable_if<EnableEnumBitwiseOperators<Enum>::value, Enum>::type
operator |(Enum lhs, Enum rhs)
{
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum> (
        static_cast<underlying>(lhs) |
        static_cast<underlying>(rhs)
    );
}

template<typename Enum>
typename std::enable_if<EnableEnumBitwiseOperators<Enum>::value, Enum>::type
operator &(Enum lhs, Enum rhs)
{
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<Enum> (
        static_cast<underlying>(lhs) &
        static_cast<underlying>(rhs)
    );
}

template<typename Enum>
typename std::enable_if<EnableEnumBitwiseOperators<Enum>::value, Enum>::type&
operator |=(Enum& lhs, Enum rhs)
{
    using underlying = typename std::underlying_type<Enum>::type;
    lhs = static_cast<Enum> (
        static_cast<underlying>(lhs) |
        static_cast<underlying>(rhs)
    );
    return lhs;
}

template<typename Enum>
typename std::enable_if<EnableEnumBitwiseOperators<Enum>::value, Enum>::type&
operator &=(Enum& lhs, Enum rhs)
{
    using underlying = typename std::underlying_type<Enum>::type;
    lhs = static_cast<Enum> (
        static_cast<underlying>(lhs) &
        static_cast<underlying>(rhs)
    );
    return lhs;
}

template<typename Enum>
typename std::enable_if<EnableEnumBitwiseOperators<Enum>::value, bool>::type operator!(Enum rhs)
{
    using underlying = typename std::underlying_type<Enum>::type;
    return static_cast<underlying>(rhs) == 0;
}
