#pragma once

template<typename IT, typename V>
inline bool IterableContains(IT& iterable, const V& value)
{
    return std::find(iterable.begin(), iterable.end(), value) != iterable.end();
}
