#include <common/config/BuildConfig.h>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/CallPrePostHook.h>
#include <patch_common/FunPrePostHook.h>
#include <patch_common/CodeInjection.h>
#include <xlog/xlog.h>
#include <cstddef>
#include <fstream>
#include "../os/console.h"
#include "../rf/multi.h"
#include "../rf/os/timer.h"
#include "../rf/os/frametime.h"
#include "debug_internal.h"
#include <common/utils/perf-utils.h>

std::vector<std::unique_ptr<PerfAggregator>> PerfAggregator::instances_;

template<typename T>
class SimpleAggregator
{
public:
    void add_sample(T value)
    {
        min_ = std::min(min_, value);
        max_ = std::max(max_, value);
        sum_ += value;
        ++count_;
    }

    void reset()
    {
        max_ = std::numeric_limits<T>::min();
        min_ = std::numeric_limits<T>::max();
        sum_ = 0;
        count_ = 0;
    }

    [[nodiscard]] std::optional<T> min() const
    {
        if (count_ > 0) {
            return {min_};
        }
        return {};
    }

    [[nodiscard]] std::optional<T> max() const
    {
        if (count_ > 0) {
            return {max_};
        }
        return {};
    }

    [[nodiscard]] T sum() const
    {
        return sum_;
    }

    [[nodiscard]] std::optional<T> avg() const
    {
        if (count_ > 0) {
            return sum_ / count_;
        }
        return {};
    }

    [[nodiscard]] int count() const
    {
        return count_;
    }

private:
    T min_;
    T max_;
    T sum_;
    int count_;
};

template<typename T, int N>
class SlotsAggregator
{
public:
    void add_sample(T value)
    {
        slots_[current_] = value;
        current_ = (current_ + 1) % N;
    }

    [[nodiscard]] T min() const
    {
        T min_value = std::numeric_limits<T>::max();
        for (auto value : slots_) {
            min_value = std::min(min_value, value);
        }
        return min_value;
    }

    [[nodiscard]] T max() const
    {
        T max_value = std::numeric_limits<T>::min();
        for (auto value : slots_) {
            max_value = std::max(max_value, value);
        }
        return max_value;
    }

    [[nodiscard]] T avg() const
    {
        T value_sum = 0;
        for (auto value : slots_) {
            value_sum += value;
        }
        return value_sum / N;
    }

private:
    int current_;
    T slots_[N];
};

class ProfilerStats
{
public:
    // 1 us resolution
    static constexpr int time_resolution = 1000000;

    void add_sample(int duration)
    {
        current_frame_times_.add_sample(duration);
        last_times_.add_sample(duration);
    }

    void next_frame()
    {
        last_frames_summed_times_.add_sample(current_frame_times_.sum());
        current_frame_times_.reset();
    }

    [[nodiscard]] const auto& current_frame_times() const
    {
        return current_frame_times_;
    }

    [[nodiscard]] const auto& last_times() const
    {
        return last_times_;
    }

    [[nodiscard]] const auto& last_frames_summed_times() const
    {
        return last_frames_summed_times_;
    }

private:
    SimpleAggregator<int> current_frame_times_;
    SlotsAggregator<int, 128> last_times_;
    SlotsAggregator<int, 128> last_frames_summed_times_;
};

class BaseProfiler
{
public:
    BaseProfiler(const char* name) :
        m_name(name)
    {}
    virtual ~BaseProfiler() = default;

    [[nodiscard]] const char* get_name() const
    {
        return m_name;
    }

    virtual void install() = 0;

    void next_frame()
    {
        m_stats.next_frame();
    }

    [[nodiscard]] ProfilerStats& stats()
    {
        return m_stats;
    }

protected:
    const char* m_name;
    ProfilerStats m_stats;
    int m_enter_time;
    bool m_is_cpu_in_range = false;

    static int current_time()
    {
        return rf::timer_get(ProfilerStats::time_resolution);
    }

    void enter()
    {
        if (m_is_cpu_in_range) {
            xlog::warn("Recursion not supported in BaseProfiler");
            return;
        }
        m_enter_time = current_time();
        m_is_cpu_in_range = true;
    }

    void leave()
    {
        if (!m_is_cpu_in_range) {
            //xlog::warn("Leaving without entering in CallProfiler");
            return;
        }
        m_is_cpu_in_range = false;
        int duration = current_time() - m_enter_time;
        m_stats.add_sample(duration);
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

    [[nodiscard]] uintptr_t get_addr() const
    {
        return m_addr;
    }

    void install() override
    {
        m_enter_inject->install();
        m_leave_inject->install();
    }

private:
    std::unique_ptr<BaseCodeInjection> m_enter_inject;
    std::unique_ptr<BaseCodeInjection> m_leave_inject;
    uintptr_t m_addr;
};

class CallProfiler : public BaseProfiler
{
public:
    CallProfiler(uintptr_t addr, const char* name) :
        BaseProfiler(name), m_call_hook(new CallPrePostHook{addr, [this]() { enter(); }, [this]() { leave(); }})
    {
    }

    void install() override
    {
        m_call_hook->install();
    }

private:
    std::unique_ptr<Installable> m_call_hook;
};

class FunProfiler : public BaseProfiler
{
public:
    FunProfiler(uintptr_t fun_addr, const char* name) :
        BaseProfiler(name),
        m_fun_hook(new FunPrePostHook{fun_addr, [this]() { enter(); }, [this]() { leave(); }})
    {
    }

    void install() override
    {
        m_fun_hook->install();
    }

private:
    std::unique_ptr<Installable> m_fun_hook;
};

std::vector<std::unique_ptr<BaseProfiler>> g_profilers;
bool g_profiler_visible = false;
std::ofstream g_profiler_log;

void install_profiler_patches() {
    static bool installed = false;
    if (!installed) {
        for (auto& p : g_profilers) {
            p->install();
        }
        installed = true;
    }
}

void profiler_log_init()
{
    g_profiler_log.open("logs/profile.csv", std::ofstream::out);
    if (!g_profiler_log.is_open()) {
        return;
    }
    g_profiler_log << "frametime;";
    for (auto& p : g_profilers) {
        g_profiler_log
            << p->get_name() << " avg frame time;"
            << p->get_name() << " num calls;"
            << p->get_name() << " avg time;"
            << p->get_name() << " min time;"
            << p->get_name() << " max time;"
            ;
    }
    g_profiler_log << '\n';
}

void profiler_log_dump()
{
    if (!g_profiler_log.is_open()) {
        return;
    }
    g_profiler_log << rf::frametime << ';';
    for (auto& p : g_profilers) {
        auto& stats = p->stats();
        g_profiler_log
            << stats.last_frames_summed_times().avg() << ';'
            << stats.current_frame_times().count() << ';'
            << stats.last_times().avg() << ';'
            << stats.current_frame_times().min().value_or(-1) << ';'
            << stats.current_frame_times().max().value_or(-1) << ';'
            ;
    }
    g_profiler_log << '\n';
}

bool profiler_log_is_active()
{
    return g_profiler_log.is_open();
}

void profiler_log_close()
{
    g_profiler_log.close();
}

ConsoleCommand2 profiler_cmd{
    "d_profiler",
    []() {
#ifdef NDEBUG
        if (rf::is_multi) {
            rf::console::printf("Profiler cannot be used in multiplayer");
            return;
        }
#endif
        install_profiler_patches();
        g_profiler_visible = !g_profiler_visible;
    },
};

ConsoleCommand2 profiler_log_cmd{
    "d_profiler_log",
    []() {
        install_profiler_patches();
        if (profiler_log_is_active()) {
            profiler_log_close();
        }
        else {
            profiler_log_init();
        }
        rf::console::printf("Profiler log: %s", profiler_log_is_active() ? "enabled" : "disabled");
    },
};

ConsoleCommand2 profiler_print_cmd{
    "d_profiler_print",
    []() {
        for (auto& p : g_profilers) {
            auto& stats = p->stats();
            rf::console::printf("%s: avg %d avg_frame %d", p->get_name(),
                stats.last_times().avg(), stats.last_frames_summed_times().avg());
        }
    },
};

ConsoleCommand2 perf_dump_cmd{
    "d_perf_dump",
    []() {
        rf::console::printf("Number of performance aggregators: %u", PerfAggregator::get_instances().size());
        for (auto& ptr : PerfAggregator::get_instances()) {
            rf::console::printf("%s: calls %u, duration %u us, avg %u us", ptr->get_name().c_str(),
                ptr->get_calls(), ptr->get_total_duration_us(), ptr->get_avg_duration_us());
        }
    },
};

template<typename T, typename... Args>
void add_profiler(Args... args)
{
    g_profilers.push_back(std::make_unique<T>(args...));
}

void profiler_init()
{
    add_profiler<CallProfiler>(0x0043363B, "simulation frame");
    add_profiler<CallProfiler>(0x00433326, "  obj move all");
    add_profiler<AddrRangeProfiler>(0x00487A6B, 0x00487AF4, "    frame init");
    add_profiler<AddrRangeProfiler>(0x00487AF4, 0x00487BF1, "    move pre");
    add_profiler<AddrRangeProfiler>(0x00487BF1, 0x00487C02, "    movers");
    add_profiler<CallProfiler>(0x00487C0E, "    process physics");
    add_profiler<CallProfiler>(0x00487C33, "    move post");
    // add_profiler<CallProfiler>(0x00487FC1, "      entity floor collider"); // causes crash on pdm04
    // add_profiler<CallProfiler>(0x004A0C5D, "        get color from lightmap");
    add_profiler<AddrRangeProfiler>(0x00487C38, 0x00487CAF, "    update water status");

    add_profiler<CallProfiler>(0x004333E5, "  item update all");
    add_profiler<CallProfiler>(0x004333F9, "  trigger update all");
    add_profiler<CallProfiler>(0x00433428, "  particle update all");
    // add_profiler<AddrRangeProfiler>(0x004951EF, 0x00495216, "    move with collisions");
    // add_profiler<CallProfiler>(0x00495904, "      detect collision");
    // add_profiler<CallProfiler>(0x00496357, "        bbox intersect");
    // add_profiler<CallProfiler>(0x00496391, "        face intersect");
    // add_profiler<AddrRangeProfiler>(0x004E1F50, 0x004E2074, "      vertices");
    // add_profiler<CallProfiler>(0x00495329, "    particle process push regions");
    // add_profiler<AddrRangeProfiler>(0x00495282, 0x004952FC, "    particle unk");
    // add_profiler<AddrRangeProfiler>(0x00495445, 0x0049560A, "    particle dmg");
    // add_profiler<CallProfiler>(0x00495170, "    find particle emitter");
    // add_profiler<AddrRangeProfiler>(0x0049514D, 0x0049560A, "    particle one");

    add_profiler<CallProfiler>(0x00433450, "  emitters update all");
    add_profiler<CallProfiler>(0x00433653, "render frame");
    add_profiler<AddrRangeProfiler>(0x00431A00, 0x00431FB0, "  before portal renderer");
    add_profiler<CallProfiler>(0x00431D14, "    gr_setup_3d");
    // add_profiler<CallProfiler>(0x00431D2B, "    in liquid test");
    // add_profiler<CallProfiler>(0x00431F4B, "    fog");
    // add_profiler<FunProfiler>(0x0050E020, "      fog");
    // add_profiler<CallProfiler>(0x00431F99, "    fog bg");

    add_profiler<CallProfiler>(0x00431FF8, "  portal renderer");
    add_profiler<CallProfiler>(0x004D4635, "    search rooms");
    add_profiler<CallProfiler>(0x004D4663, "    sky room");

    add_profiler<CallProfiler>(0x004D4703, "    static geometry");
    add_profiler<CallProfiler>(0x0055F755, "      geocache");
    add_profiler<CallProfiler>(0x005609BA, "      draw decal");
    add_profiler<CallProfiler>(0x004D473E, "    room");
    add_profiler<CallProfiler>(0x004D4C6E, "      portal object mark");
    add_profiler<AddrRangeProfiler>(0x004D4C92, 0x004D4D38, "      proctex mark");
    add_profiler<CallProfiler>(0x004D4CED, "        proctex mark single");
    add_profiler<CallProfiler>(0x004D4D4B, "      proctex process all");
    add_profiler<CallProfiler>(0x004D4D51, "      geomod debris render room");
    add_profiler<CallProfiler>(0x004D4D57, "      glass shard render room");
    add_profiler<CallProfiler>(0x004D4D68, "      render room objects");
    add_profiler<CallProfiler>(0x004D3D2F, "        sort objects");
    add_profiler<AddrRangeProfiler>(0x004D3460, 0x004D34C6, "        any object");
    add_profiler<CallProfiler>(0x00488B0B, "        standard object");
    add_profiler<CallProfiler>(0x00488BAF, "          corpse");
    add_profiler<CallProfiler>(0x00488BC2, "          clutter");
    add_profiler<CallProfiler>(0x00488BD5, "          debris");
    add_profiler<CallProfiler>(0x00488BE8, "          entity");
    add_profiler<CallProfiler>(0x00488C2A, "          item");
    add_profiler<CallProfiler>(0x00488C50, "          mover_brush");
    add_profiler<CallProfiler>(0x00488C76, "          weapon");
    add_profiler<CallProfiler>(0x00488C89, "          glare");
    add_profiler<CallProfiler>(0x0048D7D5, "        bolt emitter");
    add_profiler<FunProfiler>(0x004D33D0, "       details");
    add_profiler<AddrRangeProfiler>(0x00494B90, 0x00494D3C, "      particles");

    //add_profiler<AddrRangeProfiler>(0x004D34AB, 0x004D34B1, "      any render handler");
    //add_profiler<CallProfiler>(0x004D3499, "      prepare lights");

    add_profiler<AddrRangeProfiler>(0x00431FFD, 0x00432F55, "  after portal renderer");
    add_profiler<CallProfiler>(0x0043233E, "    glare flares");
    add_profiler<CallProfiler>(0x0043285D, "    fpgun");
    add_profiler<CallProfiler>(0x00432A18, "    hud");

    add_profiler<FunProfiler>(0x005056A0, "snd_play_3d");
    add_profiler<FunProfiler>(0x00503100, "vmesh_render");
    add_profiler<FunProfiler>(0x0048A400, "obj_hit_callback");
    add_profiler<FunProfiler>(0x004D3350, "set_currently_rendered_room");
    add_profiler<FunProfiler>(0x004E6780, "g_proctex_update_water_1");


    profiler_cmd.register_cmd();
    profiler_log_cmd.register_cmd();
    profiler_print_cmd.register_cmd();
    perf_dump_cmd.register_cmd();
}

void profiler_do_frame_post()
{
    if (g_profiler_visible || profiler_log_is_active()) {
        profiler_log_dump();
        for (auto& p : g_profilers) {
            p->next_frame();
        }
    }
}

void profiler_draw_ui()
{
    if (g_profiler_visible) {
        DebugNameValueBox dbg_box{10, 220};
        dbg_box.section("Profilers:");
        for (auto& p : g_profilers) {
            auto& stats = p->stats();
            if (stats.current_frame_times().count() > 1) {
                dbg_box.printf(p->get_name(), "%d us (%d calls, single %d us)", stats.last_frames_summed_times().avg(),
                    stats.current_frame_times().count(), stats.last_times().avg());
            }
            else {
                dbg_box.printf(p->get_name(), "%d us", stats.last_frames_summed_times().avg());
            }
        }
    }
}

void profiler_multi_init()
{
    g_profiler_visible = false;
}
