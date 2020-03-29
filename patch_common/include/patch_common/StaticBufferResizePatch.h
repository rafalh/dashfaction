#pragma once

#include <vector>
#include <memory>
#include <cstdlib>
#include <patch_common/MemUtils.h>
#include <log/Logger.h>

template<typename T>
class StaticBufferResizePatch
{
public:
    struct RefInfo
    {
        uintptr_t addr;
        int flags = 0;
    };

    enum RefFlags
    {
        end_ref = 1,
    };

private:
    uintptr_t buf_addr_;
    size_t old_num_elements_;
    size_t new_num_elements_;
    std::vector<RefInfo> refs_;
    std::unique_ptr<T[]> new_buf_;

public:

    StaticBufferResizePatch(uintptr_t buf_addr, size_t old_num_elements, size_t new_num_elements,
        std::vector<RefInfo>&& refs) :
        buf_addr_(buf_addr), old_num_elements_(old_num_elements),
        new_num_elements_(new_num_elements), refs_(refs)
    {}

    void Install()
    {
        constexpr int max_offset = 6;
        // use int array as backing buffer to get aligned starting address
        new_buf_ = std::make_unique<T[]>(new_num_elements_);
        for (auto& ref : refs_) {
            int offset;
            for (offset = 0; offset <= max_offset; ++offset) {
                auto addr = ref.addr + offset;
                auto ptr = reinterpret_cast<void*>(addr);
                uintptr_t remap_begin_addr;
                intptr_t remap_new_addr;
                if (ref.flags & end_ref) {
                    remap_begin_addr = buf_addr_ + sizeof(T) * old_num_elements_;
                    remap_new_addr = reinterpret_cast<intptr_t>(new_buf_.get()) + sizeof(T) * new_num_elements_ ;
                }
                else {
                    remap_begin_addr = buf_addr_;
                    remap_new_addr = reinterpret_cast<intptr_t>(new_buf_.get());
                }
                uintptr_t remap_end_addr = remap_begin_addr + sizeof(T);
                uintptr_t data;
                std::memcpy(&data, ptr, sizeof(data));
                if (data >= remap_begin_addr && data < remap_end_addr) {
                    data = (data - remap_begin_addr) + remap_new_addr;
                    WriteMemPtr(addr, reinterpret_cast<void*>(data));
                    break;
                }
            }
            if (offset == max_offset) {
                WARN("Invalid reference %x", ref.addr);
            }
        }
    }

    T* GetBuffer() const
    {
        return new_buf_.get();
    }
};
