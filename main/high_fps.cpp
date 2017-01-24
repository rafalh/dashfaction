#include "stdafx.h"
#include "high_fps.h"
#include "utils.h"
#include "rf.h"

static double g_FtolAccumulator_HitScreen = 0.0f;
static double g_FtolAccumulated_ToggleConsole = 0.0f;
static double g_FtolAccumulated_Timer = 0.0f;

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

void HighFpsInit()
{
    /* Fix animations broken for high FPS */
    WriteMemUInt32((PVOID)(0x00416426 + 1), (ULONG_PTR)ftol_HitScreen - (0x00416426 + 0x5)); // hit screen
    WriteMemUInt32((PVOID)(0x0050ABFB + 1), (ULONG_PTR)ftol_ToggleConsole - (0x0050ABFB + 0x5)); // console open/close
    WriteMemUInt32((PVOID)(0x005096A7 + 1), (ULONG_PTR)ftol_Timer - (0x005096A7 + 0x5)); // switching weapon and more

    /* Fix jumping on high FPS */
    const static float JUMP_THRESHOLD = 0.05f;
    WriteMemPtr((PVOID)(0x004A09A6 + 2), &JUMP_THRESHOLD);

    /* Fix water deceleration on high FPS */
    WriteMemUInt8Repeat((PVOID)0x0049D816, ASM_NOP, 5);
    WriteMemUInt8Repeat((PVOID)0x0049D82A, ASM_NOP, 5);
    WriteMemUInt8((PVOID)(0x0049D82A + 5), ASM_PUSH_ESI);
    WriteMemPtr((PVOID)(0x0049D830 + 1), (PVOID)((ULONG_PTR)EntityWaterDecelerateFix - (0x0049D830 + 0x5)));
}
