#pragma once

#include <algorithm>

template<typename IT, typename V>
inline bool iterable_contains(IT& iterable, const V& value)
{
    return std::find(iterable.begin(), iterable.end(), value) != iterable.end();
}
