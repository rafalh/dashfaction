#include "camera.h"
#include "rf.h"
#include "utils.h"
#include "config.h"

#if CAMERA_1_3_COMMANDS

int CanPlayerFireHook(CPlayer *pPlayer)
{
    if(!(pPlayer->field_10 & 0x10))
        return 0;
    if(*g_pbNetworkGame && (pPlayer->pCamera->Type == RF_CAM_FREE || pPlayer->pCamera->pPlayer != pPlayer))
        return 0;
    return 1;
}

#endif // if CAMERA_1_3_COMMANDS

void InitCamera(void)
{
#if CAMERA_1_3_COMMANDS
    /* Enable camera1-3 in multiplayer and hook CanPlayerFire to disable shooting in camera2 */
    WriteMemUInt8Repeat((PVOID)0x00431280, ASM_NOP, 2);
    WriteMemUInt8Repeat((PVOID)0x004312E0, ASM_NOP, 2);
    WriteMemUInt8Repeat((PVOID)0x00431340, ASM_NOP, 2);
    WriteMemUInt8((PVOID)0x004A68D0, ASM_LONG_JMP_REL);
    WriteMemUInt32((PVOID)0x004A68D1, ((ULONG_PTR)CanPlayerFireHook) - (0x004A68D0 + 0x5));
#endif // if CAMERA_1_3_COMMANDS
}
