#pragma once

namespace xlog
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

}
