#include "debug_internal.h"
#include "../stdafx.h"
#include "../console/console.h"
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/CodeBuffer.h>
#include <patch_common/MemUtils.h>
#include <xlog/xlog.h>
#include <cstddef>
#include "debug_internal.h"
#include "../utils/perf-utils.h"

std::vector<std::unique_ptr<PerfAggregator>> PerfAggregator::instances_;

class Installable
{
public:
    virtual ~Installable() {}
    virtual void install() = 0;
};

template<typename PreCallback, typename PostCallback>
class FunPrePostHook : public Installable
{
private:
    uintptr_t m_addr;
    PreCallback m_pre_cb;
    PostCallback m_post_cb;
    std::vector<uintptr_t> m_ret_stack; // Note: it is not thread-safe
    CodeBuffer m_leave_code;
    std::unique_ptr<BaseCodeInjection> m_enter_patch;

public:
    FunPrePostHook(uintptr_t addr, PreCallback pre_cb, PostCallback post_cb) :
        m_addr(addr), m_pre_cb(pre_cb), m_post_cb(post_cb),
        m_leave_code(256),
        m_enter_patch(new CodeInjection{addr, [this](auto& regs) { enter_handler(regs); }})
    {
        using namespace asm_regs;
        AsmWriter asm_writter{m_leave_code};
        asm_writter
            .push(eax)             // save eax for later use
            .mov(ecx, this)        // save this in ECX (thiscall)
            .call(&leave_handler)  // call leave handler that returns the final return address
            .pop(ecx)              // pop saved eax into ecx temporary
            .push(eax)             // push return address
            .mov(eax, ecx)         // restore eax (function result)
            .ret();                // return to the function caller
    }

    void install() override
    {
        m_enter_patch->Install();
    }

private:
    void enter_handler(BaseCodeInjection::Regs& regs)
    {
        auto& ret_addr = *reinterpret_cast<uintptr_t*>(regs.esp);
        m_ret_stack.push_back(ret_addr);
        ret_addr = reinterpret_cast<uintptr_t>(m_leave_code.get());
        m_pre_cb();
    }

    static uintptr_t __thiscall leave_handler(FunPrePostHook& self)
    {
        self.m_post_cb();
        auto ret_addr = self.m_ret_stack.back();
        self.m_ret_stack.pop_back();
        return ret_addr;
    }
};

class BaseProfiler
{
public:
    BaseProfiler(const char* name) :
        m_name(name)
    {}
    virtual ~BaseProfiler() {}

    const char* get_name() const
    {
        return m_name;
    }

    int get_avg_duration() const
    {
        int total_time = 0;
        for (auto t : m_duration_array) {
            total_time += t;
        }
        return total_time / num_slots;
    }

    int get_avg_frame_duration() const
    {
        int total_time = 0;
        for (auto t : m_frame_duration_array) {
            total_time += t;
        }
        return total_time / num_slots;
    }

    int get_num_calls_in_frame() const
    {
        return m_num_calls_in_frame;
    }

    virtual void install() = 0;

    void next_frame()
    {
        m_current_frame_slot = (m_current_frame_slot + 1) % num_slots;
        m_frame_duration_array[m_current_frame_slot] = 0;
        m_num_calls_in_frame = 0;
    }

protected:
    static constexpr int num_slots = 100;

    const char* m_name;
    int m_enter_time;
    int m_duration_array[num_slots];
    int m_current_slot = 0;
    int m_frame_duration_array[num_slots];
    int m_current_frame_slot = 0;
    int m_num_calls_in_frame = 0;
    bool m_is_cpu_in_range = false;

    static int current_time()
    {
        return rf::TimerGet(1000000);
    }

    void enter()
    {
        if (m_is_cpu_in_range) {
            xlog::warn("Recursion not supported in CallProfiler");
            return;
        }
        m_enter_time = current_time();
        m_is_cpu_in_range = true;
        ++m_num_calls_in_frame;
    }

    void leave()
    {
        if (!m_is_cpu_in_range) {
            //xlog::warn("Leaveing without entering in CallProfiler");
            return;
        }
        m_is_cpu_in_range = false;
        int duration = current_time() - m_enter_time;

        m_duration_array[m_current_slot] = duration;
        m_current_slot = (m_current_slot + 1) % num_slots;

        m_frame_duration_array[m_current_frame_slot] += duration;
    }
};

class AddrRangeProfiler : public BaseProfiler
{
public:
    AddrRangeProfiler(uintptr_t enter_addr, uintptr_t leave_addr, const char* name) :
        BaseProfiler(name),
        m_enter_inject(new CodeInjection{enter_addr, [this]() { enter(); }}),
        m_leave_inject(new CodeInjection{leave_addr, [this]() { leave(); }}),
        m_addr(enter_addr)
    {
    }

    uintptr_t get_addr() const
    {
        return m_addr;
    }

    virtual void install() override
    {
        m_enter_inject->Install();
        m_leave_inject->Install();
    }

private:
    std::unique_ptr<BaseCodeInjection> m_enter_inject;
    std::unique_ptr<BaseCodeInjection> m_leave_inject;
    uintptr_t m_addr;
};

class CallProfiler : public AddrRangeProfiler
{
public:
    CallProfiler(uintptr_t addr, const char* name) :
        AddrRangeProfiler(addr, addr + 5, name)
    {
    }

    void install() override
    {
#ifdef DEBUG
        auto opcode = AddrAsRef<u8>(get_addr());
        assert(opcode == asm_opcodes::call_rel_long || opcode == asm_opcodes::jmp_rel_long);
#endif
        AddrRangeProfiler::install();
    }
};

class FunProfiler : public BaseProfiler
{
public:
    FunProfiler(uintptr_t fun_addr, const char* name) :
        BaseProfiler(name),
        m_fun_hook(new FunPrePostHook{fun_addr, [this]() { enter(); }, [this]() { leave(); }})
    {
    }

    virtual void install() override
    {
        m_fun_hook->install();
    }

private:
    std::unique_ptr<Installable> m_fun_hook;
};

std::vector<std::unique_ptr<BaseProfiler>> g_profilers;

bool g_profiler_visible = false;

void InstallProfilerPatches() {
    static bool installed = false;
    if (!installed) {
        for (auto& p : g_profilers) {
            p->install();
        }
        installed = true;
    }
}

ConsoleCommand2 profiler_cmd{
    "d_profiler",
    []() {
        InstallProfilerPatches();
        g_profiler_visible = !g_profiler_visible;
    },
};

ConsoleCommand2 perf_dump_cmd{
    "d_perf_dump",
    []() {
        rf::ConsolePrintf("Number of performance aggregators: %u", PerfAggregator::get_instances().size());
        for (auto& ptr : PerfAggregator::get_instances()) {
            rf::ConsolePrintf("%s: calls %u, duration %u us, avg %u us", ptr->get_name().c_str(),
                ptr->get_calls(), ptr->get_total_duration_us(), ptr->get_avg_duration_us());
        }
    },
};

void ProfilerInit()
{
    g_profilers.push_back(std::make_unique<CallProfiler>(0x0043363B, "simulation frame"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00433326, "  obj move all"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00487B60, "    move pre"));
    //g_profilers.push_back(std::make_unique<CallProfiler>(0x00487BA3, "    entity pre2")); // causes crash
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00487C0E, "    physics"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00487C33, "    move post"));
    // g_profilers.push_back(std::make_unique<CallProfiler>(0x00487FC1, "      entity floor collider")); // causes crash on pdm04
    // g_profilers.push_back(std::make_unique<CallProfiler>(0x004A0C5D, "        get color from lightmap"));



    g_profilers.push_back(std::make_unique<CallProfiler>(0x004333E5, "  item update all"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x004333F9, "  trigger update all"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00433428, "  particle update all"));
    // g_profilers.push_back(std::make_unique<AddrRangeProfiler>(0x004951EF, 0x00495216, "    move with collisions"));
    // g_profilers.push_back(std::make_unique<CallProfiler>(0x00495904, "      detect collision"));
    // g_profilers.push_back(std::make_unique<CallProfiler>(0x00496357, "        bbox intersect"));
    // g_profilers.push_back(std::make_unique<CallProfiler>(0x00496391, "        face intersect"));
    // g_profilers.push_back(std::make_unique<AddrRangeProfiler>(0x004E1F50, 0x004E2074, "      vertices"));
    // g_profilers.push_back(std::make_unique<CallProfiler>(0x00495329, "    particle process push regions"));
    // g_profilers.push_back(std::make_unique<AddrRangeProfiler>(0x00495282, 0x004952FC, "    particle unk"));
    // g_profilers.push_back(std::make_unique<AddrRangeProfiler>(0x00495445, 0x0049560A, "    particle dmg"));
    // g_profilers.push_back(std::make_unique<CallProfiler>(0x00495170, "    find particle emitter"));

    //g_profilers.push_back(std::make_unique<AddrRangeProfiler>(0x0049514D, 0x0049560A, "    particle one"));

    g_profilers.push_back(std::make_unique<CallProfiler>(0x00433450, "  emitters update all"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00433653, "render frame"));
    g_profilers.push_back(std::make_unique<AddrRangeProfiler>(0x00431A00, 0x00431D1C, "  misc0 (before level geometry)"));
    g_profilers.push_back(std::make_unique<AddrRangeProfiler>(0x00431D1C, 0x00431FA1, "  misc1 (before level geometry)")); // slow?
    g_profilers.push_back(std::make_unique<AddrRangeProfiler>(0x00431FA1, 0x00431FF8, "  misc2 (before level geometry)"));
    // g_profilers.push_back(std::make_unique<CallProfiler>(0x00431D2B, "    in liquid test"));
    //g_profilers.push_back(std::make_unique<CallProfiler>(0x00431F4B, "    fog"));
    //g_profilers.push_back(std::make_unique<FunProfiler>(0x0050E020, "      fog"));
    //g_profilers.push_back(std::make_unique<CallProfiler>(0x00431F99, "    fog bg"));

    g_profilers.push_back(std::make_unique<CallProfiler>(0x00431FF8, "  level geometry"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x004D4703, "    static geometry"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x0055F755, "      geocache"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x005609BA, "      draw decal"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x004D4C6E, "    fill obj queue"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x004D4D57, "    glass shards"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x004D4D68, "    process obj queue"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x004D3D2F, "      sort queue"));
    g_profilers.push_back(std::make_unique<AddrRangeProfiler>(0x004D3460, 0x004D34C6, "      any object"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00488B0B, "      standard object"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00488BAF, "        corpse"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00488BC2, "        clutter"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00488BD5, "        debris"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00488BE8, "        entity"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00488C2A, "        item"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00488C50, "        mover"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00488C76, "        weapon"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00488C89, "        volumetric lights"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x0048D7D5, "      bolt emitter"));
    g_profilers.push_back(std::make_unique<FunProfiler>(0x004D33D0, "      details"));
    g_profilers.push_back(std::make_unique<AddrRangeProfiler>(0x00494B90, 0x00494D3C, "      particles"));

    //g_profilers.push_back(std::make_unique<AddrRangeProfiler>(0x004D34AB, 0x004D34B1, "      any render handler"));
    //g_profilers.push_back(std::make_unique<CallProfiler>(0x004D3499, "      prepare lights"));

    g_profilers.push_back(std::make_unique<AddrRangeProfiler>(0x00431FFD, 0x00432A18, "  misc (after level geometry)"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x0043233E, "  coronas"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x0043285D, "  fpgun"));
    g_profilers.push_back(std::make_unique<CallProfiler>(0x00432A18, "  HUD"));
    g_profilers.push_back(std::make_unique<AddrRangeProfiler>(0x00432A1D, 0x00432F4F, "  misc (after HUD)"));

    profiler_cmd.Register();
    perf_dump_cmd.Register();
}

void ProfilerDrawUI()
{
    if (g_profiler_visible) {
        //AddrAsRef<bool>(0x00596144) = 0;
        DebugNameValueBox dbg_box{10, 100};
        dbg_box.Section("Profilers:");
        for (auto& p : g_profilers) {
            if (p->get_num_calls_in_frame() > 1) {
                dbg_box.Printf(p->get_name(), "%d us (%d calls, single %d us)", p->get_avg_frame_duration(),
                    p->get_num_calls_in_frame(), p->get_avg_duration());
            }
            else {
                dbg_box.Printf(p->get_name(), "%d us", p->get_avg_frame_duration());
            }

            p->next_frame();
        }
    }
}
