#pragma once

#include <vector>
#include <memory>
#include <cstddef>
#include <cstring>
#include <patch_common/MemUtils.h>
#include <patch_common/Installable.h>
#include <xlog/xlog.h>

template<typename T>
class StaticBufferResizePatch : public Installable
{
public:
    struct RefInfo
    {
        // address of instruction referencing the buffer
        uintptr_t addr;
        // is the instruction referencing memory after the buffer
        bool is_end_ref = false;
    };

private:
    static constexpr size_t element_size = sizeof(T); // NOLINT(bugprone-sizeof-expression)

    uintptr_t buf_addr_;
    size_t old_num_elements_;
    size_t new_num_elements_;
    std::vector<RefInfo> refs_;
    std::unique_ptr<std::byte[]> new_buf_owned_;
    T* new_buf_;

public:

    StaticBufferResizePatch(uintptr_t buf_addr, size_t old_num_elements, size_t new_num_elements,
        std::vector<RefInfo>&& refs) :
        buf_addr_(buf_addr), old_num_elements_(old_num_elements),
        new_num_elements_(new_num_elements), refs_(refs), new_buf_(nullptr)
    {}

    template<size_t N>
    StaticBufferResizePatch(uintptr_t buf_addr, size_t old_num_elements, T (&new_buf)[N],
        std::vector<RefInfo>&& refs) :
        buf_addr_(buf_addr), old_num_elements_(old_num_elements),
        new_num_elements_(N), refs_(refs), new_buf_(new_buf)
    {}

    void install() override
    {
        constexpr int max_offset = 6;
        size_t old_buf_size = old_num_elements_ * element_size;
        size_t new_buf_size = new_num_elements_ * element_size;
        if (!new_buf_) {
            // Alloc owned buffer
            // Note: raw memory is used here because it is expected the patched program will call constructors
            // Note: std::make_unique uses value-initialization so buffer is zero-initialized (just like global variables)
            new_buf_owned_ = std::make_unique<std::byte[]>(new_buf_size);
            new_buf_ = reinterpret_cast<T*>(new_buf_owned_.get());
        }

        // update all references
        for (auto& ref : refs_) {
            int offset;
            for (offset = 0; offset <= max_offset; ++offset) {
                auto addr = ref.addr + offset;
                auto* ptr = reinterpret_cast<void*>(addr);
                uintptr_t remap_begin_addr;
                intptr_t remap_new_addr;
                if (ref.is_end_ref) {
                    remap_begin_addr = buf_addr_ + old_buf_size;
                    remap_new_addr = reinterpret_cast<intptr_t>(new_buf_) + new_buf_size;
                }
                else {
                    remap_begin_addr = buf_addr_;
                    remap_new_addr = reinterpret_cast<intptr_t>(new_buf_);
                }
                uintptr_t remap_end_addr = remap_begin_addr + element_size;
                uintptr_t data;
                std::memcpy(&data, ptr, sizeof(data));
                if (data >= remap_begin_addr && data < remap_end_addr) {
                    data = (data - remap_begin_addr) + remap_new_addr;
                    write_mem_ptr(addr, reinterpret_cast<void*>(data));
                    break;
                }
            }
            if (offset == max_offset) {
                xlog::warn("Invalid reference {:x}", ref.addr);
            }
        }
    }

    [[nodiscard]] T* get_buffer() const
    {
        return new_buf_;
    }

    T& operator[](int index)
    {
        return new_buf_[index];
    }
};
