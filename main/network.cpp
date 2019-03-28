#include "stdafx.h"
#include "network.h"
#include "rf.h"
#include "utils.h"
#include "inline_asm.h"
#include <ShortTypes.h>
#include <FunHook2.h>
#include <CallHook2.h>
#include <RegsPatch.h>

#if MASK_AS_PF
 #include "pf.h"
#endif

namespace rf {
static const auto MpIsConnectingToServer = AddrAsRef<uint8_t(const rf::NwAddr& addr)>(0x0044AD80);
static auto& RflStaticGeometry = AddrAsRef<void*>(0x006460E8);
static auto& SimultaneousPing = AddrAsRef<uint32_t>(0x00599CD8);

typedef void(*NwProcessGamePackets_Type)(const char *Data, int cbData, const NwAddr &Addr, Player *Player);
static const auto NwProcessGamePackets = (NwProcessGamePackets_Type)0x004790D0;
}

typedef void NwPacketHandler_Type(char* data, const rf::NwAddr& addr);

//#define TEST_BUFFER_OVERFLOW_FIXES

CallHook2<void(const char*, int, const rf::NwAddr&, rf::Player*)> ProcessUnreliableGamePackets_Hook{
    0x00479244,
    [](const char* data, int data_len, const rf::NwAddr& addr, rf::Player* player) {
        rf::NwProcessGamePackets(data, data_len, addr, player);

    #if MASK_AS_PF
        ProcessPfPacket(data, data_len, &addr, player);
    #endif
    }
};

extern "C" void SafeStrCpy(char* dest, const char* src, size_t dest_size)
{
#ifdef TEST_BUFFER_OVERFLOW_FIXES
    strcpy(dest, "test");
#else
    strncpy(dest, src, dest_size);
    dest[dest_size - 1] = 0;
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

RegsPatch ProcessGameInfoPacket_GameTypeBounds_Patch{
    0x0047B30B,
    [](X86Regs& regs) {
        regs.ecx = std::max(regs.ecx, 0);
        regs.ecx = std::min(regs.ecx, 2);
    }
};

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

FunHook2<NwPacketHandler_Type> ProcessGameInfoReqPacket_Hook{
    0x0047B480,
    [](char* data, const rf::NwAddr& addr) {
        if (rf::g_IsLocalNetworkGame) // server-side
            ProcessGameInfoReqPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessGameInfoPacket_Hook{
    0x0047B2A0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessGameInfoPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessJoinReqPacket_Hook{
    0x0047AC60,
    [](char* data, const rf::NwAddr& addr) {
        if (rf::g_IsLocalNetworkGame) // server-side
            ProcessJoinReqPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessJoinAcceptPacket_Hook{
    0x0047A840,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessJoinAcceptPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessJoinDenyPacket_Hook{
    0x0047A400,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame && rf::MpIsConnectingToServer(addr)) // client-side
            ProcessJoinDenyPacket_Hook.CallTarget(data, addr);
    }
};
FunHook2<NwPacketHandler_Type> ProcessNewPlayerPacket_Hook{
    0x0047A580,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) { // client-side
            if (GetForegroundWindow() != rf::g_hWnd)
                Beep(750, 300);
            ProcessNewPlayerPacket_Hook.CallTarget(data, addr);
        }
    }
};

FunHook2<NwPacketHandler_Type> ProcessPlayersPacket_Hook{
    0x00481E60,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessPlayersPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessLeftGamePacket_Hook{
    0x0047BBC0,
    [](char* data, const rf::NwAddr& addr) {
        // server-side and client-side
        if (rf::g_IsLocalNetworkGame) {
            rf::Player* src_player = rf::NwGetPlayerFromAddr(addr);
            data[0] = src_player->NwData->PlayerId; // fix player ID
        }
        ProcessLeftGamePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessEndGamePacket_Hook{
    0x0047BAB0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessEndGamePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessStateInfoReqPacket_Hook{
    0x00481BB0,
    [](char* data, const rf::NwAddr& addr) {
        if (rf::g_IsLocalNetworkGame) // server-side
            ProcessStateInfoReqPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessStateInfoDonePacket_Hook{
    0x00481AF0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessStateInfoDonePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessClientInGamePacket_Hook{
    0x004820D0,
    [](char* data, const rf::NwAddr& addr) {
        if (rf::g_IsLocalNetworkGame) // server-side
            ProcessClientInGamePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessChatLinePacket_Hook{
    0x00444860,
    [](char* data, const rf::NwAddr& addr) {
        // server-side and client-side
        if (rf::g_IsLocalNetworkGame) {
            rf::Player *src_player = rf::NwGetPlayerFromAddr(addr);
            if (!src_player)
                return; // shouldnt happen (protected in rf::NwProcessGamePackets)
            data[0] = src_player->NwData->PlayerId; // fix player ID
        }
        ProcessChatLinePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessNameChangePacket_Hook{
    0x0046EAE0,
    [](char* data, const rf::NwAddr& addr) {
        // server-side and client-side
        if (rf::g_IsLocalNetworkGame) {
            rf::Player* src_player = rf::NwGetPlayerFromAddr(addr);
            if (!src_player)
                return; // shouldnt happen (protected in rf::NwProcessGamePackets)
            data[0] = src_player->NwData->PlayerId; // fix player ID
        }
        ProcessNameChangePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessRespawnReqPacket_Hook{
    0x00480A20,
    [](char* data, const rf::NwAddr& addr) {
        if (rf::g_IsLocalNetworkGame) // server-side
            ProcessRespawnReqPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessTriggerActivatePacket_Hook{
    0x004831D0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessTriggerActivatePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessUseKeyPressedPacket_Hook{
    0x00483260,
    [](char* data, const rf::NwAddr& addr) {
        if (rf::g_IsLocalNetworkGame) // server-side
            ProcessUseKeyPressedPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessPregameBooleanPacket_Hook{
    0x004766B0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessPregameBooleanPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessPregameGlassPacket_Hook{
    0x004767B0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessPregameGlassPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessPregameRemoteChargePacket_Hook{
    0x0047F9E0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessPregameRemoteChargePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessSuicidePacket_Hook{
    0x00475760,
    [](char* data, const rf::NwAddr& addr) {
        if (rf::g_IsLocalNetworkGame) // server-side
            ProcessSuicidePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessEnterLimboPacket_Hook{
    0x0047C060,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessEnterLimboPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessLeaveLimboPacket_Hook{
    0x0047C160,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessLeaveLimboPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessTeamChangePacket_Hook{
    0x004825B0,
    [](char* data, const rf::NwAddr& addr) {
        // server-side and client-side
        if (rf::g_IsLocalNetworkGame) {
            rf::Player *src_player = rf::NwGetPlayerFromAddr(addr);
            if (!src_player)
                return; // shouldnt happen (protected in rf::NwProcessGamePackets)
            data[0] = src_player->NwData->PlayerId; // fix player ID
            data[1] = clamp((int)data[1], 0, 1); // team validation (fixes "green team")
        }
        ProcessTeamChangePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessPingPacket_Hook{
    0x00484CE0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessPingPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessPongPacket_Hook{
    0x00484D50,
    [](char* data, const rf::NwAddr& addr) {
        if (rf::g_IsLocalNetworkGame) // server-side
            ProcessPongPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessNetgameUpdatePacket_Hook{
    0x00484F40,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessNetgameUpdatePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessRateChangePacket_Hook{
    0x004807B0,
    [](char* data, const rf::NwAddr& addr) {
        // server-side and client-side?
        if (rf::g_IsLocalNetworkGame) {
            rf::Player *src_player = rf::NwGetPlayerFromAddr(addr);
            if (!src_player)
                return; // shouldnt happen (protected in rf::NwProcessGamePackets)
            data[0] = src_player->NwData->PlayerId; // fix player ID
        }
        ProcessRateChangePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessSelectWeaponReqPacket_Hook{
    0x00485920,
    [](char* data, const rf::NwAddr& addr) {
        if (rf::g_IsLocalNetworkGame) // server-side
            ProcessSelectWeaponReqPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessClutterUpdatePacket_Hook{
    0x0047F1A0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessClutterUpdatePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessClutterKillPacket_Hook{
    0x0047F380,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessClutterKillPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessCtfFlagPickedUpPacket_Hook{
    0x00474040,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessCtfFlagPickedUpPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessCtfFlagCapturedPacket_Hook{
    0x004742E0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessCtfFlagCapturedPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessCtfFlagUpdatePacket_Hook{
    0x00474810,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessCtfFlagUpdatePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessCtfFlagReturnedPacket_Hook{
    0x00474420,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessCtfFlagReturnedPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessCtfFlagDroppedPacket_Hook{
    0x00474D70,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessCtfFlagDroppedPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessRemoteChargeKillPacket_Hook{
    0x00485BC0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessRemoteChargeKillPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessItemUpdatePacket_Hook{
    0x0047A220,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessItemUpdatePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessObjUpdatePacket_Hook{
    0x0047DF90,
    [](char* data, const rf::NwAddr& addr) {
        // server-side and client-side
        ProcessObjUpdatePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessObjKillPacket_Hook{
    0x0047EDE0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessObjKillPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessItemApplyPacket_Hook{
    0x004798D0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessItemApplyPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessBooleanPacket_Hook{
    0x00476590,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessBooleanPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessRespawnPacket_Hook{
    0x004799E0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessRespawnPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessEntityCreatePacket_Hook{
    0x00475420,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) { // client-side
            // Update Default Player Weapon if server has it overriden
            size_t name_size = strlen(data) + 1;
            uint8_t player_id = data[name_size + 58];
            if (player_id == rf::g_LocalPlayer->NwData->PlayerId) {
                int32_t weapon_cls_id = *(int32_t*)(data + name_size + 63);
                rf::g_strDefaultPlayerWeapon = rf::g_WeaponClasses[weapon_cls_id].strName;

#if 0 // disabled because it sometimes helpful feature to switch to last used weapon
    // Reset next weapon variable so entity wont switch after pickup
            if (!g_LocalPlayer->Config.AutoswitchWeapons)
                MultiSetNextWeapon(weapon_cls_id);
#endif

                TRACE("spawn weapon %d", weapon_cls_id);
            }

            ProcessEntityCreatePacket_Hook.CallTarget(data, addr);
        }
    }
};

FunHook2<NwPacketHandler_Type> ProcessItemCreatePacket_Hook{
    0x00479F70,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessItemCreatePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessReloadPacket_Hook{
    0x00485AB0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) { // client-side
            // Update ClipSize and MaxAmmo if received values are greater than values from local weapons.tbl
            int WeaponClsId = *((int32_t*)data + 1);
            int ClipAmmo = *((int32_t*)data + 2);
            int Ammo = *((int32_t*)data + 3);
            if (rf::g_WeaponClasses[WeaponClsId].ClipSize < Ammo)
                rf::g_WeaponClasses[WeaponClsId].ClipSize = Ammo;
            if (rf::g_WeaponClasses[WeaponClsId].cMaxAmmo < ClipAmmo)
                rf::g_WeaponClasses[WeaponClsId].cMaxAmmo = ClipAmmo;
            TRACE("ProcessReloadPacket WeaponClsId %d ClipAmmo %d Ammo %d", WeaponClsId, ClipAmmo, Ammo);

            // Call original handler
            ProcessReloadPacket_Hook.CallTarget(data, addr);
        }
    }
};

FunHook2<NwPacketHandler_Type> ProcessReloadReqPacket_Hook{
    0x00485A60,
    [](char* data, const rf::NwAddr& addr) {
        if (rf::g_IsLocalNetworkGame) // server-side
            ProcessReloadReqPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessWeaponFirePacket_Hook{
    0x0047D6C0,
    [](char* data, const rf::NwAddr& addr) {
        // server-side and client-side
        ProcessWeaponFirePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessFallDamagePacket_Hook{
    0x00476370,
    [](char* data, const rf::NwAddr& addr) {
        if (rf::g_IsLocalNetworkGame) // server-side
            ProcessFallDamagePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessRconReqPacket_Hook{
    0x0046C520,
    [](char* data, const rf::NwAddr& addr) {
        if (rf::g_IsLocalNetworkGame) // server-side
            ProcessRconReqPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessRconPacket_Hook{
    0x0046C6E0,
    [](char* data, const rf::NwAddr& addr) {
        if (rf::g_IsLocalNetworkGame) // server-side
            ProcessRconPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessSoundPacket_Hook{
    0x00471FF0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessSoundPacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessTeamScorePacket_Hook{
    0x00472210,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessTeamScorePacket_Hook.CallTarget(data, addr);
    }
};

FunHook2<NwPacketHandler_Type> ProcessGlassKillPacket_Hook{
    0x00472350,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::g_IsLocalNetworkGame) // client-side
            ProcessGlassKillPacket_Hook.CallTarget(data, addr);
    }
};

rf::EntityObj* SecureObjUpdatePacket(rf::EntityObj *entity, uint8_t flags, rf::Player *src_player)
{
    if (rf::g_IsLocalNetworkGame) {
        // server-side
        if (entity && entity->_Super.Handle != src_player->hEntity) {
            TRACE("Invalid ObjUpdate entity %x %x %s", entity->_Super.Handle, src_player->hEntity, src_player->strName.CStr());
            return nullptr;
        }

        if (flags & (0x4 | 0x20 | 0x80)) { // OUF_WEAPON_TYPE | OUF_HEALTH_ARMOR | OUF_ARMOR_STATE
            TRACE("Invalid ObjUpdate flags %x", flags);
            return nullptr;
        }
    }
    return entity;
}

FunHook2<uint8_t()> MultiAllocPlayerId_Hook{
    0x0046EF00,
    []() {
        uint8_t player_id = MultiAllocPlayerId_Hook.CallTarget();
        if (player_id == 0xFF)
            player_id = MultiAllocPlayerId_Hook.CallTarget();
        return player_id;
    }
};

constexpr int MULTI_HANDLE_MAPPING_ARRAY_SIZE = 1024;

FunHook2<rf::Object*(int32_t)> MultiGetObjFromRemoteHandle_Hook{
    0x00484B00,
    [](int32_t remote_handle) {
        int index = static_cast<uint16_t>(remote_handle);
        if (index >= MULTI_HANDLE_MAPPING_ARRAY_SIZE)
            return static_cast<rf::Object*>(nullptr);
        return MultiGetObjFromRemoteHandle_Hook.CallTarget(remote_handle);
    }
};

FunHook2<int32_t(int32_t)> MultiGetLocalHandleFromRemoteHandle_Hook{
    0x00484B30,
    [](int32_t remote_handle) {
        int index = static_cast<uint16_t>(remote_handle);
        if (index >= MULTI_HANDLE_MAPPING_ARRAY_SIZE)
            return -1;
        return MultiGetLocalHandleFromRemoteHandle_Hook.CallTarget(remote_handle);
    }
};

FunHook2<void(int32_t, int32_t)> MultiSetObjHandleMapping_Hook{
    0x00484B70,
    [](int32_t remote_handle, int32_t local_handle) {
        int index = static_cast<uint16_t>(remote_handle);
        if (index >= MULTI_HANDLE_MAPPING_ARRAY_SIZE)
            return;
        MultiSetObjHandleMapping_Hook.CallTarget(remote_handle, local_handle);
    }
};

RegsPatch ProcessBooleanPacket_ValidateMeshId_Patch{
    0x004765A3,
    [](auto& regs) {
        regs.ecx = std::max(regs.ecx, 0);
        regs.ecx = std::min(regs.ecx, 3);
    }
};

RegsPatch ProcessBooleanPacket_ValidateRoomId_Patch{
    0x0047661C,
    [](auto &regs) {
        int num_rooms = StructFieldRef<int>(rf::RflStaticGeometry, 0x90);
        if (regs.edx < 0 || regs.edx >= num_rooms) {
            WARN("Invalid room in Boolean packet - skipping");
            regs.esp += 0x64;
            regs.eip = 0x004766A5;
        }
    }
};

RegsPatch ProcessPregameBooleanPacket_ValidateMeshId_Patch{
    0x0047672F,
    [](auto &regs) {
        regs.ecx = std::max(regs.ecx, 0);
        regs.ecx = std::min(regs.ecx, 3);
    }
};

RegsPatch ProcessPregameBooleanPacket_ValidateRoomId_Patch{
    0x00476752,
    [](auto &regs) {
        int num_rooms = StructFieldRef<int>(rf::RflStaticGeometry, 0x90);
        if (regs.edx < 0 || regs.edx >= num_rooms) {
            WARN("Invalid room in PregameBoolean packet - skipping");
            regs.esp += 0x68;
            regs.eip = 0x004767AA;
        }
    }
};

RegsPatch ProcessGlassKillPacket_CheckRoomExists_Patch{
    0x004723B3,
    [](auto &regs) {
        if (!regs.eax)
            regs.eip = 0x004723EC;
    }
};

void NetworkInit()
{
    /* ProcessGamePackets hook (not reliable only) */
    ProcessUnreliableGamePackets_Hook.Install();

    /* Improve SimultaneousPing */
    rf::SimultaneousPing = 32;

    /* Change server info timeout to 2s */
    WriteMem<u32>(0x0044D357 + 2, 2000);

    /* Change delay between server info requests */
    WriteMem<u8>(0x0044D338 + 1, 20);

    /* Allow ports < 1023 (especially 0 - any port) */
    AsmWritter(0x00528F24).nop(2);

    /* Default port: 0 */
    WriteMem<u16>(0x0059CDE4, 0);
    WriteMem<i32>(0x004B159D + 1, 0); // TODO: add setting in launcher

    /* Dont overwrite MpCharacter in Single Player */
    AsmWritter(0x004A415F).nop(10);

    /* Show valid info for servers with incompatible version */
    WriteMem<u8>(0x0047B3CB, ASM_SHORT_JMP_REL);

    /* Change default Server List sort to players count */
    WriteMem<u32>(0x00599D20, 4);

    /* Buffer Overflow fixes */
    AsmWritter(0x0047B2D3).jmp(ProcessGameInfoPacket_Security_0047B2D3);
    AsmWritter(0x0047B334).jmp(ProcessGameInfoPacket_Security_0047B334);
#ifndef TEST_BUFFER_OVERFLOW_FIXES
    AsmWritter(0x0047B38E).jmp(ProcessGameInfoPacket_Security_0047B38E);
#endif
    AsmWritter(0x0047AD4E).jmp(ProcessJoinReqPacket_Security_0047AD4E);
    AsmWritter(0x0047A8AE).jmp(ProcessJoinAcceptPacket_Security_0047A8AE);
    AsmWritter(0x0047A5F4).jmp(ProcessNewPlayerPacket_Security_0047A5F4);
    AsmWritter(0x00481EE6).jmp(ProcessPlayersPacket_Security_00481EE6);
    AsmWritter(0x00481BEC).jmp(ProcessStateInfoReqPacket_Security_00481BEC);
    AsmWritter(0x004448B0).jmp(ProcessChatLinePacket_Security_004448B0);
    AsmWritter(0x0046EB24).jmp(ProcessNameChangePacket_Security_0046EB24);
    AsmWritter(0x0047C1C3).jmp(ProcessLeaveLimboPacket_Security_0047C1C3);
    AsmWritter(0x0047EE6E).jmp(ProcessObjKillPacket_Security_0047EE6E);
    AsmWritter(0x00475474).jmp(ProcessEntityCreatePacket_Security_00475474);
    AsmWritter(0x00479FAA).jmp(ProcessItemCreatePacket_Security_00479FAA);
    AsmWritter(0x0046C590).jmp(ProcessRconReqPacket_Security_0046C590);
    AsmWritter(0x0046C751).jmp(ProcessRconPacket_Security_0046C751);

    // Hook all packet handlers
    ProcessGameInfoReqPacket_Hook.Install();
    ProcessGameInfoPacket_Hook.Install();
    ProcessJoinReqPacket_Hook.Install();
    ProcessJoinAcceptPacket_Hook.Install();
    ProcessJoinDenyPacket_Hook.Install();
    ProcessNewPlayerPacket_Hook.Install();
    ProcessPlayersPacket_Hook.Install();
    ProcessLeftGamePacket_Hook.Install();
    ProcessEndGamePacket_Hook.Install();
    ProcessStateInfoReqPacket_Hook.Install();
    ProcessStateInfoDonePacket_Hook.Install();
    ProcessClientInGamePacket_Hook.Install();
    ProcessChatLinePacket_Hook.Install();
    ProcessNameChangePacket_Hook.Install();
    ProcessRespawnReqPacket_Hook.Install();
    ProcessTriggerActivatePacket_Hook.Install();
    ProcessUseKeyPressedPacket_Hook.Install();
    ProcessPregameBooleanPacket_Hook.Install();
    ProcessPregameGlassPacket_Hook.Install();
    ProcessPregameRemoteChargePacket_Hook.Install();
    ProcessSuicidePacket_Hook.Install();
    ProcessEnterLimboPacket_Hook.Install(); // not needed
    ProcessLeaveLimboPacket_Hook.Install();
    ProcessTeamChangePacket_Hook.Install();
    ProcessPingPacket_Hook.Install();
    ProcessPongPacket_Hook.Install();
    ProcessNetgameUpdatePacket_Hook.Install();
    ProcessRateChangePacket_Hook.Install();
    ProcessSelectWeaponReqPacket_Hook.Install();
    ProcessClutterUpdatePacket_Hook.Install();
    ProcessClutterKillPacket_Hook.Install();
    ProcessCtfFlagPickedUpPacket_Hook.Install(); // not needed
    ProcessCtfFlagCapturedPacket_Hook.Install(); // not needed
    ProcessCtfFlagUpdatePacket_Hook.Install(); // not needed
    ProcessCtfFlagReturnedPacket_Hook.Install(); // not needed
    ProcessCtfFlagDroppedPacket_Hook.Install(); // not needed
    ProcessRemoteChargeKillPacket_Hook.Install();
    ProcessItemUpdatePacket_Hook.Install();
    ProcessObjUpdatePacket_Hook.Install();
    ProcessObjKillPacket_Hook.Install();
    ProcessItemApplyPacket_Hook.Install();
    ProcessBooleanPacket_Hook.Install();
    ProcessRespawnPacket_Hook.Install();
    ProcessEntityCreatePacket_Hook.Install();
    ProcessItemCreatePacket_Hook.Install();
    ProcessReloadPacket_Hook.Install();
    ProcessReloadReqPacket_Hook.Install();
    ProcessWeaponFirePacket_Hook.Install();
    ProcessFallDamagePacket_Hook.Install();
    ProcessRconReqPacket_Hook.Install(); // not needed
    ProcessRconPacket_Hook.Install(); // not needed
    ProcessSoundPacket_Hook.Install();
    ProcessTeamScorePacket_Hook.Install();
    ProcessGlassKillPacket_Hook.Install();

    // Fix ObjUpdate packet handling
    AsmWritter(0x0047E058, 0x0047E06A)
        .mov(asm_regs::eax, AsmMem(asm_regs::esp + (0x9C - 0x6C))) // Player
        .push(asm_regs::eax)
        .push(asm_regs::ebx)
        .push(asm_regs::edi)
        .call(SecureObjUpdatePacket)
        .add(asm_regs::esp, 12)
        .mov(asm_regs::edi, asm_regs::eax);

    // Client-side green team fix
    AsmWritter(0x0046CAD7, 0x0046CADA).cmp(asm_regs::al, (int8_t)0xFF);

    // Hide IP addresses in Players packet
    AsmWritter(0x00481D31, 0x00481D33).xor_(asm_regs::eax, asm_regs::eax);
    AsmWritter(0x00481D40, 0x00481D44).xor_(asm_regs::edx, asm_regs::edx);
    // Hide IP addresses in New Player packet
    AsmWritter(0x0047A4A0, 0x0047A4A2).xor_(asm_regs::edx, asm_regs::edx);
    AsmWritter(0x0047A4A6, 0x0047A4AA).xor_(asm_regs::ecx, asm_regs::ecx);

    // Fix "Orion bug" - default 'miner1' entity spawning client-side periodically
    MultiAllocPlayerId_Hook.Install();

    // Fix buffer-overflows in multi handle mapping
    MultiGetObjFromRemoteHandle_Hook.Install();
    MultiGetLocalHandleFromRemoteHandle_Hook.Install();
    MultiSetObjHandleMapping_Hook.Install();

    // Fix GameType out of bounds vulnerability in GameInfo packet
    ProcessGameInfoPacket_GameTypeBounds_Patch.Install();

    // Fix MeshId out of bounds vulnerability in Boolean packet
    ProcessBooleanPacket_ValidateMeshId_Patch.Install();

    // Fix RoomId out of bounds vulnerability in Boolean packet
    ProcessBooleanPacket_ValidateRoomId_Patch.Install();

    // Fix MeshId out of bounds vulnerability in PregameBoolean packet
    ProcessPregameBooleanPacket_ValidateMeshId_Patch.Install();

    // Fix RoomId out of bounds vulnerability in PregameBoolean packet
    ProcessPregameBooleanPacket_ValidateRoomId_Patch.Install();

    // Fix crash if room does not exist in GlassKill packet
    ProcessGlassKillPacket_CheckRoomExists_Patch.Install();
}
