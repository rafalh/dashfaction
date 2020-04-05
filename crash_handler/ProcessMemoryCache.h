#pragma once

#include <windows.h>
#include <unordered_map>
#include <memory>
#include <cstddef>

class ProcessMemoryCache
{
    HANDLE m_process;
    std::unordered_map<size_t, std::unique_ptr<uint32_t[]>> m_page_cache;
    DWORD m_page_size;


public:
    ProcessMemoryCache(HANDLE process) : m_process(process)
    {
        SYSTEM_INFO sys_info;
        GetSystemInfo(&sys_info);
        m_page_size = sys_info.dwPageSize;
    }

    bool read(uintptr_t addr, void* buf, size_t size)
    {
        auto out_buf_bytes = reinterpret_cast<std::byte*>(buf);
        auto addr_uint = reinterpret_cast<uintptr_t>(addr);

        while (size > 0) {
            uintptr_t offset = addr_uint % m_page_size;
            uintptr_t page_addr = addr_uint - offset;
            std::byte* page_data = reinterpret_cast<std::byte*>(load_page_into_cache(page_addr));
            if (page_data == nullptr) {
                return false;
            }
            size_t bytes_to_copy = std::min<size_t>(size, m_page_size - offset);
            std::copy(page_data + offset, page_data + offset + bytes_to_copy, out_buf_bytes);
            size -= bytes_to_copy;
            addr_uint += bytes_to_copy;
        }
        return true;
    }

    void* load_page_into_cache(uintptr_t page_addr)
    {
        auto it = m_page_cache.find(page_addr);
        if (it == m_page_cache.end()) {
            SIZE_T bytes_read;
            auto page_buf = std::make_unique<uint32_t[]>(m_page_size / sizeof(uint32_t));
            bool read_success = ReadProcessMemory(m_process, reinterpret_cast<void*>(page_addr), page_buf.get(),
                m_page_size, &bytes_read);
            if (!read_success || !bytes_read) {
                return nullptr;
            }
            it = m_page_cache.insert(std::make_pair(page_addr, std::move(page_buf))).first;
        }
        return it->second.get();
    }
};
