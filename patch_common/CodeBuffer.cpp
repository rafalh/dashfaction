#include <patch_common/CodeBuffer.h>
#include <windows.h>

static HANDLE get_code_heap()
{
    static HANDLE heap = HeapCreate(HEAP_CREATE_ENABLE_EXECUTE, 0, 0);
    return heap;
}

CodeBuffer::CodeBuffer(size_t len)
{
    m_ptr = HeapAlloc(get_code_heap(), 0, len);
}

CodeBuffer::~CodeBuffer()
{
    if (m_ptr) {
        HeapFree(get_code_heap(), 0, m_ptr);
    }
}

