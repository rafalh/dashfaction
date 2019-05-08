#pragma once

namespace logging
{

class NullStream
{
public:
    template<typename T>
    NullStream& operator<<(const T&)
    {
        return *this;
    }
};

} // namespace logging
