#include "stdafx.h"
#include "network.h"
#include "rf.h"
#include "utils.h"
#include "inline_asm.h"
#include "FunHook2.h"

#if MASK_AS_PF
 #include "pf.h"
#endif

using namespace rf;

static const auto MultiIsCurrentServer = (uint8_t(*)(const NwAddr *pAddr))0x0044AD80;

typedef void(*NwPacketHandler_Type)(char *pData, const NwAddr *pAddr);

static const auto ProcessGameInfoReqPacket = (NwPacketHandler_Type)0x0047B480;
static const auto ProcessGameInfoPacket = (NwPacketHandler_Type)0x0047B2A0;
static const auto ProcessJoinReqPacket = (NwPacketHandler_Type)0x0047AC60;
static const auto ProcessJoinAcceptPacket = (NwPacketHandler_Type)0x0047A840;
static const auto ProcessJoinDenyPacket = (NwPacketHandler_Type)0x0047A400;
static const auto ProcessNewPlayerPacket = (NwPacketHandler_Type)0x0047A580;
static const auto ProcessPlayersPacket = (NwPacketHandler_Type)0x00481E60;
static const auto ProcessLeftGamePacket = (NwPacketHandler_Type)0x0047BBC0;
static const auto ProcessEndGamePacket = (NwPacketHandler_Type)0x0047BAB0;
static const auto ProcessStateInfoReqPacket = (NwPacketHandler_Type)0x00481BB0;
static const auto ProcessStateInfoDonePacket = (NwPacketHandler_Type)0x00481AF0;
static const auto ProcessClientInGamePacket = (NwPacketHandler_Type)0x004820D0;
static const auto ProcessChatLinePacket = (NwPacketHandler_Type)0x00444860;
static const auto ProcessNameChangePacket = (NwPacketHandler_Type)0x0046EAE0;
static const auto ProcessRespawnReqPacket = (NwPacketHandler_Type)0x00480A20;
static const auto ProcessTriggerActivatePacket = (NwPacketHandler_Type)0x004831D0;
static const auto ProcessUseKeyPressedPacket = (NwPacketHandler_Type)0x00483260;
static const auto ProcessPregameBooleanPacket = (NwPacketHandler_Type)0x004766B0;
static const auto ProcessPregameGlassPacket = (NwPacketHandler_Type)0x004767B0;
static const auto ProcessPregameRemoteChargePacket = (NwPacketHandler_Type)0x0047F9E0;
static const auto ProcessSuicidePacket = (NwPacketHandler_Type)0x00475760;
static const auto ProcessEnterLimboPacket = (NwPacketHandler_Type)0x0047C060;
static const auto ProcessLeaveLimboPacket = (NwPacketHandler_Type)0x0047C160;
static const auto ProcessTeamChangePacket = (NwPacketHandler_Type)0x004825B0;
static const auto ProcessPingPacket = (NwPacketHandler_Type)0x00484CE0;
static const auto ProcessPongPacket = (NwPacketHandler_Type)0x00484D50;
static const auto ProcessNetgameUpdatePacket = (NwPacketHandler_Type)0x00484F40;
static const auto ProcessRateChangePacket = (NwPacketHandler_Type)0x004807B0;
static const auto ProcessSelectWeaponReqPacket = (NwPacketHandler_Type)0x00485920;
static const auto ProcessClutterUpdatePacket = (NwPacketHandler_Type)0x0047F1A0;
static const auto ProcessClutterKillPacket = (NwPacketHandler_Type)0x0047F380;
static const auto ProcessCtfFlagPickedUpPacket = (NwPacketHandler_Type)0x00474040;
static const auto ProcessCtfFlagCapturedPacket = (NwPacketHandler_Type)0x004742E0;
static const auto ProcessCtfFlagUpdatePacket = (NwPacketHandler_Type)0x00474810;
static const auto ProcessCtfFlagReturnedPacket = (NwPacketHandler_Type)0x00474420;
static const auto ProcessCtfFlagDroppedPacket = (NwPacketHandler_Type)0x00474D70;
static const auto ProcessRemoteChargeKillPacket = (NwPacketHandler_Type)0x00485BC0;
static const auto ProcessItemUpdatePacket = (NwPacketHandler_Type)0x0047A220;
static const auto ProcessObjUpdatePacket = (NwPacketHandler_Type)0x0047DF90;
static const auto ProcessObjKillPacket = (NwPacketHandler_Type)0x0047EDE0;
static const auto ProcessItemApplyPacket = (NwPacketHandler_Type)0x004798D0;
static const auto ProcessBooleanPacket = (NwPacketHandler_Type)0x00476590;
static const auto ProcessRespawnPacket = (NwPacketHandler_Type)0x004799E0;
static const auto ProcessEntityCreatePacket = (NwPacketHandler_Type)0x00475420;
static const auto ProcessItemCreatePacket = (NwPacketHandler_Type)0x00479F70;
static const auto ProcessReloadPacket = (NwPacketHandler_Type)0x00485AB0;
static const auto ProcessReloadReqPacket = (NwPacketHandler_Type)0x00485A60;
static const auto ProcessWeaponFirePacket = (NwPacketHandler_Type)0x0047D6C0;
static const auto ProcessFallDamagePacket = (NwPacketHandler_Type)0x00476370;
static const auto ProcessRconReqPacket = (NwPacketHandler_Type)0x0046C520;
static const auto ProcessRconPacket = (NwPacketHandler_Type)0x0046C6E0;
static const auto ProcessSoundPacket = (NwPacketHandler_Type)0x00471FF0;
static const auto ProcessTeamScorePacket = (NwPacketHandler_Type)0x00472210;
static const auto ProcessGlassKillPacket = (NwPacketHandler_Type)0x00472350;

auto ProcessGameInfoReqPacket_Hook = makeFunHook(ProcessGameInfoReqPacket);
auto ProcessGameInfoPacket_Hook = makeFunHook(ProcessGameInfoPacket);
auto ProcessJoinReqPacket_Hook = makeFunHook(ProcessJoinReqPacket);
auto ProcessJoinAcceptPacket_Hook = makeFunHook(ProcessJoinAcceptPacket);
auto ProcessJoinDenyPacket_Hook = makeFunHook(ProcessJoinDenyPacket);
auto ProcessNewPlayerPacket_Hook = makeFunHook(ProcessNewPlayerPacket);
auto ProcessPlayersPacket_Hook = makeFunHook(ProcessPlayersPacket);
auto ProcessLeftGamePacket_Hook = makeFunHook(ProcessLeftGamePacket);
auto ProcessEndGamePacket_Hook = makeFunHook(ProcessEndGamePacket);
auto ProcessStateInfoReqPacket_Hook = makeFunHook(ProcessStateInfoReqPacket);
auto ProcessStateInfoDonePacket_Hook = makeFunHook(ProcessStateInfoDonePacket);
auto ProcessClientInGamePacket_Hook = makeFunHook(ProcessClientInGamePacket);
auto ProcessChatLinePacket_Hook = makeFunHook(ProcessChatLinePacket);
auto ProcessNameChangePacket_Hook = makeFunHook(ProcessNameChangePacket);
auto ProcessRespawnReqPacket_Hook = makeFunHook(ProcessRespawnReqPacket);
auto ProcessTriggerActivatePacket_Hook = makeFunHook(ProcessTriggerActivatePacket);
auto ProcessUseKeyPressedPacket_Hook = makeFunHook(ProcessUseKeyPressedPacket);
auto ProcessPregameBooleanPacket_Hook = makeFunHook(ProcessPregameBooleanPacket);
auto ProcessPregameGlassPacket_Hook = makeFunHook(ProcessPregameGlassPacket);
auto ProcessPregameRemoteChargePacket_Hook = makeFunHook(ProcessPregameRemoteChargePacket);
auto ProcessSuicidePacket_Hook = makeFunHook(ProcessSuicidePacket);
auto ProcessEnterLimboPacket_Hook = makeFunHook(ProcessEnterLimboPacket);
auto ProcessLeaveLimboPacket_Hook = makeFunHook(ProcessLeaveLimboPacket);
auto ProcessTeamChangePacket_Hook = makeFunHook(ProcessTeamChangePacket);
auto ProcessPingPacket_Hook = makeFunHook(ProcessPingPacket);
auto ProcessPongPacket_Hook = makeFunHook(ProcessPongPacket);
auto ProcessNetgameUpdatePacket_Hook = makeFunHook(ProcessNetgameUpdatePacket);
auto ProcessRateChangePacket_Hook = makeFunHook(ProcessRateChangePacket);
auto ProcessSelectWeaponReqPacket_Hook = makeFunHook(ProcessSelectWeaponReqPacket);
auto ProcessClutterUpdatePacket_Hook = makeFunHook(ProcessClutterUpdatePacket);
auto ProcessClutterKillPacket_Hook = makeFunHook(ProcessClutterKillPacket);
auto ProcessCtfFlagPickedUpPacket_Hook = makeFunHook(ProcessCtfFlagPickedUpPacket);
auto ProcessCtfFlagCapturedPacket_Hook = makeFunHook(ProcessCtfFlagCapturedPacket);
auto ProcessCtfFlagUpdatePacket_Hook = makeFunHook(ProcessCtfFlagUpdatePacket);
auto ProcessCtfFlagReturnedPacket_Hook = makeFunHook(ProcessCtfFlagReturnedPacket);
auto ProcessCtfFlagDroppedPacket_Hook = makeFunHook(ProcessCtfFlagDroppedPacket);
auto ProcessRemoteChargeKillPacket_Hook = makeFunHook(ProcessRemoteChargeKillPacket);
auto ProcessItemUpdatePacket_Hook = makeFunHook(ProcessItemUpdatePacket);
auto ProcessObjUpdatePacket_Hook = makeFunHook(ProcessObjUpdatePacket);
auto ProcessObjKillPacket_Hook = makeFunHook(ProcessObjKillPacket);
auto ProcessItemApplyPacket_Hook = makeFunHook(ProcessItemApplyPacket);
auto ProcessBooleanPacket_Hook = makeFunHook(ProcessBooleanPacket);
auto ProcessRespawnPacket_Hook = makeFunHook(ProcessRespawnPacket);
auto ProcessEntityCreatePacket_Hook = makeFunHook(ProcessEntityCreatePacket);
auto ProcessItemCreatePacket_Hook = makeFunHook(ProcessItemCreatePacket);
auto ProcessReloadPacket_Hook = makeFunHook(ProcessReloadPacket);
auto ProcessReloadReqPacket_Hook = makeFunHook(ProcessReloadReqPacket);
auto ProcessWeaponFirePacket_Hook = makeFunHook(ProcessWeaponFirePacket);
auto ProcessFallDamagePacket_Hook = makeFunHook(ProcessFallDamagePacket);
auto ProcessRconReqPacket_Hook = makeFunHook(ProcessRconReqPacket);
auto ProcessRconPacket_Hook = makeFunHook(ProcessRconPacket);
auto ProcessSoundPacket_Hook = makeFunHook(ProcessSoundPacket);
auto ProcessTeamScorePacket_Hook = makeFunHook(ProcessTeamScorePacket);
auto ProcessGlassKillPacket_Hook = makeFunHook(ProcessGlassKillPacket);

//#define TEST_BUFFER_OVERFLOW_FIXES

static void ProcessUnreliableGamePacketsHook(const char *pData, int cbData, const NwAddr *pAddr, CPlayer *pPlayer)
{
    NwProcessGamePackets(pData, cbData, pAddr, pPlayer);

#if MASK_AS_PF
    ProcessPfPacket(pData, cbData, pAddr, pPlayer);
#endif
}

extern "C" void SafeStrCpy(char *pDest, const char *pSrc, size_t DestSize)
{
#ifdef TEST_BUFFER_OVERFLOW_FIXES
    strcpy(pDest, "test");
#else
    strncpy(pDest, pSrc, DestSize);
    pDest[DestSize - 1] = 0;
#endif
}

ASM_FUNC(ProcessGameInfoPacket_Security_0047B2D3,
// ecx - num, esi - source, ebx - dest
    ASM_I  pushad
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push ebx
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  xor eax, eax
    ASM_I  mov ecx, 0x0047B2E3
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessGameInfoPacket_Security_0047B334,
// ecx - num, esi -source, edi - dest
    ASM_I  push edx
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  pop edx
    ASM_I  xor eax, eax
    ASM_I  mov ecx, 0x0047B342
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessGameInfoPacket_Security_0047B38E,
// ecx - num, esi -source, edi - dest
    ASM_I  pushad
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  xor eax, eax
    ASM_I  mov ecx, 0x0047B39C
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessJoinReqPacket_Security_0047AD4E,
// ecx - num, esi -source, edi - dest
    ASM_I  pushad
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  mov ecx, 0x0047AD5A
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessJoinAcceptPacket_Security_0047A8AE,
// ecx - num, esi -source, edi - dest
    ASM_I  pushad
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  mov ecx, 0x0047A8BA
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessNewPlayerPacket_Security_0047A5F4,
// ecx - num, esi -source, edi - dest
    ASM_I  pushad
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  mov byte ptr[esp + 0x12C - 0x118], bl
    ASM_I  mov ecx, 0x0047A604
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessPlayersPacket_Security_00481EE6,
// ecx - num, esi -source, edi - dest
    ASM_I  pushad
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  xor eax, eax
    ASM_I  mov ecx, 0x00481EF4
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessStateInfoReqPacket_Security_00481BEC,
// ecx - num, esi -source, edi - dest
    ASM_I  pushad
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  mov ecx, 0x0064EC40
    ASM_I  mov al, [0x0064EC40] // g_GameOptions
    ASM_I  mov ecx, 0x00481BFD
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessChatLinePacket_Security_004448B0,
// ecx - num, esi -source, edi - dest
    ASM_I  pushad
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  cmp bl, 0x0FF
    ASM_I  mov ecx, 0x004448BF
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessNameChangePacket_Security_0046EB24,
// ecx - num, esi -source, edi - dest
    ASM_I  pushad
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  mov ecx, 0x0046EB30
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessLeaveLimboPacket_Security_0047C1C3,
// ecx - num, esi -source, edi - dest
    ASM_I  pushad
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  mov ecx, 0x0047C1CF
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessObjKillPacket_Security_0047EE6E,
// ecx - num, esi -source, edi - dest
    ASM_I  pushad
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  xor eax, eax
    ASM_I  push 0
    ASM_I  mov ecx, 0x0047EE7E
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessEntityCreatePacket_Security_00475474,
// ecx - num, esi -source, edi - dest
    ASM_I  pushad
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  xor eax, eax
    ASM_I  mov ecx, 0x00475482
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessItemCreatePacket_Security_00479FAA,
// ecx - num, esi -source, edi - dest
    ASM_I  pushad
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  xor eax, eax
    ASM_I  mov ecx, 0x00479FB8
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessRconReqPacket_Security_0046C590,
// ecx - num, esi -source, edi - dest
    ASM_I  pushad
    ASM_I  push 256
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  lea eax, [esp + 0x110 - 0x100]
    ASM_I  mov ecx, 0x0046C5A0
    ASM_I  jmp ecx
)

ASM_FUNC(ProcessRconPacket_Security_0046C751,
// ecx - num, esi -source, edi - dest
    ASM_I  pushad
    ASM_I  push 512
    ASM_I  push esi
    ASM_I  push edi
    ASM_I  call ASM_SYM(SafeStrCpy)
    ASM_I  add esp, 12
    ASM_I  popad
    ASM_I  xor eax, eax
    ASM_I  mov ecx, 0x0046C75F
    ASM_I  jmp ecx
)

void ProcessGameInfoReqPacket_New(char *pData, const NwAddr *pAddr)
{
    if (g_bLocalNetworkGame) // server-side
        ProcessGameInfoReqPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessGameInfoPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessGameInfoPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessJoinReqPacket_New(char *pData, const NwAddr *pAddr)
{
    if (g_bLocalNetworkGame) // server-side
        ProcessJoinReqPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessJoinAcceptPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessJoinAcceptPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessJoinDenyPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame && MultiIsCurrentServer(pAddr)) // client-side
        ProcessJoinDenyPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessNewPlayerPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
    {
        if (GetForegroundWindow() != g_hWnd)
            Beep(750, 300);
        ProcessNewPlayerPacket_Hook.callTrampoline(pData, pAddr);
    }
}

void ProcessPlayersPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessPlayersPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessLeftGamePacket_New(char *pData, const NwAddr *pAddr)
{
    // server-side and client-side
    if (g_bLocalNetworkGame)
    {
        CPlayer *pSrcPlayer = NwGetPlayerFromAddr(pAddr);
        pData[0] = pSrcPlayer->pNwData->PlayerId; // fix player ID
    }
    ProcessLeftGamePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessEndGamePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessEndGamePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessStateInfoReqPacket_New(char *pData, const NwAddr *pAddr)
{
    if (g_bLocalNetworkGame) // server-side
        ProcessStateInfoReqPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessStateInfoDonePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessStateInfoDonePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessClientInGamePacket_New(char *pData, const NwAddr *pAddr)
{
    if (g_bLocalNetworkGame) // server-side
        ProcessClientInGamePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessChatLinePacket_New(char *pData, const NwAddr *pAddr)
{
    // server-side and client-side
    if (g_bLocalNetworkGame)
    {
        CPlayer *pSrcPlayer = NwGetPlayerFromAddr(pAddr);
        if (!pSrcPlayer)
            return; // shouldnt happen (protected in NwProcessGamePackets)
        pData[0] = pSrcPlayer->pNwData->PlayerId; // fix player ID
    }
    ProcessChatLinePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessNameChangePacket_New(char *pData, const NwAddr *pAddr)
{
    // server-side and client-side
    if (g_bLocalNetworkGame)
    {
        CPlayer *pSrcPlayer = NwGetPlayerFromAddr(pAddr);
        if (!pSrcPlayer)
            return; // shouldnt happen (protected in NwProcessGamePackets)
        pData[0] = pSrcPlayer->pNwData->PlayerId; // fix player ID
    }
    ProcessNameChangePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessRespawnReqPacket_New(char *pData, const NwAddr *pAddr)
{
    if (g_bLocalNetworkGame) // server-side
        ProcessRespawnReqPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessTriggerActivatePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessTriggerActivatePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessUseKeyPressedPacket_New(char *pData, const NwAddr *pAddr)
{
    if (g_bLocalNetworkGame) // server-side
        ProcessUseKeyPressedPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessPregameBooleanPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessPregameBooleanPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessPregameGlassPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessPregameGlassPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessPregameRemoteChargePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessPregameRemoteChargePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessSuicidePacket_New(char *pData, const NwAddr *pAddr)
{
    if (g_bLocalNetworkGame) // server-side
        ProcessSuicidePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessEnterLimboPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessEnterLimboPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessLeaveLimboPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessLeaveLimboPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessTeamChangePacket_New(char *pData, const NwAddr *pAddr)
{
    // server-side and client-side
    if (g_bLocalNetworkGame)
    {
        CPlayer *pSrcPlayer = NwGetPlayerFromAddr(pAddr);
        if (!pSrcPlayer)
            return; // shouldnt happen (protected in NwProcessGamePackets)
        pData[0] = pSrcPlayer->pNwData->PlayerId; // fix player ID
        pData[1] = clamp((int)pData[1], 0, 1); // team validation (fixes "green team")
    }
    ProcessTeamChangePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessPingPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessPingPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessPongPacket_New(char *pData, const NwAddr *pAddr)
{
    if (g_bLocalNetworkGame) // server-side
        ProcessPongPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessNetgameUpdatePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessNetgameUpdatePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessRateChangePacket_New(char *pData, const NwAddr *pAddr)
{
    // server-side and client-side?
    if (g_bLocalNetworkGame)
    {
        CPlayer *pSrcPlayer = NwGetPlayerFromAddr(pAddr);
        if (!pSrcPlayer)
            return; // shouldnt happen (protected in NwProcessGamePackets)
        pData[0] = pSrcPlayer->pNwData->PlayerId; // fix player ID
    }
    ProcessRateChangePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessSelectWeaponReqPacket_New(char *pData, const NwAddr *pAddr)
{
    if (g_bLocalNetworkGame) // server-side
        ProcessSelectWeaponReqPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessClutterUpdatePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessClutterUpdatePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessClutterKillPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessClutterKillPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessCtfFlagPickedUpPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessCtfFlagPickedUpPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessCtfFlagCapturedPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessCtfFlagCapturedPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessCtfFlagUpdatePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessCtfFlagUpdatePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessCtfFlagReturnedPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessCtfFlagReturnedPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessCtfFlagDroppedPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessCtfFlagDroppedPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessRemoteChargeKillPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessRemoteChargeKillPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessItemUpdatePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessItemUpdatePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessObjUpdatePacket_New(char *pData, const NwAddr *pAddr)
{
    // server-side and client-side
    ProcessObjUpdatePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessObjKillPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessObjKillPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessItemApplyPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessItemApplyPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessBooleanPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessBooleanPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessRespawnPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessRespawnPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessEntityCreatePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
    {
        // Update Default Player Weapon if server has it overriden
        size_t NameSize = strlen(pData) + 1;
        uint8_t PlayerId = pData[NameSize + 58];
        if (PlayerId == g_pLocalPlayer->pNwData->PlayerId)
        {
            int32_t WeaponClsId = *(int32_t*)(pData + NameSize + 63);
            CString_Assign(&g_strDefaultPlayerWeapon, &g_pWeaponClasses[WeaponClsId].strName);

#if 0 // disabled because it sometimes helpful feature to switch to last used weapon
            // Reset next weapon variable so entity wont switch after pickup
            if (!g_pLocalPlayer->Config.bAutoswitchWeapons)
                MultiSetNextWeapon(WeaponClsId);
#endif

            TRACE("spawn weapon %d", WeaponClsId);
        }

        ProcessEntityCreatePacket_Hook.callTrampoline(pData, pAddr);
    }
}

void ProcessItemCreatePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessItemCreatePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessReloadPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
    {
        // Update ClipSize and MaxAmmo if received values are greater than values from local weapons.tbl
        int WeaponClsId = *((int32_t *)pData + 1);
        int ClipAmmo = *((int32_t *)pData + 2);
        int Ammo = *((int32_t *)pData + 3);
        if (g_pWeaponClasses[WeaponClsId].ClipSize < Ammo)
            g_pWeaponClasses[WeaponClsId].ClipSize = Ammo;
        if (g_pWeaponClasses[WeaponClsId].cMaxAmmo < ClipAmmo)
            g_pWeaponClasses[WeaponClsId].cMaxAmmo = ClipAmmo;
        TRACE("ProcessReloadPacket WeaponClsId %d ClipAmmo %d Ammo %d", WeaponClsId, ClipAmmo, Ammo);

        // Call original handler
        ProcessReloadPacket_Hook.callTrampoline(pData, pAddr);
    }
}

void ProcessReloadReqPacket_New(char *pData, const NwAddr *pAddr)
{
    if (g_bLocalNetworkGame) // server-side
        ProcessReloadReqPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessWeaponFirePacket_New(char *pData, const NwAddr *pAddr)
{
    // server-side and client-side
    ProcessWeaponFirePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessFallDamagePacket_New(char *pData, const NwAddr *pAddr)
{
    if (g_bLocalNetworkGame) // server-side
        ProcessFallDamagePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessRconReqPacket_New(char *pData, const NwAddr *pAddr)
{
    if (g_bLocalNetworkGame) // server-side
        ProcessRconReqPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessRconPacket_New(char *pData, const NwAddr *pAddr)
{
    if (g_bLocalNetworkGame) // server-side
        ProcessRconPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessSoundPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessSoundPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessTeamScorePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessTeamScorePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessGlassKillPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!g_bLocalNetworkGame) // client-side
        ProcessGlassKillPacket_Hook.callTrampoline(pData, pAddr);
}

EntityObj *SecureObjUpdatePacket(EntityObj *pEntity, uint8_t Flags, CPlayer *pSrcPlayer)
{
    if (g_bLocalNetworkGame)
    {
        // server-side
        if (pEntity && pEntity->_Super.Handle != pSrcPlayer->hEntity)
        {
            TRACE("Invalid ObjUpdate entity %x %x %s", pEntity->_Super.Handle, pSrcPlayer->hEntity, pSrcPlayer->strName.psz);
            return nullptr;
        }

        if (Flags & (0x4 | 0x20 | 0x80)) // OUF_WEAPON_TYPE | OUF_HEALTH_ARMOR | OUF_ARMOR_STATE
        {
            TRACE("Invalid ObjUpdate flags %x", Flags);
            return nullptr;
        }
    }
    return pEntity;
}

FunHook2<uint8_t()> MultiAllocPlayerId_Hook{ 0x0046EF00,
    []() {
        uint8_t PlayerId = MultiAllocPlayerId_Hook.CallTarget();
        if (PlayerId == 0xFF)
            PlayerId = MultiAllocPlayerId_Hook.CallTarget();
        return PlayerId;
    }
};

constexpr int MULTI_HANDLE_MAPPING_ARRAY_SIZE = 1024;

FunHook2<rf::Object*(int32_t)> MultiGetObjFromRemoteHandle_Hook{ 0x00484B00,
    [](int32_t RemoteHandle) {
        int Index = static_cast<uint16_t>(RemoteHandle);
        if (Index >= MULTI_HANDLE_MAPPING_ARRAY_SIZE)
            return static_cast<rf::Object*>(nullptr);
        return MultiGetObjFromRemoteHandle_Hook.CallTarget(RemoteHandle);
    }
};

FunHook2<int32_t(int32_t)> MultiGetLocalHandleFromRemoteHandle_Hook{ 0x00484B30,
    [](int32_t RemoteHandle) {
        int Index = static_cast<uint16_t>(RemoteHandle);
        if (Index >= MULTI_HANDLE_MAPPING_ARRAY_SIZE)
            return -1;
        return MultiGetLocalHandleFromRemoteHandle_Hook.CallTarget(RemoteHandle);
    }
};

FunHook2<void(int32_t, int32_t)> MultiSetObjHandleMapping_Hook{ 0x00484B70,
    [](int32_t RemoteHandle, int32_t LocalHandle) {
        int Index = static_cast<uint16_t>(RemoteHandle);
        if (Index >= MULTI_HANDLE_MAPPING_ARRAY_SIZE)
            return;
        MultiSetObjHandleMapping_Hook.CallTarget(RemoteHandle, LocalHandle);
    }
};

void NetworkInit()
{
    /* ProcessGamePackets hook (not reliable only) */
    WriteMemInt32(0x00479245, (uintptr_t)ProcessUnreliableGamePacketsHook - (0x00479244 + 0x5));

    /* Improve SimultaneousPing */
    g_SimultaneousPing = 32;

    /* Allow ports < 1023 (especially 0 - any port) */
    WriteMemUInt8(0x00528F24, 0x90, 2);

    /* Default port: 0 */
    WriteMemUInt16(0x0059CDE4, 0);
    WriteMemInt32(0x004B159D + 1, 0); // TODO: add setting in launcher

    /* Dont overwrite MpCharacter in Single Player */
    WriteMemUInt8(0x004A415F, ASM_NOP, 10);

    /* Show valid info for servers with incompatible version */
    WriteMemUInt8(0x0047B3CB, ASM_SHORT_JMP_REL);

    /* Change default Server List sort to players count */
    WriteMemUInt32(0x00599D20, 4);

    /* Buffer Overflow fixes */
    AsmWritter(0x0047B2D3).jmpLong(ProcessGameInfoPacket_Security_0047B2D3);
    AsmWritter(0x0047B334).jmpLong(ProcessGameInfoPacket_Security_0047B334);
#ifndef TEST_BUFFER_OVERFLOW_FIXES
    AsmWritter(0x0047B38E).jmpLong(ProcessGameInfoPacket_Security_0047B38E);
#endif
    AsmWritter(0x0047AD4E).jmpLong(ProcessJoinReqPacket_Security_0047AD4E);
    AsmWritter(0x0047A8AE).jmpLong(ProcessJoinAcceptPacket_Security_0047A8AE);
    AsmWritter(0x0047A5F4).jmpLong(ProcessNewPlayerPacket_Security_0047A5F4);
    AsmWritter(0x00481EE6).jmpLong(ProcessPlayersPacket_Security_00481EE6);
    AsmWritter(0x00481BEC).jmpLong(ProcessStateInfoReqPacket_Security_00481BEC);
    AsmWritter(0x004448B0).jmpLong(ProcessChatLinePacket_Security_004448B0);
    AsmWritter(0x0046EB24).jmpLong(ProcessNameChangePacket_Security_0046EB24);
    AsmWritter(0x0047C1C3).jmpLong(ProcessLeaveLimboPacket_Security_0047C1C3);
    AsmWritter(0x0047EE6E).jmpLong(ProcessObjKillPacket_Security_0047EE6E);
    AsmWritter(0x00475474).jmpLong(ProcessEntityCreatePacket_Security_00475474);
    AsmWritter(0x00479FAA).jmpLong(ProcessItemCreatePacket_Security_00479FAA);
    AsmWritter(0x0046C590).jmpLong(ProcessRconReqPacket_Security_0046C590);
    AsmWritter(0x0046C751).jmpLong(ProcessRconPacket_Security_0046C751);

    // Hook all packet handlers
    ProcessGameInfoReqPacket_Hook.hook(ProcessGameInfoReqPacket_New);
    ProcessGameInfoPacket_Hook.hook(ProcessGameInfoPacket_New);
    ProcessJoinReqPacket_Hook.hook(ProcessJoinReqPacket_New);
    ProcessJoinAcceptPacket_Hook.hook(ProcessJoinAcceptPacket_New);
    ProcessJoinDenyPacket_Hook.hook(ProcessJoinDenyPacket_New);
    ProcessNewPlayerPacket_Hook.hook(ProcessNewPlayerPacket_New);
    ProcessPlayersPacket_Hook.hook(ProcessPlayersPacket_New);
    ProcessLeftGamePacket_Hook.hook(ProcessLeftGamePacket_New);
    ProcessEndGamePacket_Hook.hook(ProcessEndGamePacket_New);
    ProcessStateInfoReqPacket_Hook.hook(ProcessStateInfoReqPacket_New);
    ProcessStateInfoDonePacket_Hook.hook(ProcessStateInfoDonePacket_New);
    ProcessClientInGamePacket_Hook.hook(ProcessClientInGamePacket_New);
    ProcessChatLinePacket_Hook.hook(ProcessChatLinePacket_New);
    ProcessNameChangePacket_Hook.hook(ProcessNameChangePacket_New);
    ProcessRespawnReqPacket_Hook.hook(ProcessRespawnReqPacket_New);
    ProcessTriggerActivatePacket_Hook.hook(ProcessTriggerActivatePacket_New);
    ProcessUseKeyPressedPacket_Hook.hook(ProcessUseKeyPressedPacket_New);
    ProcessPregameBooleanPacket_Hook.hook(ProcessPregameBooleanPacket_New);
    ProcessPregameGlassPacket_Hook.hook(ProcessPregameGlassPacket_New);
    ProcessPregameRemoteChargePacket_Hook.hook(ProcessPregameRemoteChargePacket_New);
    ProcessSuicidePacket_Hook.hook(ProcessSuicidePacket_New);
    ProcessEnterLimboPacket_Hook.hook(ProcessEnterLimboPacket_New); // not needed
    ProcessLeaveLimboPacket_Hook.hook(ProcessLeaveLimboPacket_New);
    ProcessTeamChangePacket_Hook.hook(ProcessTeamChangePacket_New);
    ProcessPingPacket_Hook.hook(ProcessPingPacket_New);
    ProcessPongPacket_Hook.hook(ProcessPongPacket_New);
    ProcessNetgameUpdatePacket_Hook.hook(ProcessNetgameUpdatePacket_New);
    ProcessRateChangePacket_Hook.hook(ProcessRateChangePacket_New);
    ProcessSelectWeaponReqPacket_Hook.hook(ProcessSelectWeaponReqPacket_New);
    ProcessClutterUpdatePacket_Hook.hook(ProcessClutterUpdatePacket_New);
    ProcessClutterKillPacket_Hook.hook(ProcessClutterKillPacket_New);
    ProcessCtfFlagPickedUpPacket_Hook.hook(ProcessCtfFlagPickedUpPacket_New); // not needed
    ProcessCtfFlagCapturedPacket_Hook.hook(ProcessCtfFlagCapturedPacket_New); // not needed
    ProcessCtfFlagUpdatePacket_Hook.hook(ProcessCtfFlagUpdatePacket_New); // not needed
    ProcessCtfFlagReturnedPacket_Hook.hook(ProcessCtfFlagReturnedPacket_New); // not needed
    ProcessCtfFlagDroppedPacket_Hook.hook(ProcessCtfFlagDroppedPacket_New); // not needed
    ProcessRemoteChargeKillPacket_Hook.hook(ProcessRemoteChargeKillPacket_New);
    ProcessItemUpdatePacket_Hook.hook(ProcessItemUpdatePacket_New);
    ProcessObjUpdatePacket_Hook.hook(ProcessObjUpdatePacket_New);
    ProcessObjKillPacket_Hook.hook(ProcessObjKillPacket_New);
    ProcessItemApplyPacket_Hook.hook(ProcessItemApplyPacket_New);
    ProcessBooleanPacket_Hook.hook(ProcessBooleanPacket_New);
    ProcessRespawnPacket_Hook.hook(ProcessRespawnPacket_New);
    ProcessEntityCreatePacket_Hook.hook(ProcessEntityCreatePacket_New);
    ProcessItemCreatePacket_Hook.hook(ProcessItemCreatePacket_New);
    ProcessReloadPacket_Hook.hook(ProcessReloadPacket_New);
    ProcessReloadReqPacket_Hook.hook(ProcessReloadReqPacket_New);
    ProcessWeaponFirePacket_Hook.hook(ProcessWeaponFirePacket_New);
    ProcessFallDamagePacket_Hook.hook(ProcessFallDamagePacket_New);
    ProcessRconReqPacket_Hook.hook(ProcessRconReqPacket_New); // not needed
    ProcessRconPacket_Hook.hook(ProcessRconPacket_New); // not needed
    ProcessSoundPacket_Hook.hook(ProcessSoundPacket_New);
    ProcessTeamScorePacket_Hook.hook(ProcessTeamScorePacket_New);
    ProcessGlassKillPacket_Hook.hook(ProcessGlassKillPacket_New);

    // Fix ObjUpdate packet handling
    AsmWritter(0x0047E058, 0x0047E06A)
        .movEaxMemEsp(0x9C - 0x6C) // pPlayer
        .push(AsmRegs::EAX).push(AsmRegs::EBX).push(AsmRegs::EDI)
        .callLong(SecureObjUpdatePacket)
        .addEsp(12).mov(AsmRegs::EDI, AsmRegs::EAX);

    // Client-side green team fix
    AsmWritter(0x0046CAD7, 0x0046CADA).cmp(AsmRegs::AL, (int8_t)0xFF);

    // Hide IP addresses in Players packet
    AsmWritter(0x00481D31, 0x00481D33).xor_(AsmRegs::EAX, AsmRegs::EAX);
    AsmWritter(0x00481D40, 0x00481D44).xor_(AsmRegs::EDX, AsmRegs::EDX);
    // Hide IP addresses in New Player packet
    AsmWritter(0x0047A4A0, 0x0047A4A2).xor_(AsmRegs::EDX, AsmRegs::EDX);
    AsmWritter(0x0047A4A6, 0x0047A4AA).xor_(AsmRegs::ECX, AsmRegs::ECX);

    // Fix "Orion bug" - default 'miner1' entity spawning client-side periodically
    MultiAllocPlayerId_Hook.Install();

    // Fix buffer-overflows in multi handle mapping
    MultiGetObjFromRemoteHandle_Hook.Install();
    MultiGetLocalHandleFromRemoteHandle_Hook.Install();
    MultiSetObjHandleMapping_Hook.Install();
}
