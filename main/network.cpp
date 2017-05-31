#include "stdafx.h"
#include "network.h"
#include "rf.h"
#include "pf.h"
#include "utils.h"

using namespace rf;

constexpr auto MultiIsCurrentServer = (uint8_t(*)(const NwAddr *pAddr))0x0044AD80;

typedef void(*NwPacketHandler_Type)(char *pData, const NwAddr *pAddr);

constexpr auto ProcessGameInfoReqPacket = (NwPacketHandler_Type)0x0047B480;
constexpr auto ProcessGameInfoPacket = (NwPacketHandler_Type)0x0047B2A0;
constexpr auto ProcessJoinReqPacket = (NwPacketHandler_Type)0x0047AC60;
constexpr auto ProcessJoinAcceptPacket = (NwPacketHandler_Type)0x0047A840;
constexpr auto ProcessJoinDenyPacket = (NwPacketHandler_Type)0x0047A400;
constexpr auto ProcessNewPlayerPacket = (NwPacketHandler_Type)0x0047A580;
constexpr auto ProcessPlayersPacket = (NwPacketHandler_Type)0x00481E60;
constexpr auto ProcessLeftGamePacket = (NwPacketHandler_Type)0x0047BBC0;
constexpr auto ProcessEndGamePacket = (NwPacketHandler_Type)0x0047BAB0;
constexpr auto ProcessStateInfoReqPacket = (NwPacketHandler_Type)0x00481BB0;
constexpr auto ProcessStateInfoDonePacket = (NwPacketHandler_Type)0x00481AF0;
constexpr auto ProcessClientInGamePacket = (NwPacketHandler_Type)0x004820D0;
constexpr auto ProcessChatLinePacket = (NwPacketHandler_Type)0x00444860;
constexpr auto ProcessNameChangePacket = (NwPacketHandler_Type)0x0046EAE0;
constexpr auto ProcessRespawnReqPacket = (NwPacketHandler_Type)0x00480A20;
constexpr auto ProcessTriggerActivatePacket = (NwPacketHandler_Type)0x004831D0;
constexpr auto ProcessUseKeyPressedPacket = (NwPacketHandler_Type)0x00483260;
constexpr auto ProcessPregameBooleanPacket = (NwPacketHandler_Type)0x004766B0;
constexpr auto ProcessPregameGlassPacket = (NwPacketHandler_Type)0x004767B0;
constexpr auto ProcessPregameRemoteChargePacket = (NwPacketHandler_Type)0x0047F9E0;
constexpr auto ProcessSuicidePacket = (NwPacketHandler_Type)0x00475760;
constexpr auto ProcessEnterLimboPacket = (NwPacketHandler_Type)0x0047C060;
constexpr auto ProcessLeaveLimboPacket = (NwPacketHandler_Type)0x0047C160;
constexpr auto ProcessTeamChangePacket = (NwPacketHandler_Type)0x004825B0;
constexpr auto ProcessPingPacket = (NwPacketHandler_Type)0x00484CE0;
constexpr auto ProcessPongPacket = (NwPacketHandler_Type)0x00484D50;
constexpr auto ProcessNetgameUpdatePacket = (NwPacketHandler_Type)0x00484F40;
constexpr auto ProcessRateChangePacket = (NwPacketHandler_Type)0x004807B0;
constexpr auto ProcessSelectWeaponReqPacket = (NwPacketHandler_Type)0x00485920;
constexpr auto ProcessClutterUpdatePacket = (NwPacketHandler_Type)0x0047F1A0;
constexpr auto ProcessClutterKillPacket = (NwPacketHandler_Type)0x0047F380;
constexpr auto ProcessCtfFlagPickedUpPacket = (NwPacketHandler_Type)0x00474040;
constexpr auto ProcessCtfFlagCapturedPacket = (NwPacketHandler_Type)0x004742E0;
constexpr auto ProcessCtfFlagUpdatePacket = (NwPacketHandler_Type)0x00474810;
constexpr auto ProcessCtfFlagReturnedPacket = (NwPacketHandler_Type)0x00474420;
constexpr auto ProcessCtfFlagDroppedPacket = (NwPacketHandler_Type)0x00474D70;
constexpr auto ProcessRemoteChargeKillPacket = (NwPacketHandler_Type)0x00485BC0;
constexpr auto ProcessItemUpdatePacket = (NwPacketHandler_Type)0x0047A220;
constexpr auto ProcessObjUpdatePacket = (NwPacketHandler_Type)0x0047DF90;
constexpr auto ProcessObjKillPacket = (NwPacketHandler_Type)0x0047EDE0;
constexpr auto ProcessItemApplyPacket = (NwPacketHandler_Type)0x004798D0;
constexpr auto ProcessBooleanPacket = (NwPacketHandler_Type)0x00476590;
constexpr auto ProcessRespawnPacket = (NwPacketHandler_Type)0x004799E0;
constexpr auto ProcessEntityCreatePacket = (NwPacketHandler_Type)0x00475420;
constexpr auto ProcessItemCreatePacket = (NwPacketHandler_Type)0x00479F70;
constexpr auto ProcessReloadPacket = (NwPacketHandler_Type)0x00485AB0;
constexpr auto ProcessReloadReqPacket = (NwPacketHandler_Type)0x00485A60;
constexpr auto ProcessWeaponFirePacket = (NwPacketHandler_Type)0x0047D6C0;
constexpr auto ProcessFallDamagePacket = (NwPacketHandler_Type)0x00476370;
constexpr auto ProcessRconReqPacket = (NwPacketHandler_Type)0x0046C520;
constexpr auto ProcessRconPacket = (NwPacketHandler_Type)0x0046C6E0;
constexpr auto ProcessSoundPacket = (NwPacketHandler_Type)0x00471FF0;
constexpr auto ProcessTeamScorePacket = (NwPacketHandler_Type)0x00472210;
constexpr auto ProcessGlassKillPacket = (NwPacketHandler_Type)0x00472350;

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

NAKED void ProcessGameInfoPacket_Security_0047B2D3()
{ // ecx - num, esi - source, ebx - dest
    _asm
    {
        pushad
        push 256
        push esi
        push ebx
        call SafeStrCpy
        add esp, 12
        popad
        xor eax, eax
        mov ecx, 0047B2E3h
        jmp ecx
    }
}

NAKED void ProcessGameInfoPacket_Security_0047B334()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        push edx
        push 256
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        pop edx
        xor eax, eax
        mov ecx, 0047B342h
        jmp ecx
    }
}

NAKED void ProcessGameInfoPacket_Security_0047B38E()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        pushad
        push 256
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        popad
        xor eax, eax
        mov ecx, 0047B39Ch
        jmp ecx
    }
}

NAKED void ProcessJoinReqPacket_Security_0047AD4E()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        pushad
        push 256
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        popad
        mov ecx, 0047AD5Ah
        jmp ecx
    }
}

NAKED void ProcessJoinAcceptPacket_Security_0047A8AE()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        pushad
        push 256
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        popad
        mov ecx, 0047A8BAh
        jmp ecx
    }
}

NAKED void ProcessNewPlayerPacket_Security_0047A5F4()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        pushad
        push 256
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        popad
        mov byte ptr[esp + 12Ch - 118h], bl
        mov ecx, 0047A604h
        jmp ecx
    }
}

NAKED void ProcessPlayersPacket_Security_00481EE6()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        pushad
        push 256
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        popad
        xor eax, eax
        mov ecx, 00481EF4h
        jmp ecx
    }
}

NAKED void ProcessStateInfoReqPacket_Security_00481BEC()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        pushad
        push 256
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        popad
        mov al, byte ptr 0x0064EC40 // g_GameOptions
        mov ecx, 00481BFDh
        jmp ecx
    }
}

NAKED void ProcessChatLinePacket_Security_004448B0()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        pushad
        push 256
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        popad
        cmp bl, 0FFh
        mov ecx, 004448BFh
        jmp ecx
    }
}

NAKED void ProcessNameChangePacket_Security_0046EB24()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        pushad
        push 256
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        popad
        mov ecx, 0046EB30h
        jmp ecx
    }
}

NAKED void ProcessLeaveLimboPacket_Security_0047C1C3()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        pushad
        push 256
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        popad
        mov ecx, 0047C1CFh
        jmp ecx
    }
}

NAKED void ProcessObjKillPacket_Security_0047EE6E()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        pushad
        push 256
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        popad
        xor eax, eax
        push 0
        mov ecx, 0047EE7Eh
        jmp ecx
    }
}

NAKED void ProcessEntityCreatePacket_Security_00475474()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        pushad
        push 256
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        popad
        xor eax, eax
        mov ecx, 00475482h
        jmp ecx
    }
}

NAKED void ProcessItemCreatePacket_Security_00479FAA()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        pushad
        push 256
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        popad
        xor eax, eax
        mov ecx, 00479FB8h
        jmp ecx
    }
}

NAKED void ProcessRconReqPacket_Security_0046C590()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        pushad
        push 256
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        popad
        lea eax, [esp + 110h - 100h]
        mov ecx, 0046C5A0h
        jmp ecx
    }
}

NAKED void ProcessRconPacket_Security_0046C751()
{ // ecx - num, esi -source, edi - dest
    _asm
    {
        pushad
        push 512
        push esi
        push edi
        call SafeStrCpy
        add esp, 12
        popad
        xor eax, eax
        mov ecx, 0046C75Fh
        jmp ecx
    }
}


void ProcessGameInfoReqPacket_New(char *pData, const NwAddr *pAddr)
{
    if (*g_pbLocalNetworkGame) // server-side
        ProcessGameInfoReqPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessGameInfoPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessGameInfoPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessJoinReqPacket_New(char *pData, const NwAddr *pAddr)
{
    if (*g_pbLocalNetworkGame) // server-side
        ProcessJoinReqPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessJoinAcceptPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessJoinAcceptPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessJoinDenyPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame && MultiIsCurrentServer(pAddr)) // client-side
        ProcessJoinDenyPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessNewPlayerPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
    {
        if (GetForegroundWindow() != *g_phWnd)
            Beep(750, 300);
        ProcessNewPlayerPacket_Hook.callTrampoline(pData, pAddr);
    }
}

void ProcessPlayersPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessPlayersPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessLeftGamePacket_New(char *pData, const NwAddr *pAddr)
{
    // server-side and client-side
    if (*g_pbLocalNetworkGame)
    {
        CPlayer *pSrcPlayer = NwGetPlayerFromAddr(pAddr);
        pData[0] = pSrcPlayer->pNwData->PlayerId; // fix player ID
    }
    ProcessLeftGamePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessEndGamePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessEndGamePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessStateInfoReqPacket_New(char *pData, const NwAddr *pAddr)
{
    if (*g_pbLocalNetworkGame) // server-side
        ProcessStateInfoReqPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessStateInfoDonePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessStateInfoDonePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessClientInGamePacket_New(char *pData, const NwAddr *pAddr)
{
    if (*g_pbLocalNetworkGame) // server-side
        ProcessClientInGamePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessChatLinePacket_New(char *pData, const NwAddr *pAddr)
{
    // server-side and client-side
    if (*g_pbLocalNetworkGame)
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
    if (*g_pbLocalNetworkGame)
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
    if (*g_pbLocalNetworkGame) // server-side
        ProcessRespawnReqPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessTriggerActivatePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessTriggerActivatePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessUseKeyPressedPacket_New(char *pData, const NwAddr *pAddr)
{
    if (*g_pbLocalNetworkGame) // server-side
        ProcessUseKeyPressedPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessPregameBooleanPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessPregameBooleanPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessPregameGlassPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessPregameGlassPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessPregameRemoteChargePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessPregameRemoteChargePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessSuicidePacket_New(char *pData, const NwAddr *pAddr)
{
    if (*g_pbLocalNetworkGame) // server-side
        ProcessSuicidePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessEnterLimboPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessEnterLimboPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessLeaveLimboPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessLeaveLimboPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessTeamChangePacket_New(char *pData, const NwAddr *pAddr)
{
    // server-side and client-side
    if (*g_pbLocalNetworkGame)
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
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessPingPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessPongPacket_New(char *pData, const NwAddr *pAddr)
{
    if (*g_pbLocalNetworkGame) // server-side
        ProcessPongPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessNetgameUpdatePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessNetgameUpdatePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessRateChangePacket_New(char *pData, const NwAddr *pAddr)
{
    // server-side and client-side?
    if (*g_pbLocalNetworkGame)
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
    if (*g_pbLocalNetworkGame) // server-side
        ProcessSelectWeaponReqPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessClutterUpdatePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessClutterUpdatePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessClutterKillPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessClutterKillPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessCtfFlagPickedUpPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessCtfFlagPickedUpPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessCtfFlagCapturedPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessCtfFlagCapturedPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessCtfFlagUpdatePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessCtfFlagUpdatePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessCtfFlagReturnedPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessCtfFlagReturnedPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessCtfFlagDroppedPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessCtfFlagDroppedPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessRemoteChargeKillPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessRemoteChargeKillPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessItemUpdatePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessItemUpdatePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessObjUpdatePacket_New(char *pData, const NwAddr *pAddr)
{
    // server-side and client-side
    ProcessObjUpdatePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessObjKillPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessObjKillPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessItemApplyPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessItemApplyPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessBooleanPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessBooleanPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessRespawnPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessRespawnPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessEntityCreatePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
    {
        // Update Default Player Weapon if server has it overriden
        size_t NameSize = strlen(pData) + 1;
        uint8_t PlayerId = pData[NameSize + 58];
        if (PlayerId == (*g_ppLocalPlayer)->pNwData->PlayerId)
        {
            int32_t WeaponClsId = *(int32_t*)(pData + NameSize + 63);
            CString_Assign(g_pstrDefaultPlayerWeapon, &g_pWeaponClasses[WeaponClsId].strName);
            DcPrintf("spawn weapon %d", WeaponClsId);
        }

        ProcessEntityCreatePacket_Hook.callTrampoline(pData, pAddr);
    }
}

void ProcessItemCreatePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessItemCreatePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessReloadPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
    {
        // Update ClipSize and MaxAmmo if received values are greater than values from local weapons.tbl
        int WeaponClsId = *((int32_t *)pData + 1);
        int ClipAmmo = *((int32_t *)pData + 2);
        int Ammo = *((int32_t *)pData + 3);
        if (g_pWeaponClasses[WeaponClsId].ClipSize < Ammo)
            g_pWeaponClasses[WeaponClsId].ClipSize = Ammo;
        if (g_pWeaponClasses[WeaponClsId].cMaxAmmo < ClipAmmo)
            g_pWeaponClasses[WeaponClsId].cMaxAmmo = ClipAmmo;
        DcPrintf("ProcessReloadPacket WeaponClsId %d ClipAmmo %d Ammo %d", WeaponClsId, ClipAmmo, Ammo);

        // Call original handler
        ProcessReloadPacket_Hook.callTrampoline(pData, pAddr);
    }
}

void ProcessReloadReqPacket_New(char *pData, const NwAddr *pAddr)
{
    if (*g_pbLocalNetworkGame) // server-side
        ProcessReloadReqPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessWeaponFirePacket_New(char *pData, const NwAddr *pAddr)
{
    // server-side and client-side
    ProcessWeaponFirePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessFallDamagePacket_New(char *pData, const NwAddr *pAddr)
{
    if (*g_pbLocalNetworkGame) // server-side
        ProcessFallDamagePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessRconReqPacket_New(char *pData, const NwAddr *pAddr)
{
    if (*g_pbLocalNetworkGame) // server-side
        ProcessRconReqPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessRconPacket_New(char *pData, const NwAddr *pAddr)
{
    if (*g_pbLocalNetworkGame) // server-side
        ProcessRconPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessSoundPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessSoundPacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessTeamScorePacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessTeamScorePacket_Hook.callTrampoline(pData, pAddr);
}

void ProcessGlassKillPacket_New(char *pData, const NwAddr *pAddr)
{
    if (!*g_pbLocalNetworkGame) // client-side
        ProcessGlassKillPacket_Hook.callTrampoline(pData, pAddr);
}

EntityObj *SecureObjUpdatePacket(EntityObj *pEntity, uint8_t Flags, CPlayer *pSrcPlayer)
{
    if (*g_pbLocalNetworkGame)
    {
        // server-side
        if (pEntity && pEntity->_Super.Handle != pSrcPlayer->hEntity)
            return nullptr;
        if (Flags & (0x4 | 0x20 | 0x80)) // OUF_WEAPON_TYPE | OUF_HEALTH_ARMOR | OUF_ARMOR_STATE
            return nullptr;
    }
    return pEntity;
}

void NetworkInit()
{
    /* ProcessGamePackets hook (not reliable only) */
    WriteMemInt32(0x00479245, (uintptr_t)ProcessUnreliableGamePacketsHook - (0x00479244 + 0x5));

    /* Improve SimultaneousPing */
    *g_pSimultaneousPing = 32;

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
    AsmWritter(0x00481D31, 0x00481D33).xor(AsmRegs::EAX, AsmRegs::EAX);
    AsmWritter(0x00481D40, 0x00481D44).xor(AsmRegs::EDX, AsmRegs::EDX);
    // Hide IP addresses in New Player packet
    AsmWritter(0x0047A4A0, 0x0047A4A2). xor (AsmRegs::EDX, AsmRegs::EDX);
    AsmWritter(0x0047A4A6, 0x0047A4AA). xor (AsmRegs::ECX, AsmRegs::ECX);
}
