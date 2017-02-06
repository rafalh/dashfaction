#include "stdafx.h"
#include "high_fps.h"
#include "utils.h"
#include "rf.h"

constexpr auto REF_FPS = 30.0f;
constexpr auto REF_FRAMERATE = 1.0f / REF_FPS;

static double g_FtolAccumulator_HitScreen = 0.0f;
static double g_FtolAccumulated_ToggleConsole = 0.0f;
static double g_FtolAccumulated_Timer = 0.0f;
static float g_JumpThreshold = 0.05f;

long __stdcall AccumulatingFtoL(double fVal, double *pAccumulator)
{
    //ERR("fVal %lf pAccumulator %lf", fVal, *pAccumulator);
    fVal += *pAccumulator;
    long Result = (long)fVal;
    *pAccumulator = fVal - Result;
    //ERR("after Result %ld pAccumulator %lf", Result, *pAccumulator);
    return Result;
}

void NAKED ftol_HitScreen()  // Note: value is in ST(0), not on stack
{
    _asm
    {
        sub esp, 12
        lea ecx, [g_FtolAccumulator_HitScreen]
        mov [esp + 8], ecx
        fstp qword ptr [esp + 0]
        call AccumulatingFtoL
        ret
    }
}

void NAKED ftol_ToggleConsole()
{
    _asm
    {
        sub esp, 12
        lea ecx, [g_FtolAccumulated_ToggleConsole]
        mov [esp + 8], ecx
        fstp qword ptr [esp + 0]
        call AccumulatingFtoL
        ret
    }
}

void NAKED ftol_Timer()
{
    _asm
    {
        sub esp, 12
        lea ecx, [g_FtolAccumulated_Timer]
        mov [esp + 8], ecx
        fstp qword ptr [esp + 0]
        call AccumulatingFtoL
        ret
    }
}

void __stdcall EntityWaterDecelerateFix(rf::CEntity *pEntity)
{
    float fVelFactor = 1.0f - (*rf::g_pfFramerate * 4.5f);
    pEntity->Head.vVel.x *= fVelFactor;
    pEntity->Head.vVel.y *= fVelFactor;
    pEntity->Head.vVel.z *= fVelFactor;
}

void WaterAnimateWaves_UpdatePos(rf::CVector3 *pResult)
{
    constexpr float flt_5A3BF4 = 12.8f;
    constexpr float flt_5A3C00 = 3.878788f;
    constexpr float flt_5A3C0C = 4.2666669f;
    pResult->x += flt_5A3BF4 * (*rf::g_pfFramerate) / REF_FRAMERATE;
    pResult->y += flt_5A3C0C * (*rf::g_pfFramerate) / REF_FRAMERATE;
    pResult->z += flt_5A3C00 * (*rf::g_pfFramerate) / REF_FRAMERATE;
}

void NAKED WaterAnimateWaves_004E68A0()
{
    _asm
    {
        mov eax, esi
        add eax, 2Ch
        push eax
        call WaterAnimateWaves_UpdatePos
        add esp, 4
        mov     ecx, [esi + 24h]
        lea     eax, [esp + 6Ch - 20h] // var_20
        push    0
        push    eax
        push    0
        push    ecx
        mov eax, 004E68D1h
        jmp eax
    }
}

void HighFpsInit()
{
    // Fix animations broken for high FPS
    WriteMemUInt32((PVOID)(0x00416426 + 1), (ULONG_PTR)ftol_HitScreen - (0x00416426 + 0x5)); // hit screen
    WriteMemUInt32((PVOID)(0x0050ABFB + 1), (ULONG_PTR)ftol_ToggleConsole - (0x0050ABFB + 0x5)); // console open/close
    WriteMemUInt32((PVOID)(0x005096A7 + 1), (ULONG_PTR)ftol_Timer - (0x005096A7 + 0x5)); // switching weapon and more

    // Fix jumping on high FPS
    WriteMemPtr((PVOID)(0x004A09A6 + 2), &g_JumpThreshold);

    // Fix water deceleration on high FPS
    WriteMemUInt8Repeat((PVOID)0x0049D816, ASM_NOP, 5);
    WriteMemUInt8Repeat((PVOID)0x0049D82A, ASM_NOP, 5);
    WriteMemUInt8((PVOID)(0x0049D82A + 5), ASM_PUSH_ESI);
    WriteMemPtr((PVOID)(0x0049D830 + 1), (PVOID)((ULONG_PTR)EntityWaterDecelerateFix - (0x0049D830 + 0x5)));

    // Fix water waves animation on high FPS
    WriteMemUInt8((PVOID)0x004E68A0, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)(0x004E68A0 + 1), (ULONG_PTR)WaterAnimateWaves_004E68A0 - (0x004E68A0 + 0x5));

    // Fix incorrect frame time calculation
    WriteMemUInt8Repeat((PVOID)0x00509595, ASM_NOP, 2);
}

void HighFpsUpdate()
{
    // Make jump fix framerate dependent to fix bouncing on small FPS
    g_JumpThreshold = 0.1f * (*rf::g_pfFramerate) / (1/60.0f);
}
