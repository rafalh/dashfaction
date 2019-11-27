#pragma once

#include <windows.h>

class CodeBuffer
{
public:
    CodeBuffer(size_t len)
    {
        m_ptr = HeapAlloc(get_heap(), 0, len);
    }

    CodeBuffer(const CodeBuffer& other) = delete;

    ~CodeBuffer()
    {
        if (m_ptr) {
            HeapFree(get_heap(), 0, m_ptr);
        }
    }

    void* get() const
    {
        return m_ptr;
    }

    operator void*() const
    {
        return get();
    }

private:
    void* m_ptr;

    static HANDLE get_heap()
    {
        static auto heap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
        return heap;
    }
};
