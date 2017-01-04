#include "network.h"
#include "rf.h"
#include "utils.h"

static void ProcessUnreliableGamePacketsHook(const char *pData, int cbData, void *pAddr, void *pPlayer)
{
    RfProcessGamePackets(pData, cbData, pAddr, pPlayer);

#if MASK_AS_PF
    ProcessPfPacket(pData, cbData, pAddr, pPlayer);
#endif
}

static void HandleNewPlayerPacketHook(BYTE *pData, NET_ADDR *pAddr)
{
    if (*g_pbNetworkGame && !*g_pbLocalNetworkGame && GetForegroundWindow() != *g_phWnd)
        Beep(750, 300);
    RfHandleNewPlayerPacket(pData, pAddr);
}

void NetworkInit()
{
    /* ProcessGamePackets hook (not reliable only) */
    WriteMemPtr((PVOID)0x00479245, (PVOID)((ULONG_PTR)ProcessUnreliableGamePacketsHook - (0x00479244 + 0x5)));

    /* Improve SimultaneousPing */
    *g_pSimultaneousPing = 32;

    /* Allow ports < 1023 (especially 0 - any port) */
    WriteMemUInt8Repeat((PVOID)0x00528F24, 0x90, 2);

    /* Default port: 0 */
    WriteMemUInt16((PVOID)0x0059CDE4, 0);

    /* If server forces player character, don't save it in settings */
    WriteMemUInt8Repeat((PVOID)0x004755C1, ASM_NOP, 6);
    WriteMemUInt8Repeat((PVOID)0x004755C7, ASM_NOP, 6);

    /* Show valid info for servers with incompatible version */
    WriteMemUInt8((PVOID)0x0047B3CB, ASM_SHORT_JMP_REL);

    /* Beep when new player joins */
    WriteMemPtr((PVOID)0x0059E158, HandleNewPlayerPacketHook);
}