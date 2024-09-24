#pragma once

#include <vector>
#include <memory>

template <typename T, size_t N>
class MemPool
{
    using Slot = typename std::aligned_storage<sizeof(T), alignof(T)>::type;

    class Delete
    {
        MemPool* pool;

    public:
        Delete(MemPool* pool) : pool(pool) {}

        void operator()(T* ptr) const
        {
            pool->release(ptr);
        }
    };

    std::vector<Slot*> free_slots;
    std::vector<std::unique_ptr<Slot[]>> pages;

public:
    using Pointer = std::unique_ptr<T, Delete>;

    MemPool() = default;
    MemPool(const MemPool&) = delete;
    MemPool& operator=(const MemPool&) = delete;

    Pointer alloc()
    {
        return {acquire(), {this}};
    }

private:
    void alloc_page()
    {
        pages.push_back(std::make_unique<Slot[]>(N));
        auto& page = pages.back();
        free_slots.reserve(free_slots.size() + N);
        for (std::size_t i = 0; i < N; ++i) {
            Slot& slot = page[i];
            free_slots.push_back(&slot);
        }
    }

    T* acquire()
    {
        if (free_slots.empty()) {
            alloc_page();
        }
        Slot* p = free_slots.back();
        free_slots.pop_back();
        return ::new (p) T;
    }

    void release(T* p)
    {
        p->~T();
        free_slots.push_back(std::launder(reinterpret_cast<Slot*>(p)));
    }
};
