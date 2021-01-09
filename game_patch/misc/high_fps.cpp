#include "high_fps.h"
#include "../os/console.h"
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/InlineAsm.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>
#include <array>
#include <unordered_map>
#include <unordered_set>
#include <optional>

class FtolAccuracyFix
{
    struct StateData
    {
        double remainder = 0.0;
        double last_val = 0.0;
        int num_calls_in_frame = 0;
        int age = 0;
    };

    uintptr_t m_ftol_call_addr;
    std::optional<AsmRegMem> m_key_loc_opt;
    std::unordered_map<void*, StateData> m_state_map;

public:
    FtolAccuracyFix(uintptr_t ftol_call_addr, std::optional<AsmRegMem> key_loc = {}) :
        m_ftol_call_addr(ftol_call_addr), m_key_loc_opt(key_loc)
    {}

    static long __thiscall ftol(FtolAccuracyFix* self, double value, void* key)
    {
        auto& state = self->m_state_map[key];

        if (state.num_calls_in_frame > 0 && state.last_val != value)
            xlog::trace("Different ftol argument during a single frame in address 0x%X", self->m_ftol_call_addr);

        value += state.remainder;
        long result = static_cast<long>(value);
        state.remainder = value - result;
        state.last_val = value;
        state.num_calls_in_frame++;
        state.age = 0;

        return result;
    }

    void next_frame()
    {
        std::vector<void*> keys_to_erase;
        for (auto& p : m_state_map) {
            auto& state = p.second;
            state.num_calls_in_frame = 0;
            state.age++;
            if (state.age >= 100)
                keys_to_erase.push_back(p.first);
        }
        for (auto& k : keys_to_erase) {
            m_state_map.erase(k);
        }
    }

    void install()
    {
        char* code_buf = new char[512];
        unprotect_mem(code_buf, 512);

        using namespace asm_regs;
        AsmWriter{code_buf}
            .mov(ecx, this) // thiscall
            .sub(esp, 12)
            .mov(eax, m_key_loc_opt.value_or(ecx))
            .mov(*(esp + 8), eax)
            .fstp<double>(*(esp + 0))
            .call(&ftol)
            .ret();

        AsmWriter(m_ftol_call_addr).call(code_buf);
    }
};

FtolAccuracyFix g_ftol_accuracy_fixes[]{
    {0x00416426},                // hit screen
    {0x004D5214, asm_regs::esi}, // decal fade out
    {0x005096A7},                // timer
    {0x0050ABFB},                // console open/close
    {0x0051BAD7, asm_regs::esi}, // vmesh
};

#ifdef DEBUG

struct FtolDebugInfo
{
    double val_sum = 0.0;
    int num_calls = 0;
};

int g_num_frames = 0;
bool g_ftol_issue_detection = false;
float g_ftol_issue_detection_fps;
std::unordered_map<uintptr_t, FtolDebugInfo> g_ftol_debug_info_map;
std::unordered_map<uintptr_t, FtolDebugInfo> g_ftol_debug_info_map_old;
std::unordered_set<uintptr_t> g_ftol_issues;

extern "C" long ftol2(double value, uintptr_t ret_addr)
{
    long result = static_cast<long>(value);
    if (g_ftol_issue_detection) {
        FtolDebugInfo& info = g_ftol_debug_info_map[ret_addr];
        info.val_sum += value;
        info.num_calls++;
    }
    return result;
}

// clang-format off
ASM_FUNC(ftol_Wrapper,
    ASM_I  mov ecx, [esp]
    ASM_I  sub esp, 12
    ASM_I  mov [esp + 8], ecx
    ASM_I  fstp qword ptr [esp + 0]
    ASM_I  call ASM_SYM(ftol2)
    ASM_I  add esp, 12
    ASM_I  ret
)
// clang-format on

void FtolIssuesDetectionStart()
{
    g_ftol_issues.clear();
    g_ftol_issue_detection = true;
}

void ftol_issues_detection_do_frame()
{
    if (!g_ftol_issue_detection)
        return;
    if (g_num_frames == 0) {
        for (auto p : g_ftol_debug_info_map) {
            auto& info_60 = p.second;
            auto& info_30 = g_ftol_debug_info_map_old[p.first];
            float avg_60 = static_cast<float>(info_60.val_sum / info_60.num_calls);
            float avg_30 = static_cast<float>(info_30.val_sum / info_30.num_calls);
            float value_ratio = avg_60 / avg_30;
            float framerate_ratio = 30.0f / 60.0f;
            float ratio = value_ratio / framerate_ratio;
            bool is_fps_dependent = fabs(ratio - 1.0f) < 0.1f;
            float avg_high_fps = fabs(avg_60 * 60.0f / g_ftol_issue_detection_fps);
            bool is_significant = avg_high_fps < 1.0f;
            if (is_fps_dependent && is_significant) {
                bool is_new = g_ftol_issues.insert(p.first).second;
                if (is_new)
                    rf::console_printf("ftol issue detected: address %08X ratio %.2f estimated value %.4f", p.first - 5,
                                 ratio, avg_high_fps);
            }
        }

        rf::frametime_min = 1.0f / 30.0f;
        g_ftol_debug_info_map.clear();
        g_ftol_debug_info_map_old.clear();
    }
    else if (g_num_frames == 5) {
        g_ftol_debug_info_map_old = std::move(g_ftol_debug_info_map);
        g_ftol_debug_info_map.clear();
        rf::frametime_min = 1.0f / 60.0f;
    }

    g_num_frames = (g_num_frames + 1) % 10;
}

ConsoleCommand2 detect_ftol_issues_cmd{
    "detect_ftol_issues",
    [](std::optional<float> fps_opt) {
        static bool patched = false;
        if (!patched) {
            AsmWriter(0x00573528).jmp(ftol_Wrapper);
            patched = true;
        }
        if (!g_ftol_issue_detection) {
            g_ftol_issue_detection_fps = fps_opt.value_or(240);
            FtolIssuesDetectionStart();
        }
        else {
            g_ftol_issue_detection = false;
        }
        rf::console_printf("ftol issues detection is %s", g_ftol_issue_detection ? "enabled" : "disabled");
    },
    "detect_ftol_issues <fps>",
};

#endif // DEBUG

void high_fps_init()
{
    // Fix animations broken on high FPS because of ignored ftol remainder
    for (auto& ftol_fix : g_ftol_accuracy_fixes) {
        ftol_fix.install();
    }

#ifdef DEBUG
    detect_ftol_issues_cmd.register_cmd();
#endif
}

void high_fps_update()
{
#ifdef DEBUG
    ftol_issues_detection_do_frame();
#endif

    for (auto& ftol_fix : g_ftol_accuracy_fixes) {
        ftol_fix.next_frame();
    }
}
