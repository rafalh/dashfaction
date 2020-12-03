#include "debug_internal.h"
#include <patch_common/FunHook.h>
#include <xlog/xlog.h>
#include "../console/console.h"

#ifdef NDEBUG
#define MEMORY_TRACKING 0
#define VARRAY_OOB_CHECK 0
#define EMULATE_PACKET_LOSS 0
#else // NDEBUG
#define DEBUG_PERF
#define MEMORY_TRACKING 1
#define VARRAY_OOB_CHECK 0
#define EMULATE_PACKET_LOSS 0
#define PACKET_LOSS_RATE 10 // every n packet is lost
#endif // NDEBUG

constexpr uint32_t BOUND_MARKER = 0xDEADBEEF;

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
        auto bytes = reinterpret_cast<std::byte*>(ptr);
        *reinterpret_cast<size_t*>(bytes) = size;
        *reinterpret_cast<uint32_t*>(bytes + 4) = BOUND_MARKER;
        *reinterpret_cast<uint32_t*>(bytes + 8 + size) = BOUND_MARKER;
        auto result = bytes + 8;
        xlog::trace("nh_malloc %x -> %p", size, result);
        return result;
    },
};

FunHook<void(void*)> free_hook{
    0x00573C71,
    [](void* ptr) {
        xlog::trace("free %p", ptr);
        auto bytes = reinterpret_cast<std::byte*>(ptr);
        bytes -= 8;
        auto size = *reinterpret_cast<size_t*>(bytes);
        auto front_marker = *reinterpret_cast<size_t*>(bytes + 4);
        auto tail_marker = *reinterpret_cast<size_t*>(bytes + 8 + size);
        g_current_heap_usage -= size;
        if (front_marker != BOUND_MARKER) {
            xlog::warn("Memory corruption detected: front marker %x", front_marker);
        }
        if (tail_marker != BOUND_MARKER) {
            xlog::warn("Memory corruption detected: tail marker %x", tail_marker);
        }
        free_hook.call_target(bytes);
    },
};

FunHook<void*(void*, size_t)> realloc_hook{
    0x00578873,
    [](void* ptr, size_t size) {
        auto bytes = reinterpret_cast<std::byte*>(ptr);
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
        ptr = reinterpret_cast<std::byte*>(ptr) - 8;
        return msize_hook.call_target(ptr) - 12;
    },
};

ConsoleCommand2 mem_stats_cmd{
    "d_mem_stats",
    []() {
        constexpr float mb_float = 1024.0f * 1024.0f;
        rf::console_printf("Number of heap allocations: %d", g_num_heap_allocs);
        rf::console_printf("Current heap usage: %.2f MB", g_current_heap_usage / mb_float);
        rf::console_printf("Max heap usage: %.2f MB", g_max_heap_usage / mb_float);
    },
};

#endif // MEMORY_TRACKING

#if VARRAY_OOB_CHECK
CodeInjection VArray_Ptr__get_out_of_bounds_check{
    0x0040A480,
    [](auto& regs) {
        int size = *reinterpret_cast<int*>(regs.ecx);
        int index = *reinterpret_cast<int*>(regs.esp + 4);
        if (index < 0 || index >= size) {
            ERR("VArray out of bounds access! index %d size %d", index, size);
        }
    },
};
#endif // VARRAY_OOB_CHECK

#if EMULATE_PACKET_LOSS

FunHook<int(const void*, unsigned, int, const rf::NwAddr*, int)> nw_send_hook{
    0x00528820,
    [](const void* packet, unsigned packet_len, int flags, const rf::NwAddr* addr, int packet_kind) {
        if (rand() % PACKET_LOSS_RATE == 0)
            return 0;
        return nw_send_hook.call_target(packet, packet_len, flags, addr, packet_kind);
    },
};

FunHook<void(void*, const void*, unsigned, rf::NwAddr*)> nw_add_packet_to_buffer_hook{
    0x00528950,
    [](void* buffer, const void* data, unsigned data_len, rf::NwAddr* addr) {
        if (rand() % PACKET_LOSS_RATE == 0)
            return;
        return nw_add_packet_to_buffer_hook.call_target(buffer, data, data_len, addr);
    },
};

#endif // EMULATE_PACKET_LOSS

void DebugApplyPatches()
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
    nw_send_hook.install();
    nw_add_packet_to_buffer_hook.install();
#endif

    DebugCmdApplyPatches();
    DebugUnresponsiveApplyPatches();
#ifdef DEBUG_PERF
    ProfilerInit();
#endif
}

void DebugInit()
{
#if MEMORY_TRACKING
    mem_stats_cmd.Register();
#endif

    DebugCmdInit();
    DebugUnresponsiveInit();
#ifndef NDEBUG
    RegisterObjDebugCommands();
#endif
}

void DebugRender()
{
    DebugCmdRender();
}

void DebugRenderUI()
{
    DebugCmdRenderUI();
#ifdef DEBUG_PERF
    ProfilerDrawUI();
#endif
#ifndef NDEBUG
    RenderObjDebugUI();
#endif
}

void DebugCleanup()
{
    DebugUnresponsiveCleanup();
}

void DebugDoUpdate()
{
    DebugUnresponsiveDoUpdate();
}
