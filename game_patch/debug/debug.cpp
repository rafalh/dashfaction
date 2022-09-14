#include "debug_internal.h"
#include <patch_common/FunHook.h>
#include <xlog/xlog.h>
#include "../os/console.h"

#define DEBUG_PERF 1

#ifdef NDEBUG
#define MEMORY_TRACKING 0
#define VARRAY_OOB_CHECK 0
#define EMULATE_PACKET_LOSS 0
#else // NDEBUG
#define MEMORY_TRACKING 1
#define VARRAY_OOB_CHECK 0
#define EMULATE_PACKET_LOSS 0
#define PACKET_LOSS_RATE 10 // every n packet is lost
#endif // NDEBUG

#if MEMORY_TRACKING
constexpr uint32_t BOUND_MARKER = 0xDEADBEEF;
#endif

int g_num_heap_allocs = 0;
size_t g_current_heap_usage = 0;
size_t g_max_heap_usage = 0;

FunHook<int(size_t)> callnewh_hook{
    0x0057A212,
    [](size_t size) {
        xlog::error("Failed to allocate %u bytes", size);
        return callnewh_hook.call_target(size);
    },
};

#if MEMORY_TRACKING

FunHook<void*(size_t, bool)> nh_malloc_hook{
    0x00573B49,
    [](size_t size, bool unk) -> void* {
        g_num_heap_allocs++;
        g_current_heap_usage += size;
        g_max_heap_usage = std::max(g_max_heap_usage, g_current_heap_usage);
        void* ptr = nh_malloc_hook.call_target(size + 12, unk);
        if (!ptr) {
            xlog::trace("Allocation of %u bytes failed!", size);
            return nullptr;
        }
        auto bytes = static_cast<std::byte*>(ptr);
        *reinterpret_cast<uint32_t*>(bytes) = size;
        *reinterpret_cast<uint32_t*>(bytes + 4) = BOUND_MARKER;
        *reinterpret_cast<uint32_t*>(bytes + 8 + size) = BOUND_MARKER;
        // Overwrite old data to make detecting use-after-free errors easier
        std::memset(bytes + 8, 0xCC, size);
        auto result = bytes + 8;
        xlog::trace("nh_malloc %x -> %p", size, result);
        return result;
    },
};

FunHook<void(void*)> free_hook{
    0x00573C71,
    [](void* ptr) {
        xlog::trace("free %p", ptr);
        if (!ptr) {
            return;
        }
        auto bytes = static_cast<std::byte*>(ptr);
        bytes -= 8;
        auto size = *reinterpret_cast<uint32_t*>(bytes);
        auto front_marker = *reinterpret_cast<uint32_t*>(bytes + 4);
        auto tail_marker = *reinterpret_cast<uint32_t*>(bytes + 8 + size);
        g_current_heap_usage -= size;
        if (front_marker != BOUND_MARKER) {
            xlog::warn("Memory corruption detected: front marker %x", front_marker);
        }
        if (tail_marker != BOUND_MARKER) {
            xlog::warn("Memory corruption detected: tail marker %x", tail_marker);
        }
        // Overwrite old data to make detecting use-after-free errors easier
        std::memset(bytes + 8, 0xCC, size);
        free_hook.call_target(bytes);
    },
};

FunHook<void*(void*, size_t)> realloc_hook{
    0x00578873,
    [](void* ptr, size_t size) {
        auto bytes = static_cast<std::byte*>(ptr);
        bytes -= 8;
        auto old_size = *reinterpret_cast<size_t*>(bytes);
        g_current_heap_usage -= old_size;
        g_current_heap_usage += size;
        g_max_heap_usage = std::max(g_max_heap_usage, g_current_heap_usage);
        auto out_ptr = realloc_hook.call_target(bytes, size + 12);
        auto out_bytes = reinterpret_cast<std::byte*>(out_ptr);
        out_ptr = out_bytes + 8;
        xlog::trace("realloc %p %x -> %p", ptr, size, out_ptr);
        return out_ptr;
    },
};

FunHook<size_t(void*)> msize_hook{
    0x00578BA2,
    [](void* ptr) {
        xlog::trace("msize %p", ptr);
        ptr = static_cast<std::byte*>(ptr) - 8;
        return msize_hook.call_target(ptr) - 12;
    },
};

ConsoleCommand2 mem_stats_cmd{
    "d_mem_stats",
    []() {
        constexpr float mb_float = 1024.0f * 1024.0f;
        rf::console::printf("Number of heap allocations: %d", g_num_heap_allocs);
        rf::console::printf("Current heap usage: %.2f MB", g_current_heap_usage / mb_float);
        rf::console::printf("Max heap usage: %.2f MB", g_max_heap_usage / mb_float);
    },
};

#endif // MEMORY_TRACKING

#if VARRAY_OOB_CHECK
CodeInjection VArray_Ptr__get_out_of_bounds_check{
    0x0040A480,
    [](auto& regs) {
        int size = *static_cast<int*>(regs.ecx);
        int index = *reinterpret_cast<int*>(regs.esp + 4);
        if (index < 0 || index >= size) {
            ERR("VArray out of bounds access! index %d size %d", index, size);
        }
    },
};
#endif // VARRAY_OOB_CHECK

#if EMULATE_PACKET_LOSS

FunHook<int(const void*, unsigned, int, const rf::NetAddr*, int)> net_send_hook{
    0x00528820,
    [](const void* packet, unsigned packet_len, int flags, const rf::NetAddr* addr, int packet_kind) {
        if (rand() % PACKET_LOSS_RATE == 0)
            return 0;
        return net_send_hook.call_target(packet, packet_len, flags, addr, packet_kind);
    },
};

FunHook<void(void*, const void*, unsigned, rf::NetAddr*)> net_buffer_packet_hook{
    0x00528950,
    [](void* buffer, const void* data, unsigned data_len, rf::NetAddr* addr) {
        if (rand() % PACKET_LOSS_RATE == 0)
            return;
        return net_buffer_packet_hook.call_target(buffer, data, data_len, addr);
    },
};

#endif // EMULATE_PACKET_LOSS

void debug_multi_init()
{
    debug_cmd_multi_init();
    profiler_multi_init();
}

void debug_apply_patches()
{
    // Log error when memory allocation fails
    callnewh_hook.install();

#if MEMORY_TRACKING
    nh_malloc_hook.install();
    free_hook.install();
    realloc_hook.install();
    msize_hook.install();
#endif // MEMORY_TRACKING

#if VARRAY_OOB_CHECK
    VArray_Ptr__get_out_of_bounds_check.install();
#endif

#if EMULATE_PACKET_LOSS
    net_send_hook.install();
    net_buffer_packet_hook.install();
#endif

    debug_unresponsive_apply_patches();
#if DEBUG_PERF
    profiler_init();
#endif
}

void debug_init()
{
#if MEMORY_TRACKING
    mem_stats_cmd.register_cmd();
#endif

    debug_cmd_init();
    debug_unresponsive_init();
#ifndef NDEBUG
    register_obj_debug_commands();
#endif
}

void debug_render()
{
    debug_cmd_render();
}

void debug_render_ui()
{
    debug_cmd_render_ui();
#if DEBUG_PERF
    profiler_draw_ui();
#endif
#ifndef NDEBUG
    render_obj_debug_ui();
#endif
}

void debug_cleanup()
{
    debug_unresponsive_cleanup();
}

void debug_do_frame_pre()
{
    debug_unresponsive_do_update();
}

void debug_do_frame_post()
{
    profiler_do_frame_post();
}
