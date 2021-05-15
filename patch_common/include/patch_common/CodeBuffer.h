#pragma once

#include <cstddef>

class CodeBuffer
{
public:
    CodeBuffer(size_t len);
    CodeBuffer(const CodeBuffer& other) = delete;
    ~CodeBuffer();

    [[nodiscard]] void* get() const
    {
        return m_ptr;
    }

    operator void*() const
    {
        return get();
    }

private:
    void* m_ptr;
};
