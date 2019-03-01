#include "stdafx.h"
#include "high_fps.h"
#include "utils.h"
#include "rf.h"
#include "inline_asm.h"
#include <FunHook2.h>

constexpr auto reference_fps = 30.0f;
constexpr auto reference_framerate = 1.0f / reference_fps;
constexpr auto screen_shake_fps = 150.0f;

double g_ftol_accumulated_HitScreen = 0.0f;
double g_ftol_accumulated_ToggleConsole = 0.0f;
double g_ftol_accumulated_Timer = 0.0f;
static float g_jump_threshold = 0.05f;
static float g_camera_shake_factor = 0.6f;

extern "C" long AccumulatingFtoL(double value, double& accumulator)
{
    //ERR("value %lf accumulator %lf", value, accumulator);
    value += accumulator;
    long result = static_cast<long>(value);
    accumulator = value - result;
    //ERR("after result %ld accumulator %lf", result, accumulator);
    return result;
}

ASM_FUNC(ftol_HitScreen, // Note: value is in ST(0), not on stack
    ASM_I  sub esp, 12
    ASM_I  lea ecx, [ASM_SYM(g_ftol_accumulated_HitScreen)]
    ASM_I  mov [esp + 8], ecx
    ASM_I  fstp qword ptr [esp + 0]
    ASM_I  call ASM_SYM(AccumulatingFtoL)
    ASM_I  add esp, 12
    ASM_I  ret
)

ASM_FUNC(ftol_ToggleConsole,
    ASM_I  sub esp, 12
    ASM_I  lea ecx, [ASM_SYM(g_ftol_accumulated_ToggleConsole)]
    ASM_I  mov [esp + 8], ecx
    ASM_I  fstp qword ptr [esp + 0]
    ASM_I  call ASM_SYM(AccumulatingFtoL)
    ASM_I  add esp, 12
    ASM_I  ret
)

ASM_FUNC(ftol_Timer,
    ASM_I  sub esp, 12
    ASM_I  lea ecx, [ASM_SYM(g_ftol_accumulated_Timer)]
    ASM_I  mov [esp + 8], ecx
    ASM_I  fstp qword ptr [esp + 0]
    ASM_I  call ASM_SYM(AccumulatingFtoL)
    ASM_I  add esp, 12
    ASM_I  ret
)

void STDCALL EntityWaterDecelerateFix(rf::EntityObj* entity)
{
    float vel_factor = 1.0f - (rf::g_fFramerate * 4.5f);
    entity->_Super.PhysInfo.vVel.x *= vel_factor;
    entity->_Super.PhysInfo.vVel.y *= vel_factor;
    entity->_Super.PhysInfo.vVel.z *= vel_factor;
}

extern "C" void WaterAnimateWaves_UpdatePos(rf::Vector3* result)
{
    constexpr float flt_5A3BF4 = 12.8f;
    constexpr float flt_5A3C00 = 3.878788f;
    constexpr float flt_5A3C0C = 4.2666669f;
    result->x += flt_5A3BF4 * (rf::g_fFramerate) / reference_framerate;
    result->y += flt_5A3C0C * (rf::g_fFramerate) / reference_framerate;
    result->z += flt_5A3C00 * (rf::g_fFramerate) / reference_framerate;
}

ASM_FUNC(WaterAnimateWaves_004E68A0,
    ASM_I  mov eax, esi
    ASM_I  add eax, 0x2C
    ASM_I  push eax
    ASM_I  call ASM_SYM(WaterAnimateWaves_UpdatePos)
    ASM_I  add esp, 4
    ASM_I  mov ecx, [esi + 0x24]
    ASM_I  lea eax, [esp + 0x6C - 0x20] // var_20
    ASM_I  mov eax, 0x004E68D1
    ASM_I  jmp eax
)

FunHook2<int(rf::String*, rf::String*, char*)> RflLoad_Hook{
    0x0045C540,
    [](rf::String* level_filename, rf::String* a2, char* error_desc) {
        int ret = RflLoad_Hook.CallTarget(level_filename, a2, error_desc);
        if (ret == 0 && strstr(level_filename->psz, "L5S3")) {
            // Fix submarine exploding - change delay of two events to make submarine physics enabled later
            //INFO("Fixing Submarine exploding bug...");
            rf::Object* obj = rf::ObjGetFromUid(4679);
            if (obj && obj->Type == rf::OT_EVENT) {
                rf::EventObj* event = reinterpret_cast<rf::EventObj*>(reinterpret_cast<uintptr_t>(obj) - 4);
                event->fDelay += 1.5f;
            }
            obj = rf::ObjGetFromUid(4680);
            if (obj && obj->Type == rf::OT_EVENT) {
                rf::EventObj* event = reinterpret_cast<rf::EventObj*>(reinterpret_cast<uintptr_t>(obj) - 4);
                event->fDelay += 1.5f;
            }
        }
        return ret;
    }
};

void HighFpsInit()
{
    // Fix animations broken for high FPS
    AsmWritter(0x00416426).callLong(ftol_HitScreen); // hit screen
    AsmWritter(0x0050ABFB).callLong(ftol_ToggleConsole); // console open/close
    AsmWritter(0x005096A7).callLong(ftol_Timer); // switching weapon and more

    // Fix jumping on high FPS
    WriteMemPtr(0x004A09A6 + 2, &g_jump_threshold);

    // Fix water deceleration on high FPS
    WriteMemUInt8(0x0049D816, ASM_NOP, 5);
    WriteMemUInt8(0x0049D82A, ASM_NOP, 5);
    WriteMemUInt8(0x0049D82A + 5, ASM_PUSH_ESI);
    AsmWritter(0x0049D830).callLong(EntityWaterDecelerateFix);

    // Fix water waves animation on high FPS
    WriteMemUInt8(0x004E68A0, ASM_NOP, 9);
    WriteMemUInt8(0x004E68B6, ASM_LONG_JMP_REL);
    AsmWritter(0x004E68B6).jmpLong(WaterAnimateWaves_004E68A0);

    // Fix incorrect frame time calculation
    WriteMemUInt8(0x00509595, ASM_NOP, 2);
    WriteMemUInt8(0x00509532, ASM_SHORT_JMP_REL);

    // Fix submarine exploding on high FPS
    RflLoad_Hook.Install();

    // Fix screen shake caused by some weapons (eg. Assault Rifle)
    WriteMemPtr(0x0040DBCC + 2, &g_camera_shake_factor);
}

void HighFpsUpdate()
{
    float frame_time = rf::g_fFramerate;
    if (frame_time > 0.0001f) {
        // Make jump fix framerate dependent to fix bouncing on small FPS
        g_jump_threshold = 0.025f + 0.075f * frame_time / (1 / 60.0f);

        // Fix screen shake caused by some weapons (eg. Assault Rifle)
        g_camera_shake_factor = pow(0.6f, frame_time / (1 / screen_shake_fps));
    }
}
