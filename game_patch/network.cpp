#include "network.h"
#include "rf.h"
#include "stdafx.h"
#include "utils.h"
#include "server/server.h"
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <array>
#include <algorithm>
#include <cstddef>
#include <functional>
#include <common/rfproto.h>
#include <common/version.h>

#if MASK_AS_PF
#include "purefaction/pf.h"
#endif

namespace rf
{
static const auto MpIsConnectingToServer = AddrAsRef<uint8_t(const rf::NwAddr& addr)>(0x0044AD80);

static auto& simultaneous_ping = AddrAsRef<uint32_t>(0x00599CD8);

typedef void NwProcessGamePackets_Type(const void* data, size_t len, const NwAddr& addr, Player* player);
static auto& NwProcessGamePackets = AddrAsRef<NwProcessGamePackets_Type>(0x004790D0);
} // namespace rf

typedef void NwPacketHandler_Type(char* data, const rf::NwAddr& addr);

CallHook<void(const void*, size_t, const rf::NwAddr&, rf::Player*)> ProcessUnreliableGamePackets_hook{
    0x00479244,
    [](const void* data, size_t len, const rf::NwAddr& addr, rf::Player* player) {
        rf::NwProcessGamePackets(data, len, addr, player);

#if MASK_AS_PF
        ProcessPfPacket(data, len, addr, player);
#endif
    },
};

class BufferOverflowPatch
{
private:
    CodeInjection2<std::function<void(X86Regs&)>> m_movsb_patch;
    uintptr_t m_shr_ecx_2_addr;
    uintptr_t m_and_ecx_3_addr;
    int32_t m_buffer_size;
    int32_t m_ret_addr;

public:
    BufferOverflowPatch(uintptr_t shr_ecx_2_addr, uintptr_t and_ecx_3_addr, int32_t buffer_size) :
        m_movsb_patch(and_ecx_3_addr, std::bind(&BufferOverflowPatch::movsb_handler, this, std::placeholders::_1)),
        m_shr_ecx_2_addr(shr_ecx_2_addr), m_and_ecx_3_addr(and_ecx_3_addr), m_buffer_size(buffer_size)
    {}

    void Install()
    {
        const std::byte* shr_ecx_2_ptr = reinterpret_cast<std::byte*>(m_shr_ecx_2_addr);
        const std::byte* and_ecx_3_ptr = reinterpret_cast<std::byte*>(m_and_ecx_3_addr);
        assert(std::memcmp(shr_ecx_2_ptr, "\xC1\xE9\x02", 3) == 0); // shr ecx, 2
        assert(std::memcmp(and_ecx_3_ptr, "\x83\xE1\x03", 3) == 0); // and ecx, 3

        if (std::memcmp(shr_ecx_2_ptr + 3, "\xF3\xA5", 2) == 0) { // rep movsd
            m_movsb_patch.SetAddr(m_shr_ecx_2_addr);
            m_ret_addr = m_shr_ecx_2_addr + 5;
            AsmWritter(m_and_ecx_3_addr, m_and_ecx_3_addr + 3).xor_(asm_regs::ecx, asm_regs::ecx);
        }
        else if (std::memcmp(and_ecx_3_ptr + 3, "\xF3\xA4", 2) == 0) { // rep movsb
            AsmWritter(m_shr_ecx_2_addr, m_shr_ecx_2_addr + 3).xor_(asm_regs::ecx, asm_regs::ecx);
            m_movsb_patch.SetAddr(m_and_ecx_3_addr);
            m_ret_addr = m_and_ecx_3_addr + 5;
        }
        else {
            assert(false);
        }

        m_movsb_patch.Install();
    }

private:
    void movsb_handler(X86Regs& regs)
    {
        auto dst_ptr = reinterpret_cast<char*>(regs.edi);
        auto src_ptr = reinterpret_cast<char*>(regs.esi);
        auto num_bytes = regs.ecx;
        num_bytes = std::min(num_bytes, m_buffer_size - 1);
        std::memcpy(dst_ptr, src_ptr, num_bytes);
        dst_ptr[num_bytes] = '\0';
        regs.eip = m_ret_addr;
    }
};

// Note: player name is limited to 32 because some functions assume it is short (see 0x0046EBA5 for example)
// Note: server browser internal functions use strings safely (see 0x0044DDCA for example)
// Note: level filename was limited to 64 because of VPP format limits
std::array g_buffer_overflow_patches{
    BufferOverflowPatch{0x0047B2D3, 0x0047B2DE, 256}, // ProcessGameInfoPacket (server name)
    BufferOverflowPatch{0x0047B334, 0x0047B33D, 256}, // ProcessGameInfoPacket (level name)
    BufferOverflowPatch{0x0047B38E, 0x0047B397, 256}, // ProcessGameInfoPacket (mod name)
    BufferOverflowPatch{0x0047ACF6, 0x0047AD03, 32},  // ProcessJoinReqPacket (player name)
    BufferOverflowPatch{0x0047AD4E, 0x0047AD55, 256}, // ProcessJoinReqPacket (password)
    BufferOverflowPatch{0x0047A8AE, 0x0047A8B5, 64},  // ProcessJoinAcceptPacket (level filename)
    BufferOverflowPatch{0x0047A5F4, 0x0047A5FF, 32},  // ProcessNewPlayerPacket (player name)
    BufferOverflowPatch{0x00481EE6, 0x00481EEF, 32},  // ProcessPlayersPacket (player name)
    BufferOverflowPatch{0x00481BEC, 0x00481BF8, 64},  // ProcessStateInfoReqPacket (level filename)
    BufferOverflowPatch{0x004448B0, 0x004448B7, 256}, // ProcessChatLinePacket (message)
    BufferOverflowPatch{0x0046EB24, 0x0046EB2B, 32},  // ProcessNameChangePacket (player name)
    BufferOverflowPatch{0x0047C1C3, 0x0047C1CA, 64},  // ProcessLeaveLimboPacket (level filename)
    BufferOverflowPatch{0x0047EE6E, 0x0047EE77, 256}, // ProcessObjKillPacket (item name)
    BufferOverflowPatch{0x0047EF9C, 0x0047EFA5, 256}, // ProcessObjKillPacket (item name)
    BufferOverflowPatch{0x00475474, 0x0047547D, 256}, // ProcessEntityCreatePacket (entity name)
    BufferOverflowPatch{0x00479FAA, 0x00479FB3, 256}, // ProcessItemCreatePacket (item name)
    BufferOverflowPatch{0x0046C590, 0x0046C59B, 256}, // ProcessRconReqPacket (password)
    BufferOverflowPatch{0x0046C751, 0x0046C75A, 512}, // ProcessRconPacket (command)
};

// clang-format off
enum packet_type : uint8_t {
    game_info_request      = 0x00,
    game_info              = 0x01,
    join_request           = 0x02,
    join_accept            = 0x03,
    join_deny              = 0x04,
    new_player             = 0x05,
    players                = 0x06,
    left_game              = 0x07,
    end_game               = 0x08,
    state_info_request     = 0x09,
    state_info_done        = 0x0A,
    client_in_game         = 0x0B,
    chat_line              = 0x0C,
    name_change            = 0x0D,
    respawn_request        = 0x0E,
    trigger_activate       = 0x0F,
    use_key_pressed        = 0x10,
    pregame_boolean        = 0x11,
    pregame_glass          = 0x12,
    pregame_remote_charge  = 0x13,
    suicide                = 0x14,
    enter_limbo            = 0x15, // level end
    leave_limbo            = 0x16, // level change
    team_change            = 0x17,
    ping                   = 0x18,
    pong                   = 0x19,
    netgame_update         = 0x1A, // stats
    rate_change            = 0x1B,
    select_weapon_request  = 0x1C,
    clutter_udate          = 0x1D,
    clutter_kill           = 0x1E,
    ctf_flag_pick_up       = 0x1F,
    ctf_flag_capture       = 0x20,
    ctf_flag_update        = 0x21,
    ctf_flag_return        = 0x22,
    ctf_flag_drop          = 0x23,
    remote_charge_kill     = 0x24,
    item_update            = 0x25,
    obj_update             = 0x26,
    obj_kill               = 0x27,
    item_apply             = 0x28,
    boolean_               = 0x29,
    mover_update           = 0x2A, // unused
    respawn                = 0x2B,
    entity_create          = 0x2C,
    item_create            = 0x2D,
    reload                 = 0x2E,
    reload_request         = 0x2F,
    weapon_fire            = 0x30,
    fall_damage            = 0x31,
    rcon_request           = 0x32,
    rcon                   = 0x33,
    sound                  = 0x34,
    team_score             = 0x35,
    glass_kill             = 0x36,
};

// client -> server
std::array g_server_side_packet_whitelist{
    game_info_request,
    join_request,
    left_game,
    state_info_request,
    client_in_game,
    chat_line,
    name_change,
    respawn_request,
    use_key_pressed,
    suicide,
    team_change,
    pong,
    rate_change,
    select_weapon_request,
    obj_update,
    reload_request,
    weapon_fire,
    fall_damage,
    rcon_request,
    rcon,
};

// server -> client
std::array g_client_side_packet_whitelist{
    game_info,
    join_accept,
    join_deny,
    new_player,
    players,
    left_game,
    end_game,
    state_info_done,
    chat_line,
    name_change,
    trigger_activate,
    pregame_boolean,
    pregame_glass,
    pregame_remote_charge,
    enter_limbo,
    leave_limbo,
    team_change,
    ping,
    netgame_update,
    rate_change,
    clutter_udate,
    clutter_kill,
    ctf_flag_pick_up,
    ctf_flag_capture,
    ctf_flag_update,
    ctf_flag_return,
    ctf_flag_drop,
    remote_charge_kill,
    item_update,
    obj_update,
    obj_kill,
    item_apply,
    boolean_,
    // Note: mover_update packet is sent by PF server. Handler is empty so it is safe to enable it.
    mover_update,
    respawn,
    entity_create,
    item_create,
    reload,
    weapon_fire,
    sound,
    team_score,
    glass_kill,
};
// clang-format on

CodeInjection ProcessGamePacket_whitelist_filter{
    0x0047918D,
    [](auto& regs) {
        bool allowed = false;
        int packet_type = regs.esi;
        if (rf::is_local_net_game) {
            auto& whitelist = g_server_side_packet_whitelist;
            allowed = std::find(whitelist.begin(), whitelist.end(), packet_type) != whitelist.end();
        }
        else {
            auto& whitelist = g_client_side_packet_whitelist;
            allowed = std::find(whitelist.begin(), whitelist.end(), packet_type) != whitelist.end();
        }
        if (!allowed) {
            WARN("Ignoring packet 0x%X", packet_type);
            regs.eip = 0x00479194;
        }
        else {
            TRACE("Processing packet 0x%X", packet_type);
        }
    },
};

CodeInjection ProcessGameInfoPacket_GameTypeBounds_patch{
    0x0047B30B,
    [](X86Regs& regs) {
        // Valid game types are between 0 and 2
        regs.ecx = std::clamp(regs.ecx, 0, 2);
    },
};

FunHook<NwPacketHandler_Type> ProcessJoinDenyPacket_hook{
    0x0047A400,
    [](char* data, const rf::NwAddr& addr) {
        if (rf::MpIsConnectingToServer(addr)) // client-side
            ProcessJoinDenyPacket_hook.CallTarget(data, addr);
    },
};

FunHook<NwPacketHandler_Type> ProcessNewPlayerPacket_hook{
    0x0047A580,
    [](char* data, const rf::NwAddr& addr) {
        if (GetForegroundWindow() != rf::main_wnd)
            Beep(750, 300);
        ProcessNewPlayerPacket_hook.CallTarget(data, addr);
    },
};

FunHook<NwPacketHandler_Type> ProcessLeftGamePacket_hook{
    0x0047BBC0,
    [](char* data, const rf::NwAddr& addr) {
        // server-side and client-side
        if (rf::is_local_net_game) {
            rf::Player* src_player = rf::NwGetPlayerFromAddr(addr);
            data[0] = src_player->nw_data->player_id; // fix player ID
        }
        ProcessLeftGamePacket_hook.CallTarget(data, addr);
    },
};

FunHook<NwPacketHandler_Type> ProcessChatLinePacket_hook{
    0x00444860,
    [](char* data, const rf::NwAddr& addr) {
        // server-side and client-side
        if (rf::is_local_net_game) {
            rf::Player* src_player = rf::NwGetPlayerFromAddr(addr);
            if (!src_player)
                return; // shouldnt happen (protected in rf::NwProcessGamePackets)

            char* msg = data + 2;
            if (CheckServerChatCommand(msg, src_player))
                return;

            data[0] = src_player->nw_data->player_id; // fix player ID
        }
        ProcessChatLinePacket_hook.CallTarget(data, addr);
    },
};

FunHook<NwPacketHandler_Type> ProcessNameChangePacket_hook{
    0x0046EAE0,
    [](char* data, const rf::NwAddr& addr) {
        // server-side and client-side
        if (rf::is_local_net_game) {
            rf::Player* src_player = rf::NwGetPlayerFromAddr(addr);
            if (!src_player)
                return;                               // shouldnt happen (protected in rf::NwProcessGamePackets)
            data[0] = src_player->nw_data->player_id; // fix player ID
        }
        ProcessNameChangePacket_hook.CallTarget(data, addr);
    },
};

FunHook<NwPacketHandler_Type> ProcessTeamChangePacket_hook{
    0x004825B0,
    [](char* data, const rf::NwAddr& addr) {
        // server-side and client-side
        if (rf::is_local_net_game) {
            rf::Player* src_player = rf::NwGetPlayerFromAddr(addr);
            if (!src_player)
                return; // shouldnt happen (protected in rf::NwProcessGamePackets)

            data[0] = src_player->nw_data->player_id;  // fix player ID
            data[1] = std::clamp(data[1], '\0', '\1'); // team validation (fixes "green team")
        }
        ProcessTeamChangePacket_hook.CallTarget(data, addr);
    },
};

FunHook<NwPacketHandler_Type> ProcessRateChangePacket_hook{
    0x004807B0,
    [](char* data, const rf::NwAddr& addr) {
        // server-side and client-side?
        if (rf::is_local_net_game) {
            rf::Player* src_player = rf::NwGetPlayerFromAddr(addr);
            if (!src_player)
                return;                               // shouldnt happen (protected in rf::NwProcessGamePackets)
            data[0] = src_player->nw_data->player_id; // fix player ID
        }
        ProcessRateChangePacket_hook.CallTarget(data, addr);
    },
};

FunHook<NwPacketHandler_Type> ProcessEntityCreatePacket_hook{
    0x00475420,
    [](char* data, const rf::NwAddr& addr) {
        // Temporary change default player weapon to the weapon type from the received packet
        // Created entity always receives Default Player Weapon (from game.tbl) and if server has it overriden
        // player weapons would be in inconsistent state with server without this change.
        size_t name_size = strlen(data) + 1;
        int32_t weapon_cls_id = *reinterpret_cast<int32_t*>(data + name_size + 63);
        auto old_default_player_weapon = rf::default_player_weapon;
        rf::default_player_weapon = rf::weapon_classes[weapon_cls_id].name;
        ProcessEntityCreatePacket_hook.CallTarget(data, addr);
        rf::default_player_weapon = old_default_player_weapon;
    },
};

FunHook<NwPacketHandler_Type> ProcessReloadPacket_hook{
    0x00485AB0,
    [](char* data, const rf::NwAddr& addr) {
        if (!rf::is_local_net_game) { // client-side
            // Update ClipSize and MaxAmmo if received values are greater than values from local weapons.tbl
            int weapon_cls_id = *reinterpret_cast<int32_t*>(data + 4);
            int clip_ammo = *reinterpret_cast<int32_t*>(data + 8);
            int ammo = *reinterpret_cast<int32_t*>(data + 12);
            if (rf::weapon_classes[weapon_cls_id].clip_size < ammo)
                rf::weapon_classes[weapon_cls_id].clip_size = ammo;
            if (rf::weapon_classes[weapon_cls_id].max_ammo < clip_ammo)
                rf::weapon_classes[weapon_cls_id].max_ammo = clip_ammo;
            TRACE("ProcessReloadPacket WeaponClsId %d ClipAmmo %d Ammo %d", weapon_cls_id, clip_ammo, ammo);

            // Call original handler
            ProcessReloadPacket_hook.CallTarget(data, addr);
        }
    },
};

rf::EntityObj* SecureObjUpdatePacket(rf::EntityObj* entity, uint8_t flags, rf::Player* src_player)
{
    if (rf::is_local_net_game) {
        // server-side
        if (entity && entity->_super.handle != src_player->entity_handle) {
            TRACE("Invalid ObjUpdate entity %x %x %s", entity->_super.handle, src_player->entity_handle,
                  src_player->name.CStr());
            return nullptr;
        }

        if (flags & (0x4 | 0x20 | 0x80)) { // OUF_WEAPON_TYPE | OUF_HEALTH_ARMOR | OUF_ARMOR_STATE
            TRACE("Invalid ObjUpdate flags %x", flags);
            return nullptr;
        }
    }
    return entity;
}

FunHook<uint8_t()> MultiAllocPlayerId_hook{
    0x0046EF00,
    []() {
        uint8_t player_id = MultiAllocPlayerId_hook.CallTarget();
        if (player_id == 0xFF)
            player_id = MultiAllocPlayerId_hook.CallTarget();
        return player_id;
    },
};

constexpr int MULTI_HANDLE_MAPPING_ARRAY_SIZE = 1024;

FunHook<rf::Object*(int32_t)> MultiGetObjFromRemoteHandle_hook{
    0x00484B00,
    [](int32_t remote_handle) {
        int index = static_cast<uint16_t>(remote_handle);
        if (index >= MULTI_HANDLE_MAPPING_ARRAY_SIZE)
            return static_cast<rf::Object*>(nullptr);
        return MultiGetObjFromRemoteHandle_hook.CallTarget(remote_handle);
    },
};

FunHook<int32_t(int32_t)> MultiGetLocalHandleFromRemoteHandle_hook{
    0x00484B30,
    [](int32_t remote_handle) {
        int index = static_cast<uint16_t>(remote_handle);
        if (index >= MULTI_HANDLE_MAPPING_ARRAY_SIZE)
            return -1;
        return MultiGetLocalHandleFromRemoteHandle_hook.CallTarget(remote_handle);
    },
};

FunHook<void(int32_t, int32_t)> MultiSetObjHandleMapping_hook{
    0x00484B70,
    [](int32_t remote_handle, int32_t local_handle) {
        int index = static_cast<uint16_t>(remote_handle);
        if (index >= MULTI_HANDLE_MAPPING_ARRAY_SIZE)
            return;
        MultiSetObjHandleMapping_hook.CallTarget(remote_handle, local_handle);
    },
};

CodeInjection ProcessBooleanPacket_ValidateMeshId_patch{
    0x004765A3,
    [](auto& regs) { regs.ecx = std::clamp(regs.ecx, 0, 3); },
};

CodeInjection ProcessBooleanPacket_ValidateRoomId_patch{
    0x0047661C,
    [](auto& regs) {
        int num_rooms = StructFieldRef<int>(rf::rfl_static_geometry, 0x90);
        if (regs.edx < 0 || regs.edx >= num_rooms) {
            WARN("Invalid room in Boolean packet - skipping");
            regs.esp += 0x64;
            regs.eip = 0x004766A5;
        }
    },
};

CodeInjection ProcessPregameBooleanPacket_ValidateMeshId_patch{
    0x0047672F,
    [](auto& regs) {
        // only meshes 0 - 3 are supported
        regs.ecx = std::clamp(regs.ecx, 0, 3);
    },
};

CodeInjection ProcessPregameBooleanPacket_ValidateRoomId_patch{
    0x00476752,
    [](auto& regs) {
        int num_rooms = StructFieldRef<int>(rf::rfl_static_geometry, 0x90);
        if (regs.edx < 0 || regs.edx >= num_rooms) {
            WARN("Invalid room in PregameBoolean packet - skipping");
            regs.esp += 0x68;
            regs.eip = 0x004767AA;
        }
    },
};

CodeInjection ProcessGlassKillPacket_CheckRoomExists_patch{
    0x004723B3,
    [](auto& regs) {
        if (!regs.eax)
            regs.eip = 0x004723EC;
    },
};

CallHook<int(void*, int, int, rf::NwAddr&, int)> nw_get_packet_tracker_hook{
    0x00482ED4,
    [](void* data, int a2, int a3, rf::NwAddr& addr, int super_type) {
        int res = nw_get_packet_tracker_hook.CallTarget(data, a2, a3, addr, super_type);
        auto& tracker_addr = AddrAsRef<rf::NwAddr>(0x006FC550);
        if (res != -1 && addr != tracker_addr)
            res = -1;
        return res;
    },
};

constexpr uint32_t DASH_FACTION_SIGNATURE = 0xDA58FAC7;

// Appended to game_info and join_req packets
struct df_sign_packet_ext
{
    uint32_t df_signature; // DASH_FACTION_SIGNATURE
    uint8_t version_major;
    uint8_t version_minor;
};

std::pair<std::unique_ptr<std::byte[]>, size_t> ExtendPacketWithDashFactionSignature(std::byte* data, size_t len)
{
    df_sign_packet_ext ext;
    ext.df_signature = DASH_FACTION_SIGNATURE;
    ext.version_major = VERSION_MAJOR;
    ext.version_minor = VERSION_MINOR;
    auto new_data = std::make_unique<std::byte[]>(len + sizeof(ext));
    std::copy(data, data + len, new_data.get());
    std::copy(&ext, &ext + 1, reinterpret_cast<df_sign_packet_ext*>(new_data.get() + len));
    auto& new_header = *reinterpret_cast<rfPacketHeader*>(new_data.get());
    new_header.size += sizeof(ext);
    return {std::move(new_data), len + sizeof(ext)};
}

CallHook<int(const rf::NwAddr*, std::byte*, size_t)> send_game_info_packet_hook{
    0x0047B287,
    [](const rf::NwAddr* addr, std::byte* data, size_t len) {
        // Add Dash Faction signature to game_info packet
        auto [new_data, new_len] = ExtendPacketWithDashFactionSignature(data, len);
        return send_game_info_packet_hook.CallTarget(addr, new_data.get(), new_len);
    },
};

CallHook<int(const rf::NwAddr*, std::byte*, size_t)> send_join_req_packet_hook{
    0x0047ABFB,
    [](const rf::NwAddr* addr, std::byte* data, size_t len) {
        // Add Dash Faction signature to join_req packet
        auto [new_data, new_len] = ExtendPacketWithDashFactionSignature(data, len);
        return send_join_req_packet_hook.CallTarget(addr, new_data.get(), new_len);
    },
};

void NetworkInit()
{
    /* ProcessGamePackets hook (not reliable only) */
    ProcessUnreliableGamePackets_hook.Install();

    /* Improve simultaneous ping */
    rf::simultaneous_ping = 32;

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

    // Buffer Overflow fixes
    for (auto& patch : g_buffer_overflow_patches) {
        patch.Install();
    }

    //  Filter packets based on the side (client-side vs server-side)
    ProcessGamePacket_whitelist_filter.Install();

    // Hook packet handlers
    ProcessJoinDenyPacket_hook.Install();
    ProcessNewPlayerPacket_hook.Install();
    ProcessLeftGamePacket_hook.Install();
    ProcessChatLinePacket_hook.Install();
    ProcessNameChangePacket_hook.Install();
    ProcessTeamChangePacket_hook.Install();
    ProcessRateChangePacket_hook.Install();
    ProcessEntityCreatePacket_hook.Install();
    ProcessReloadPacket_hook.Install();

    // Fix ObjUpdate packet handling
    using namespace asm_regs;
    AsmWritter(0x0047E058, 0x0047E06A)
        .mov(eax, *(esp + 0x9C - 0x6C)) // Player
        .push(eax)
        .push(ebx)
        .push(edi)
        .call(SecureObjUpdatePacket)
        .add(esp, 12)
        .mov(edi, eax);

    // Client-side green team fix
    AsmWritter(0x0046CAD7, 0x0046CADA).cmp(al, (int8_t)0xFF);

    // Hide IP addresses in Players packet
    AsmWritter(0x00481D31, 0x00481D33).xor_(eax, eax);
    AsmWritter(0x00481D40, 0x00481D44).xor_(edx, edx);
    // Hide IP addresses in New Player packet
    AsmWritter(0x0047A4A0, 0x0047A4A2).xor_(edx, edx);
    AsmWritter(0x0047A4A6, 0x0047A4AA).xor_(ecx, ecx);

    // Fix "Orion bug" - default 'miner1' entity spawning client-side periodically
    MultiAllocPlayerId_hook.Install();

    // Fix buffer-overflows in multi handle mapping
    MultiGetObjFromRemoteHandle_hook.Install();
    MultiGetLocalHandleFromRemoteHandle_hook.Install();
    MultiSetObjHandleMapping_hook.Install();

    // Fix GameType out of bounds vulnerability in GameInfo packet
    ProcessGameInfoPacket_GameTypeBounds_patch.Install();

    // Fix MeshId out of bounds vulnerability in Boolean packet
    ProcessBooleanPacket_ValidateMeshId_patch.Install();

    // Fix RoomId out of bounds vulnerability in Boolean packet
    ProcessBooleanPacket_ValidateRoomId_patch.Install();

    // Fix MeshId out of bounds vulnerability in PregameBoolean packet
    ProcessPregameBooleanPacket_ValidateMeshId_patch.Install();

    // Fix RoomId out of bounds vulnerability in PregameBoolean packet
    ProcessPregameBooleanPacket_ValidateRoomId_patch.Install();

    // Fix crash if room does not exist in GlassKill packet
    ProcessGlassKillPacket_CheckRoomExists_patch.Install();

    // Make sure tracker packets come from configured tracker
    nw_get_packet_tracker_hook.Install();

    // Add Dash Faction signature to game_info and join_req packets
    send_game_info_packet_hook.Install();
    send_join_req_packet_hook.Install();
}
