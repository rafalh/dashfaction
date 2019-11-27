#include "high_fps.h"
#include "commands.h"
#include "rf.h"
#include "stdafx.h"
#include "utils.h"
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/InlineAsm.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <array>
#include <unordered_map>
#include <unordered_set>

constexpr auto reference_fps = 30.0f;
constexpr auto reference_framerate = 1.0f / reference_fps;
constexpr auto screen_shake_fps = 150.0f;

static float g_camera_shake_factor = 0.6f;
static LARGE_INTEGER g_qpc_frequency;

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

    static long __fastcall ftol(FtolAccuracyFix* self, [[maybe_unused]] void* edx, double value, void* key)
    {
        auto& state = self->m_state_map[key];

        if (state.num_calls_in_frame > 0 && state.last_val != value)
            TRACE("Different ftol argument during a single frame in address 0x%X", self->m_ftol_call_addr);

        value += state.remainder;
        long result = static_cast<long>(value);
        state.remainder = value - result;
        state.last_val = value;
        state.num_calls_in_frame++;
        state.age = 0;

        return result;
    }

    void NextFrame()
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

    void Install()
    {
        char* code_buf = new char[512];
        UnprotectMem(code_buf, 512);

        using namespace asm_regs;
        AsmWritter{code_buf}
            .mov(ecx, this) // thiscall
            .sub(esp, 12)
            .mov(eax, m_key_loc_opt.value_or(ecx))
            .mov(*(esp + 8), eax)
            .fstp<double>(*(esp + 0))
            .call(&ftol)
            .ret();

        AsmWritter(m_ftol_call_addr).call(code_buf);
    }
};

FtolAccuracyFix g_ftol_accuracy_fixes[]{
    {0x00416426},                // hit screen
    {0x004D5214, asm_regs::esi}, // decal fade out
    {0x005096A7},                // timer
    {0x0050ABFB},                // console open/close
    {0x0051BAD7, asm_regs::esi}, // anim mesh
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

void FtolIssuesDetectionDoFrame()
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
                    rf::DcPrintf("ftol issue detected: address %08X ratio %.2f estimated value %.4f", p.first - 5,
                                 ratio, avg_high_fps);
            }
        }

        rf::min_framerate = 1.0f / 30.0f;
        g_ftol_debug_info_map.clear();
        g_ftol_debug_info_map_old.clear();
    }
    else if (g_num_frames == 5) {
        g_ftol_debug_info_map_old = std::move(g_ftol_debug_info_map);
        g_ftol_debug_info_map.clear();
        rf::min_framerate = 1.0f / 60.0f;
    }

    g_num_frames = (g_num_frames + 1) % 10;
}

DcCommand2 detect_ftol_issues_cmd{
    "detect_ftol_issues",
    [](std::optional<float> fps_opt) {
        static bool patched = false;
        if (!patched) {
            AsmWritter(0x00573528).jmp(ftol_Wrapper);
            patched = true;
        }
        if (!g_ftol_issue_detection) {
            g_ftol_issue_detection_fps = fps_opt.value_or(240);
            FtolIssuesDetectionStart();
        }
        else {
            g_ftol_issue_detection = false;
        }
        rf::DcPrintf("ftol issues detection is %s", g_ftol_issue_detection ? "enabled" : "disabled");
    },
    "detect_ftol_issues <fps>",
};

#endif // DEBUG

rf::Timer g_player_jump_timer;

CodeInjection stuck_to_ground_when_jumping_fix{
    0x0042891E,
    []([[ maybe_unused ]] auto& regs) {
        auto entity = reinterpret_cast<rf::EntityObj*>(regs.esi);
        if (entity->local_player) {
            // Skip land handling code for next 64 ms (like in PF)
            g_player_jump_timer.Set(64);
        }
    },
};

CodeInjection stuck_to_ground_when_using_jump_pad_fix{
    0x00486B60,
    []([[ maybe_unused ]] auto& regs) {
        auto entity = reinterpret_cast<rf::EntityObj*>(regs.esi);
        if (entity->local_player) {
            // Skip land handling code for next 64 ms
            g_player_jump_timer.Set(64);
        }
    },
};

CodeInjection stuck_to_ground_fix{
    0x00487F82,
    [](auto& regs) {
        auto entity = reinterpret_cast<rf::EntityObj*>(regs.esi);
        if (entity->local_player && g_player_jump_timer.IsSet() && !g_player_jump_timer.IsFinished()) {
            // Jump to jump handling code that sets entity to falling movement mode
            regs.eip = 0x00487F7B;
        }
    },
};




void STDCALL EntityWaterDecelerateFix(rf::EntityObj* entity)
{
    float vel_factor = 1.0f - (rf::frametime * 4.5f);
    entity->_super.phys_info.vel.x *= vel_factor;
    entity->_super.phys_info.vel.y *= vel_factor;
    entity->_super.phys_info.vel.z *= vel_factor;
}

CodeInjection WaterAnimateWaves_speed_fix{
    0x004E68A0,
    [](auto& regs) {
        rf::Vector3& result = *reinterpret_cast<rf::Vector3*>(regs.esi + 0x2C);
        result.x += 12.8f * (rf::frametime) / reference_framerate;
        result.y += 4.2666669f * (rf::frametime) / reference_framerate;
        result.z += 3.878788f * (rf::frametime) / reference_framerate;
    },
};

FunHook<int(rf::String&, rf::String&, char*)> RflLoad_hook{
    0x0045C540,
    [](rf::String& level_filename, rf::String& a2, char* error_desc) {
        int ret = RflLoad_hook.CallTarget(level_filename, a2, error_desc);
        //INFO("Loaded level: %s", level_filename.CStr());
        if (ret == 0 && _stricmp(level_filename, "L5S3.rfl") == 0) {
            // Fix submarine exploding - change delay of two events to make submarine physics enabled later
            //INFO("Fixing Submarine exploding bug...");
            int uids[] = {4679, 4680};
            for (int uid : uids) {
                rf::Object* obj = rf::ObjGetFromUid(uid);
                if (obj && obj->type == rf::OT_EVENT) {
                    rf::EventObj* event = reinterpret_cast<rf::EventObj*>(reinterpret_cast<uintptr_t>(obj) - 4);
                    event->delay += 1.5f;
                }
            }
        }
        return ret;
    },
};

FunHook<int(int)> timer_get_hook{
    0x00504AB0,
    [](int scale) {
        static auto& timer_base = AddrAsRef<LARGE_INTEGER>(0x01751BF8);
        static auto& timer_last_value = AddrAsRef<LARGE_INTEGER>(0x01751BD0);
        // get QPC current value
        LARGE_INTEGER current_qpc_value;
        QueryPerformanceCounter(&current_qpc_value);
        // make sure time never goes backward
        if (current_qpc_value.QuadPart < timer_last_value.QuadPart) {
            current_qpc_value = timer_last_value;
        }
        timer_last_value = current_qpc_value;
        // Make sure we count from game start
        current_qpc_value.QuadPart -= timer_base.QuadPart;
        // Multiply with unit scale (eg. ms/us) before division
        current_qpc_value.QuadPart *= scale;
        // Divide by frequency using 64 bits and then cast to 32 bits
        // Note: sign of result does not matter because it is used only for deltas
        return static_cast<int>(current_qpc_value.QuadPart / g_qpc_frequency.QuadPart);
    },
};

CallHook<void(int)> frametime_update_sleep_hook{
    0x005095B4,
    [](int ms) {
        --ms;
        if (ms > 0) {
            frametime_update_sleep_hook.CallTarget(ms);
        }
    },
};

CodeInjection cutscene_shot_sync_fix{
    0x0045B43B,
    [](X86Regs& regs) {
        auto& current_shot_idx = StructFieldRef<int>(rf::active_cutscene, 0x808);
        auto& current_shot_timer = StructFieldRef<rf::Timer>(rf::active_cutscene, 0x810);
        if (current_shot_idx > 1) {
            // decrease time for next shot using current shot timer value
            int shot_time_left_ms = current_shot_timer.GetTimeLeftMs();
            if (shot_time_left_ms > 0 || shot_time_left_ms < -100)
                WARN("invalid shot_time_left_ms %d", shot_time_left_ms);
            regs.eax += shot_time_left_ms;
        }
    },
};

void HighFpsInit()
{
    // Fix animations broken on high FPS because of ignored ftol remainder
    for (auto& ftol_fix : g_ftol_accuracy_fixes) {
        ftol_fix.Install();
    }

#ifdef DEBUG
    detect_ftol_issues_cmd.Register();
#endif

    // Fix player being stuck to ground when jumping, especially when FPS is greater than 200
    stuck_to_ground_when_jumping_fix.Install();
    stuck_to_ground_when_using_jump_pad_fix.Install();
    stuck_to_ground_fix.Install();

    // Fix water deceleration on high FPS
    AsmWritter(0x0049D816).nop(5);
    AsmWritter(0x0049D82A, 0x0049D835).nop(5).push(asm_regs::esi).call(EntityWaterDecelerateFix);

    // Fix water waves animation on high FPS
    AsmWritter(0x004E68A0, 0x004E68A9).nop();
    AsmWritter(0x004E68B6, 0x004E68D1).nop();
    WaterAnimateWaves_speed_fix.Install();

    // Fix TimerGet handling of frequency greater than 2MHz (sign bit is set in 32 bit dword)
    QueryPerformanceFrequency(&g_qpc_frequency);
    timer_get_hook.Install();

    // Fix incorrect frame time calculation
    AsmWritter(0x00509595).nop(2);
    WriteMem<u8>(0x00509532, ASM_SHORT_JMP_REL);
    frametime_update_sleep_hook.Install();

    // Fix submarine exploding on high FPS
    RflLoad_hook.Install();

    // Fix screen shake caused by some weapons (eg. Assault Rifle)
    WriteMemPtr(0x0040DBCC + 2, &g_camera_shake_factor);

    // Remove cutscene sync RF hackfix
    WriteMem<float>(0x005897B4, 1000.0f);
    WriteMem<float>(0x005897B8, 1.0f);
    static float zero = 0.0f;
    WriteMemPtr(0x0045B42A + 2, &zero);

    // Fix cutscene shot timer sync on high fps
    cutscene_shot_sync_fix.Install();
}

void HighFpsUpdate()
{
    float frame_time = rf::frametime;
    if (frame_time > 0.0001f) { // < 1000FPS
        // Fix screen shake caused by some weapons (eg. Assault Rifle)
        g_camera_shake_factor = std::pow(0.6f, frame_time / (1 / screen_shake_fps));
    }

#if DEBUG
    FtolIssuesDetectionDoFrame();
#endif

    for (auto& ftol_fix : g_ftol_accuracy_fixes) {
        ftol_fix.NextFrame();
    }
}
