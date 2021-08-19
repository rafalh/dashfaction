#include <array>
#include <algorithm>
#include <cstddef>
#include <cassert>
#include <cstring>
#include <functional>
#include <thread>
#include <winsock2.h>
#include <iphlpapi.h>
#include <ws2ipdef.h>
#include <natupnp.h>
#include <common/config/BuildConfig.h>
#include <common/rfproto.h>
#include <common/version/version.h>
#include <xlog/xlog.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/ShortTypes.h>
#include <patch_common/ComPtr.h>
#include "multi.h"
#include "server.h"
#include "server_internal.h"
#include "../main/main.h"
#include "../rf/multi.h"
#include "../rf/misc.h"
#include "../rf/player/player.h"
#include "../rf/weapon.h"
#include "../rf/entity.h"
#include "../rf/os/console.h"
#include "../rf/os/os.h"
#include "../rf/os/timer.h"
#include "../rf/geometry.h"
#include "../rf/level.h"
#include "../misc/misc.h"
#include "../misc/player.h"
#include "../object/object.h"
#include <common/utils/enum-bitwise-operators.h>
#include <common/utils/list-utils.h>
#include "../os/console.h"

#if MASK_AS_PF
#include "../purefaction/pf.h"
#endif

// NET_IFINDEX_UNSPECIFIED is not defined in MinGW headers
#ifndef NET_IFINDEX_UNSPECIFIED
#define NET_IFINDEX_UNSPECIFIED 0
#endif

int g_update_rate = 30;

using MultiIoPacketHandler = void(char* data, const rf::NetAddr& addr);

class BufferOverflowPatch
{
private:
    std::unique_ptr<BaseCodeInjection> m_movsb_patch;
    uintptr_t m_shr_ecx_2_addr;
    uintptr_t m_and_ecx_3_addr;
    int32_t m_buffer_size;
    int32_t m_ret_addr;

public:
    BufferOverflowPatch(uintptr_t shr_ecx_2_addr, uintptr_t and_ecx_3_addr, int32_t buffer_size) :
        m_movsb_patch(new CodeInjection{and_ecx_3_addr, [this](auto& regs) { movsb_handler(regs); }}),
        m_shr_ecx_2_addr(shr_ecx_2_addr), m_and_ecx_3_addr(and_ecx_3_addr), m_buffer_size(buffer_size)
    {}

    void install()
    {
        const std::byte* shr_ecx_2_ptr = reinterpret_cast<std::byte*>(m_shr_ecx_2_addr);
        const std::byte* and_ecx_3_ptr = reinterpret_cast<std::byte*>(m_and_ecx_3_addr);
        assert(std::memcmp(shr_ecx_2_ptr, "\xC1\xE9\x02", 3) == 0); // shr ecx, 2
        assert(std::memcmp(and_ecx_3_ptr, "\x83\xE1\x03", 3) == 0); // and ecx, 3

        using namespace asm_regs;
        if (std::memcmp(shr_ecx_2_ptr + 3, "\xF3\xA5", 2) == 0) { // rep movsd
            m_movsb_patch->set_addr(m_shr_ecx_2_addr);
            m_ret_addr = m_shr_ecx_2_addr + 5;
            AsmWriter(m_and_ecx_3_addr, m_and_ecx_3_addr + 3).xor_(ecx, ecx);
        }
        else if (std::memcmp(and_ecx_3_ptr + 3, "\xF3\xA4", 2) == 0) { // rep movsb
            AsmWriter(m_shr_ecx_2_addr, m_shr_ecx_2_addr + 3).xor_(ecx, ecx);
            m_movsb_patch->set_addr(m_and_ecx_3_addr);
            m_ret_addr = m_and_ecx_3_addr + 5;
        }
        else {
            assert(false);
        }

        m_movsb_patch->install();
    }

private:
    void movsb_handler(BaseCodeInjection::Regs& regs) const
    {
        char* dst_ptr = regs.edi;
        char* src_ptr = regs.esi;
        int num_bytes = regs.ecx;
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
    BufferOverflowPatch{0x0047B2D3, 0x0047B2DE, 256}, // process_game_info_packet (server name)
    BufferOverflowPatch{0x0047B334, 0x0047B33D, 256}, // process_game_info_packet (level name)
    BufferOverflowPatch{0x0047B38E, 0x0047B397, 256}, // process_game_info_packet (mod name)
    BufferOverflowPatch{0x0047ACF6, 0x0047AD03, 32},  // process_join_req_packet (player name)
    BufferOverflowPatch{0x0047AD4E, 0x0047AD55, 256}, // process_join_req_packet (password)
    BufferOverflowPatch{0x0047A8AE, 0x0047A8B5, 64},  // process_join_accept_packet (level filename)
    BufferOverflowPatch{0x0047A5F4, 0x0047A5FF, 32},  // process_new_player_packet (player name)
    BufferOverflowPatch{0x00481EE6, 0x00481EEF, 32},  // process_players_packet (player name)
    BufferOverflowPatch{0x00481BEC, 0x00481BF8, 64},  // process_state_info_req_packet (level filename)
    BufferOverflowPatch{0x004448B0, 0x004448B7, 256}, // process_chat_line_packet (message)
    BufferOverflowPatch{0x0046EB24, 0x0046EB2B, 32},  // process_name_change_packet (player name)
    BufferOverflowPatch{0x0047C1C3, 0x0047C1CA, 64},  // process_leave_limbo_packet (level filename)
    BufferOverflowPatch{0x0047EE6E, 0x0047EE77, 256}, // process_obj_kill_packet (item name)
    BufferOverflowPatch{0x0047EF9C, 0x0047EFA5, 256}, // process_obj_kill_packet (item name)
    BufferOverflowPatch{0x00475474, 0x0047547D, 256}, // process_entity_create_packet (entity name)
    BufferOverflowPatch{0x00479FAA, 0x00479FB3, 256}, // process_item_create_packet (item name)
    BufferOverflowPatch{0x0046C590, 0x0046C59B, 256}, // process_rcon_req_packet (password)
    BufferOverflowPatch{0x0046C751, 0x0046C75A, 512}, // process_rcon_packet (command)
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

std::optional<DashFactionServerInfo> g_df_server_info;

CodeInjection process_game_packet_whitelist_filter{
    0x0047918D,
    [](auto& regs) {
        bool allowed = false;
        int packet_type = regs.esi;
        if (rf::is_server) {
            auto& whitelist = g_server_side_packet_whitelist;
            allowed = std::find(whitelist.begin(), whitelist.end(), packet_type) != whitelist.end();
        }
        else {
            auto& whitelist = g_client_side_packet_whitelist;
            allowed = std::find(whitelist.begin(), whitelist.end(), packet_type) != whitelist.end();
        }
        if (!allowed) {
            xlog::warn("Ignoring packet 0x%X", packet_type);
            regs.eip = 0x00479194;
        }
        else {
            xlog::trace("Processing packet 0x%X", packet_type);
        }
    },
};

FunHook<MultiIoPacketHandler> process_game_info_packet_hook{
    0x0047B2A0,
    [](char* data, const rf::NetAddr& addr) {
        process_game_info_packet_hook.call_target(data, addr);

        // If this packet is from the server that we are connected to, use game_info for the netgame name
        // Useful for joining using protocol handler because when we join we do not have the server name available yet
        const char* server_name = data + 1;
        if (addr == rf::netgame.server_addr) {
            rf::netgame.name = server_name;
        }
    },
};

CodeInjection process_game_info_packet_game_type_bounds_patch{
    0x0047B30B,
    [](auto& regs) {
        // Valid game types are between 0 and 2
        regs.ecx = std::clamp<int>(regs.ecx, 0, 2);
    },
};

FunHook<MultiIoPacketHandler> process_join_deny_packet_hook{
    0x0047A400,
    [](char* data, const rf::NetAddr& addr) {
        if (rf::multi_is_connecting_to_server(addr)) // client-side
            process_join_deny_packet_hook.call_target(data, addr);
    },
};

FunHook<MultiIoPacketHandler> process_new_player_packet_hook{
    0x0047A580,
    [](char* data, const rf::NetAddr& addr) {
        if (GetForegroundWindow() != rf::main_wnd)
            Beep(750, 300);
        process_new_player_packet_hook.call_target(data, addr);
    },
};

static void verify_player_id_in_packet(char* player_id_ptr, const rf::NetAddr& addr, const char* packet_name)
{
    if (!rf::is_server) {
        // Only server-side checking makes sense because client receives all packets from the server address
        return;
    }
    rf::Player* src_player = rf::multi_find_player_by_addr(addr);
    if (!src_player) {
        // should not happen except for join/game_info packets (protected in rf::multi_io_process_packets)
        assert(false);
        return;
    }
    rf::ubyte real_player_id = src_player->net_data->player_id;
    if (static_cast<rf::ubyte>(*player_id_ptr) != real_player_id) {
        xlog::warn("Wrong player ID in %s packet from %s (expected %02X but got %02X)",
            packet_name, src_player->name.c_str(), real_player_id, *player_id_ptr);
        *player_id_ptr = real_player_id; // fix player ID
    }
}

FunHook<MultiIoPacketHandler> process_left_game_packet_hook{
    0x0047BBC0,
    [](char* data, const rf::NetAddr& addr) {
        // server-side and client-side
        verify_player_id_in_packet(&data[0], addr, "left_game");
        process_left_game_packet_hook.call_target(data, addr);
    },
};

FunHook<MultiIoPacketHandler> process_chat_line_packet_hook{
    0x00444860,
    [](char* data, const rf::NetAddr& addr) {
        // server-side and client-side
        if (rf::is_server) {
            verify_player_id_in_packet(&data[0], addr, "chat_line");

            rf::Player* src_player = rf::multi_find_player_by_addr(addr);
            if (!src_player)
                return; // shouldnt happen (protected in rf::multi_io_process_packets)

            char* msg = data + 2;
            if (check_server_chat_command(msg, src_player))
                return;
        }
        process_chat_line_packet_hook.call_target(data, addr);
    },
};

FunHook<MultiIoPacketHandler> process_name_change_packet_hook{
    0x0046EAE0,
    [](char* data, const rf::NetAddr& addr) {
        // server-side and client-side
        verify_player_id_in_packet(&data[0], addr, "name_change");
        process_name_change_packet_hook.call_target(data, addr);
    },
};

FunHook<MultiIoPacketHandler> process_team_change_packet_hook{
    0x004825B0,
    [](char* data, const rf::NetAddr& addr) {
        // server-side and client-side
        if (rf::is_server) {
            verify_player_id_in_packet(&data[0], addr, "team_change");
            data[1] = std::clamp(data[1], '\0', '\1'); // team validation (fixes "green team")
        }
        process_team_change_packet_hook.call_target(data, addr);
    },
};

FunHook<MultiIoPacketHandler> process_rate_change_packet_hook{
    0x004807B0,
    [](char* data, const rf::NetAddr& addr) {
        // server-side and client-side?
        verify_player_id_in_packet(&data[0], addr, "rate_change");
        process_rate_change_packet_hook.call_target(data, addr);
    },
};

FunHook<MultiIoPacketHandler> process_entity_create_packet_hook{
    0x00475420,
    [](char* data, const rf::NetAddr& addr) {
        // Temporary change default player weapon to the weapon type from the received packet
        // Created entity always receives Default Player Weapon (from game.tbl) and if server has it overriden
        // player weapons would be in inconsistent state with server without this change.
        size_t name_size = strlen(data) + 1;
        char player_id = data[name_size + 58];
        // Check if this is not NPC
        if (player_id != '\xFF') {
            int weapon_type;
            std::memcpy(&weapon_type, data + name_size + 63, sizeof(weapon_type));
            auto old_default_player_weapon = rf::default_player_weapon;
            rf::default_player_weapon = rf::weapon_types[weapon_type].name;
            process_entity_create_packet_hook.call_target(data, addr);
            rf::default_player_weapon = old_default_player_weapon;
        }
        else {
            process_entity_create_packet_hook.call_target(data, addr);
        }
    },
};

FunHook<MultiIoPacketHandler> process_reload_packet_hook{
    0x00485AB0,
    [](char* data, const rf::NetAddr& addr) {
        if (!rf::is_server) { // client-side
            // Update clip_size and max_ammo if received values are greater than values from local weapons.tbl
            int weapon_type, ammo, clip_ammo;
            std::memcpy(&weapon_type, data + 4, sizeof(weapon_type));
            std::memcpy(&ammo, data + 8, sizeof(ammo));
            std::memcpy(&clip_ammo, data + 12, sizeof(clip_ammo));

            if (rf::weapon_types[weapon_type].clip_size < clip_ammo)
                rf::weapon_types[weapon_type].clip_size = clip_ammo;
            if (rf::weapon_types[weapon_type].max_ammo < ammo)
                rf::weapon_types[weapon_type].max_ammo = ammo;
            xlog::trace("process_reload_packet weapon_type %d clip_ammo %d ammo %d", weapon_type, clip_ammo, ammo);

            // Call original handler
            process_reload_packet_hook.call_target(data, addr);
        }
    },
};

FunHook<MultiIoPacketHandler> process_reload_request_packet_hook{
    0x00485A60,
    [](char* data, const rf::NetAddr& addr) {
        if (!rf::is_server) {
            return;
        }
        rf::Player* pp = rf::multi_find_player_by_addr(addr);
        int weapon_type;
        std::memcpy(&weapon_type, data, sizeof(weapon_type));
        if (pp) {
            void multi_reload_weapon_server_side(rf::Player* pp, int weapon_type);
            multi_reload_weapon_server_side(pp, weapon_type);
        }
    },
};

CodeInjection process_obj_update_check_flags_injection{
        0x0047E058,
        [](auto& regs) {
            auto stack_frame = regs.esp + 0x9C;
            rf::Player* pp = addr_as_ref<rf::Player*>(stack_frame - 0x6C);
            int flags = regs.ebx;
            rf::Entity* ep = regs.edi;
            bool valid = true;
            if (rf::is_server) {
                // server-side
                if (ep && ep->handle != pp->entity_handle) {
                    xlog::trace("Invalid obj_update entity %x %x %s", ep->handle, pp->entity_handle,
                        pp->name.c_str());
                    valid = false;
                }
                else if (flags & (0x4 | 0x20 | 0x80)) { // OUF_WEAPON_TYPE | OUF_HEALTH_ARMOR | OUF_ARMOR_STATE
                    xlog::info("Invalid obj_update flags %x", flags);
                    valid = false;
                }
            }
            if (!valid) {
                regs.edi = 0;
            }
        },
    };

CodeInjection process_obj_update_weapon_fire_injection{
    0x0047E2FF,
    [](auto& regs) {
        rf::Entity* entity = regs.edi;
        int flags = regs.ebx;
        auto stack_frame = regs.esp + 0x9C;
        auto* pp = addr_as_ref<rf::Player*>(stack_frame - 0x6C); // null client-side

        constexpr int ouf_fire = 0x40;
        constexpr int ouf_alt_fire = 0x10;

        bool is_on = flags & ouf_fire;
        bool alt_fire = flags & ouf_alt_fire;
        void multi_turn_weapon_on(rf::Entity* ep, rf::Player* pp, bool alt_fire);
        void multi_turn_weapon_off(rf::Entity* ep);
        if (is_on) {
            multi_turn_weapon_on(entity, pp, alt_fire);
        }
        else {
            multi_turn_weapon_off(entity);
        }
        regs.eip = 0x0047E346;
    },
};

FunHook<uint8_t()> multi_alloc_player_id_hook{
    0x0046EF00,
    []() {
        uint8_t player_id = multi_alloc_player_id_hook.call_target();
        if (player_id == 0xFF)
            player_id = multi_alloc_player_id_hook.call_target();
        return player_id;
    },
};

FunHook<rf::Object*(int32_t)> multi_get_obj_from_server_handle_hook{
    0x00484B00,
    [](int32_t remote_handle) {
        size_t index = static_cast<uint16_t>(remote_handle);
        if (index >= obj_limit)
            return static_cast<rf::Object*>(nullptr);
        return multi_get_obj_from_server_handle_hook.call_target(remote_handle);
    },
};

FunHook<int32_t(int32_t)> multi_get_obj_handle_from_server_handle_hook{
    0x00484B30,
    [](int32_t remote_handle) {
        size_t index = static_cast<uint16_t>(remote_handle);
        if (index >= obj_limit)
            return -1;
        return multi_get_obj_handle_from_server_handle_hook.call_target(remote_handle);
    },
};

FunHook<void(int32_t, int32_t)> multi_set_obj_handle_mapping_hook{
    0x00484B70,
    [](int32_t remote_handle, int32_t local_handle) {
        size_t index = static_cast<uint16_t>(remote_handle);
        if (index >= obj_limit)
            return;
        multi_set_obj_handle_mapping_hook.call_target(remote_handle, local_handle);
    },
};

CodeInjection process_boolean_packet_validate_shape_index_patch{
    0x004765A3,
    [](auto& regs) { regs.ecx = std::clamp<int>(regs.ecx, 0, 3); },
};

CodeInjection process_boolean_packet_validate_room_uid_patch{
    0x0047661C,
    [](auto& regs) {
        int num_rooms = rf::level.geometry->all_rooms.size();
        if (regs.edx < 0 || regs.edx >= num_rooms) {
            xlog::warn("Invalid room in Boolean packet - skipping");
            regs.esp += 0x64;
            regs.eip = 0x004766A5;
        }
    },
};

CodeInjection process_pregame_boolean_packet_validate_shape_index_patch{
    0x0047672F,
    [](auto& regs) {
        // only meshes 0 - 3 are supported
        regs.ecx = std::clamp<int>(regs.ecx, 0, 3);
    },
};

CodeInjection process_pregame_boolean_packet_validate_room_uid_patch{
    0x00476752,
    [](auto& regs) {
        int num_rooms = rf::level.geometry->all_rooms.size();
        if (regs.edx < 0 || regs.edx >= num_rooms) {
            xlog::warn("Invalid room in PregameBoolean packet - skipping");
            regs.esp += 0x68;
            regs.eip = 0x004767AA;
        }
    },
};

CodeInjection process_glass_kill_packet_check_room_exists_patch{
    0x004723B3,
    [](auto& regs) {
        if (regs.eax == 0)
            regs.eip = 0x004723EC;
    },
};

CallHook<int(void*, int, int, rf::NetAddr&, int)> net_get_tracker_hook{
    0x00482ED4,
    [](void* data, int a2, int a3, rf::NetAddr& addr, int super_type) {
        int res = net_get_tracker_hook.call_target(data, a2, a3, addr, super_type);
        if (res != -1 && addr != rf::tracker_addr)
            res = -1;
        return res;
    },
};

constexpr uint32_t DASH_FACTION_SIGNATURE = 0xDA58FAC7;

// Appended to game_info and join_req packets
struct df_sign_packet_ext
{
    uint32_t df_signature = DASH_FACTION_SIGNATURE;
    uint8_t version_major = VERSION_MAJOR;
    uint8_t version_minor = VERSION_MINOR;
};

template<typename T>
std::pair<std::unique_ptr<std::byte[]>, size_t> extend_packet(const std::byte* data, size_t len, const T& ext_data)
{
    auto new_data = std::make_unique<std::byte[]>(len + sizeof(ext_data));

    // Modify size in packet header
    RF_GamePacketHeader header;
    std::memcpy(&header, data, sizeof(header));
    header.size += sizeof(ext_data);
    std::memcpy(new_data.get(), &header, sizeof(header));

    // Copy old data
    std::memcpy(new_data.get() + sizeof(header), data + sizeof(header), len - sizeof(header));

    // Append extension data
    std::memcpy(new_data.get() + len, &ext_data, sizeof(ext_data));

    return {std::move(new_data), len + sizeof(ext_data)};
}

std::pair<std::unique_ptr<std::byte[]>, size_t> extend_packet_with_df_signature(std::byte* data, size_t len)
{
    df_sign_packet_ext ext;
    ext.df_signature = DASH_FACTION_SIGNATURE;
    ext.version_major = VERSION_MAJOR;
    ext.version_minor = VERSION_MINOR;
    return extend_packet(data, len, ext);
}

CallHook<int(const rf::NetAddr*, std::byte*, size_t)> send_game_info_packet_hook{
    0x0047B287,
    [](const rf::NetAddr* addr, std::byte* data, size_t len) {
        // Add Dash Faction signature to game_info packet
        auto [new_data, new_len] = extend_packet_with_df_signature(data, len);
        return send_game_info_packet_hook.call_target(addr, new_data.get(), new_len);
    },
};

struct DashFactionJoinAcceptPacketExt
{
    uint32_t df_signature = DASH_FACTION_SIGNATURE;
    uint8_t version_major = VERSION_MAJOR;
    uint8_t version_minor = VERSION_MINOR;

    enum class Flags : uint32_t {
        none           = 0,
        saving_enabled = 1,
        max_fov        = 2,
    } flags = Flags::none;

    float max_fov;

};
template<>
struct EnableEnumBitwiseOperators<DashFactionJoinAcceptPacketExt::Flags> : std::true_type {};

CallHook<int(const rf::NetAddr*, std::byte*, size_t)> send_join_req_packet_hook{
    0x0047ABFB,
    [](const rf::NetAddr* addr, std::byte* data, size_t len) {
        // Add Dash Faction signature to join_req packet
        auto [new_data, new_len] = extend_packet_with_df_signature(data, len);
        return send_join_req_packet_hook.call_target(addr, new_data.get(), new_len);
    },
};

CallHook<int(const rf::NetAddr*, std::byte*, size_t)> send_join_accept_packet_hook{
    0x0047A825,
    [](const rf::NetAddr* addr, std::byte* data, size_t len) {
        // Add Dash Faction signature to join_accept packet
        DashFactionJoinAcceptPacketExt ext_data;
        if (server_is_saving_enabled()) {
            ext_data.flags |= DashFactionJoinAcceptPacketExt::Flags::saving_enabled;
        }
        if (server_get_df_config().max_fov) {
            ext_data.flags |= DashFactionJoinAcceptPacketExt::Flags::max_fov;
            ext_data.max_fov = server_get_df_config().max_fov.value();
        }
        auto [new_data, new_len] = extend_packet(data, len, ext_data);
        return send_join_accept_packet_hook.call_target(addr, new_data.get(), new_len);
    },
};

CodeInjection process_join_accept_injection{
    0x0047A979,
    [](auto& regs) {
        std::byte* packet = regs.ebp;
        auto ext_offset = regs.esi + 5;
        DashFactionJoinAcceptPacketExt ext_data;
        std::copy(packet + ext_offset, packet + ext_offset + sizeof(DashFactionJoinAcceptPacketExt),
            reinterpret_cast<std::byte*>(&ext_data));
        xlog::debug("Checking for join_accept DF extension: %08X", ext_data.df_signature);
        if (ext_data.df_signature == DASH_FACTION_SIGNATURE) {
            DashFactionServerInfo server_info;
            server_info.version_major = ext_data.version_major;
            server_info.version_minor = ext_data.version_minor;
            xlog::debug("Got DF server info: %d %d %d", ext_data.version_major, ext_data.version_minor,
                static_cast<int>(ext_data.flags));
            server_info.saving_enabled = !!(ext_data.flags & DashFactionJoinAcceptPacketExt::Flags::saving_enabled);

            constexpr float default_fov = 90.0f;
            if (!!(ext_data.flags & DashFactionJoinAcceptPacketExt::Flags::max_fov) && ext_data.max_fov >= default_fov) {
                server_info.max_fov = ext_data.max_fov;
            }
            g_df_server_info = std::optional{server_info};
        }
        else {
            g_df_server_info.reset();
        }
    },
};

CodeInjection process_join_accept_send_game_info_req_injection{
    0x0047AA00,
    [](auto& regs) {
        // Force game_info update in case we were joining using protocol handler (server list not fully refreshed) or
        // using old fav entry with outdated name
        rf::NetAddr* server_addr = regs.edi;
        xlog::trace("Sending game_info_req to %x:%d", server_addr->ip_addr, server_addr->port);
        rf::send_game_info_req_packet(*server_addr);
    },
};

std::optional<std::string> determine_local_ip_address()
{
    ULONG forward_table_size = 0;
    if (GetIpForwardTable(nullptr, &forward_table_size, FALSE) != ERROR_INSUFFICIENT_BUFFER) {
        xlog::error("GetIpForwardTable failed");
        return {};
    }

    std::unique_ptr<MIB_IPFORWARDTABLE> forward_table{ reinterpret_cast<PMIB_IPFORWARDTABLE>(operator new(forward_table_size)) };
    if (GetIpForwardTable(forward_table.get(), &forward_table_size, TRUE) != NO_ERROR) {
        xlog::error("GetIpForwardTable failed");
        return {};
    }
    IF_INDEX default_route_if_index = NET_IFINDEX_UNSPECIFIED;
    for (unsigned i = 0; i < forward_table->dwNumEntries; i++) {
        auto& row = forward_table->table[i];
        if (row.dwForwardDest == 0) {
            // Default route to gateway
            xlog::debug("Found default route: IfIndex %lu", row.dwForwardIfIndex);
            default_route_if_index = row.dwForwardIfIndex;
            break;
        }
    }
    if (default_route_if_index == NET_IFINDEX_UNSPECIFIED) {
        xlog::info("No default route found - is this computer connected to internet?");
        return {};
    }

    ULONG ip_table_size = 0;
    if (GetIpAddrTable(nullptr, &ip_table_size, FALSE) != ERROR_INSUFFICIENT_BUFFER) {
        xlog::error("GetIpAddrTable failed");
        return {};
    }
    std::unique_ptr<MIB_IPADDRTABLE> ip_table{ reinterpret_cast<PMIB_IPADDRTABLE>(operator new(ip_table_size)) };
    if (GetIpAddrTable(ip_table.get(), &ip_table_size, FALSE) != NO_ERROR) {
        xlog::error("GetIpAddrTable failed");
        return {};
    }
    for (unsigned i = 0; i < ip_table->dwNumEntries; ++i) {
        auto& row = ip_table->table[i];
        auto* addr_bytes = reinterpret_cast<uint8_t*>(&row.dwAddr);
        auto addr_str = string_format("%d.%d.%d.%d", addr_bytes[0], addr_bytes[1], addr_bytes[2], addr_bytes[3]);
        xlog::debug("IpAddrTable: dwIndex %lu dwAddr %s", row.dwIndex, addr_str.c_str());
        if (row.dwIndex == default_route_if_index) {
            return {addr_str};
        }
    }
    xlog::debug("Interface %lu not found", default_route_if_index);
    return {};
}

bool try_to_auto_forward_port(int port)
{
    xlog::Logger log{"UPnP"};
    log.info("Configuring UPnP port forwarding (port %d)", port);

    auto local_ip_addr_opt = determine_local_ip_address();
    if (!local_ip_addr_opt) {
        log.warn("Cannot determine local IP address");
        return false;
    }
    log.info("Local IP address: %s", local_ip_addr_opt.value().c_str());

    HRESULT hr = CoInitializeEx(nullptr, COINIT_MULTITHREADED);
    if (FAILED(hr)) {
        log.warn("CoInitializeEx failed: hr %lx", hr);
        return false;
    }

    ComPtr<IUPnPNAT> nat;
    hr = CoCreateInstance(__uuidof(UPnPNAT), nullptr, CLSCTX_ALL, __uuidof(IUPnPNAT), reinterpret_cast<void**>(&nat));
    if (FAILED(hr)) {
        log.warn("CoCreateInstance IUPnPNAT failed: hr %lx", hr);
        return false;
    }

    ComPtr<IStaticPortMappingCollection> collection;
    int attempt_num = 0;
    while (true) {
        hr = nat->get_StaticPortMappingCollection(&collection);
        if (FAILED(hr)) {
            log.warn("IUPnPNAT::get_StaticPortMappingCollection failed: hr %lx", hr);
            return false;
        }
        if (collection) {
            break;
        }

        // get_StaticPortMappingCollection sometimes sets collection to nullptr and returns success
        // It may return a proper collecion pointer after few seconds. See UltraVNC code:
        // https://github.com/veyon/ultravnc/blob/master/uvnc_settings2/uvnc_settings/upnp.cpp
        log.info("IUPnPNAT::get_StaticPortMappingCollection returned hr %lx, collection %p", hr, &*collection);
        ++attempt_num;
        if (attempt_num == 10) {
            return false;
        }
        Sleep(1000);
    }
    log.info("Got NAT port mapping table after %d tries", attempt_num);

    wchar_t ip_addr_wide_str[256];
    mbstowcs(ip_addr_wide_str, local_ip_addr_opt.value().c_str(), std::size(ip_addr_wide_str));
    auto* proto = SysAllocString(L"UDP");
    auto* desc = SysAllocString(L"Red Faction");
    auto* internal_client = SysAllocString(ip_addr_wide_str);
    ComPtr<IStaticPortMapping> mapping;
    hr = collection->Add(port, proto, port, internal_client, TRUE, desc, &mapping);
    SysFreeString(proto);
    SysFreeString(desc);
    SysFreeString(internal_client);
    if (FAILED(hr)) {
        log.warn("IStaticPortMappingCollection::Add failed: hr %lx", hr);
        return false;
    }
    log.info("Successfully added UPnP port forwarding (port %d)", port);
    return true;
}

FunHook<void(int, rf::NetAddr*)> multi_start_hook{
    0x0046D5B0,
    [](int is_client, rf::NetAddr *serv_addr) {
        if (!rf::net_port && !is_client) {
            // If no port was specified and this is a server recreate the socket and bind it to port 7755
            xlog::info("Recreating socket using TCP port 7755");
            shutdown(rf::net_udp_socket, 1);
            closesocket(rf::net_udp_socket);
            rf::net_init_socket(7755);
        }
        multi_start_hook.call_target(is_client, serv_addr);
    },
};

FunHook<void()> tracker_do_broadcast_server_hook{
    0x00483130,
    []() {
        tracker_do_broadcast_server_hook.call_target();
        if (g_additional_server_config.upnp_enabled) {
            // Auto forward server port using UPnP (in background thread)
            std::thread upnp_thread{try_to_auto_forward_port, rf::net_port};
            upnp_thread.detach();
        }
    },
};

FunHook<void()> multi_stop_hook{
    0x0046E2C0,
    []() {
        // Clear server info when leaving
        g_df_server_info.reset();
        multi_stop_hook.call_target();
    },
};

const std::optional<DashFactionServerInfo>& get_df_server_info()
{
    return g_df_server_info;
}

void send_chat_line_packet(const char* msg, rf::Player* target, rf::Player* sender, bool is_team_msg)
{
    rf::ubyte buf[512];
    RF_ChatLinePacket packet;
    packet.header.type = RF_GPT_CHAT_LINE;
    packet.header.size = static_cast<uint16_t>(sizeof(packet) - sizeof(packet.header) + std::strlen(msg) + 1);
    packet.player_id = sender ? sender->net_data->player_id : 0xFF;
    packet.is_team_msg = is_team_msg;
    std::memcpy(buf, &packet, sizeof(packet));
    char* packet_msg = reinterpret_cast<char*>(buf + sizeof(packet));
    std::strncpy(packet_msg, msg, 255);
    packet_msg[255] = 0;
    if (target == nullptr && rf::is_server) {
        rf::multi_io_send_reliable_to_all(buf, packet.header.size + sizeof(packet.header), 0);
        rf::console::printf("Server: %s", msg);
    }
    else {
        rf::multi_io_send_reliable(target, buf, packet.header.size + sizeof(packet.header), 0);
    }
}

CodeInjection client_update_rate_injection{
    0x0047E5D8,
    [](auto& regs) {
        auto& send_obj_update_interval = *static_cast<int*>(regs.esp);
        send_obj_update_interval = 1000 / g_update_rate;
    },
};

CodeInjection server_update_rate_injection{
    0x0047E891,
    [](auto& regs) {
        auto& min_send_obj_update_interval = *static_cast<int*>(regs.esp);
        min_send_obj_update_interval = 1000 / g_update_rate;
    },
};

ConsoleCommand2 update_rate_cmd{
    "update_rate",
    [](std::optional<int> update_rate) {
        if (update_rate) {
            if (rf::is_server) {
                // By default server-side update-rate is set to 1/0.085 ~= 12, don't allow lower values
                g_update_rate = std::clamp(update_rate.value(), 12, 60);
                if (g_update_rate > 30) {
                    static rf::Color yellow{255, 255, 0, 255};
                    rf::console::output(
                        "Server update rate greater than 30 is not recommended. It can cause jitter for clients with "
                        "default update rate and break hit registration for clients with high ping.", &yellow);
                }
            }
            else {
                // By default client-side update-rate is set to 20, don't allow lower values
                g_update_rate = std::clamp(update_rate.value(), 20, 60);
            }
        }
        rf::console::printf("Update rate per second: %d", g_update_rate);
    },
};

CodeInjection obj_interp_rotation_fix{
    0x0048443C,
    [](auto& regs) {
        auto& phb_diff = addr_as_ref<rf::Vector3>(regs.ecx);
        constexpr float pi = 3.141592f;
        if (phb_diff.y > pi) {
            phb_diff.y -= 2 * pi;
        }
        else if (phb_diff.y < -pi) {
            phb_diff.y += 2 * pi;
        }
    },
};

CodeInjection obj_interp_too_fast_fix{
    0x00483C3B,
    [](auto& regs) {
        // Make all calculations on milliseconds instead of using microseconds and rounding them up
        auto now = rf::timer_get(1000);
        int frame_time_us = regs.ebp;
        regs.eax = now - frame_time_us;
        regs.edi = now;
    },
};

CodeInjection send_state_info_injection{
    0x0048186F,
    [](auto& regs) {
        rf::Player* player = regs.edi;
        trigger_send_state_info(player);
#if MASK_AS_PF
        pf_player_level_load(player);
#endif
    },
};

FunHook<void(rf::Player*)> send_players_packet_hook{
    0x00481C70,
    [](rf::Player *player) {
        send_players_packet_hook.call_target(player);
#if MASK_AS_PF
        pf_player_init(player);
#endif
        if (rf::is_server) {
            server_reliable_socket_ready(player);
        }
    },
};

extern FunHook<void __fastcall(void*, int, int, bool, int)> multi_io_stats_add_hook;

void __fastcall multi_io_stats_add_new(void *this_, int edx, int size, bool is_send, int packet_type)
{
    // Fix memory corruption when sending/processing packets with non-standard type
    if (packet_type < 56) {
        multi_io_stats_add_hook.call_target(this_, edx, size, is_send, packet_type);
    }
}

FunHook<void __fastcall(void*, int, int, bool, int)> multi_io_stats_add_hook{0x0047CAC0, multi_io_stats_add_new};

static void process_custom_packet(void* data, int len, const rf::NetAddr& addr, rf::Player* player)
{
#if MASK_AS_PF
    pf_process_packet(data, len, addr, player);
#endif
}

CodeInjection multi_io_process_packets_injection{
    0x0047918D,
    [](auto& regs) {
        int packet_type = regs.esi;
        if (packet_type > 0x37) {
            auto stack_frame = regs.esp + 0x1C;
            std::byte* data = regs.ecx;
            int offset = regs.ebp;
            int len = regs.edi;
            auto& addr = *addr_as_ref<rf::NetAddr*>(stack_frame + 0xC);
            auto player = addr_as_ref<rf::Player*>(stack_frame + 0x10);
            process_custom_packet(data + offset, len, addr, player);
            regs.eip = 0x00479194;
        }
    },
};

CallHook<void(const void*, size_t, const rf::NetAddr&, rf::Player*)> process_unreliable_game_packets_hook{
    0x00479244,
    [](const void* data, size_t len, const rf::NetAddr& addr, rf::Player* player) {
#if MASK_AS_PF
        if (pf_process_raw_unreliable_packet(data, len, addr)) {
            return;
        }
#endif
        rf::multi_io_process_packets(data, len, addr, player);
    },
};

CodeInjection net_rel_work_injection{
    0x005291F7,
    [](auto& regs) {
        // Clear rsocket variable before processing next packet
        regs.ebp = 0;
    },
};

CallHook<int()> game_info_num_players_hook{
    0x0047B141,
    []() {
        int player_count = 0;
        auto player_list = SinglyLinkedList{rf::player_list};
        for (auto& current_player : player_list) {
            if (get_player_additional_data(&current_player).is_browser) continue;
            player_count++;
        }
        return player_count;
    },
};

void network_init()
{
    // Improve simultaneous ping
    rf::simultaneous_ping = 32;

    // Change server info timeout to 2s
    write_mem<u32>(0x0044D357 + 2, 2000);

    // Change delay between server info requests
    write_mem<u8>(0x0044D338 + 1, 20);

    // Allow ports < 1023 (especially 0 - any port)
    AsmWriter(0x00528F24).nop(2);

    // Default port: 0
    write_mem<u16>(0x0059CDE4, 0);
    write_mem<i32>(0x004B159D + 1, 0); // TODO: add setting in launcher

    // Do not overwrite multi_entity in Single Player
    AsmWriter(0x004A415F).nop(10);

    // Show valid info for servers with incompatible version
    write_mem<u8>(0x0047B3CB, asm_opcodes::jmp_rel_short);

    // Change default Server List sort to players count
    write_mem<u32>(0x00599D20, 4);

    // Buffer Overflow fixes
    for (auto& patch : g_buffer_overflow_patches) {
        patch.install();
    }

    //  Filter packets based on the side (client-side vs server-side)
    process_game_packet_whitelist_filter.install();

    // Hook packet handlers
    process_join_deny_packet_hook.install();
    process_new_player_packet_hook.install();
    process_left_game_packet_hook.install();
    process_chat_line_packet_hook.install();
    process_name_change_packet_hook.install();
    process_team_change_packet_hook.install();
    process_rate_change_packet_hook.install();
    process_entity_create_packet_hook.install();
    process_reload_packet_hook.install();
    process_reload_request_packet_hook.install();

    // Fix obj_update packet handling
    process_obj_update_check_flags_injection.install();

    // Verify on/off weapons handling
    process_obj_update_weapon_fire_injection.install();

    // Client-side green team fix
    using namespace asm_regs;
    AsmWriter(0x0046CAD7, 0x0046CADA).cmp(al, -1);

    // Hide IP addresses in players packet
    AsmWriter(0x00481D31, 0x00481D33).xor_(eax, eax);
    AsmWriter(0x00481D40, 0x00481D44).xor_(edx, edx);
    // Hide IP addresses in new_player packet
    AsmWriter(0x0047A4A0, 0x0047A4A2).xor_(edx, edx);
    AsmWriter(0x0047A4A6, 0x0047A4AA).xor_(ecx, ecx);

    // Fix "Orion bug" - default 'miner1' entity spawning client-side periodically
    multi_alloc_player_id_hook.install();

    // Fix buffer-overflows in multi handle mapping
    multi_get_obj_from_server_handle_hook.install();
    multi_get_obj_handle_from_server_handle_hook.install();
    multi_set_obj_handle_mapping_hook.install();

    // Use server name from game_info packet for netgame name if address matches current server
    process_game_info_packet_hook.install();

    // Fix game_type out of bounds vulnerability in game_info packet
    process_game_info_packet_game_type_bounds_patch.install();

    // Fix shape_index out of bounds vulnerability in boolean packet
    process_boolean_packet_validate_shape_index_patch.install();

    // Fix room_index out of bounds vulnerability in boolean packet
    process_boolean_packet_validate_room_uid_patch.install();

    // Fix shape_index out of bounds vulnerability in pregame_boolean packet
    process_pregame_boolean_packet_validate_shape_index_patch.install();

    // Fix room_index out of bounds vulnerability in pregame_boolean packet
    process_pregame_boolean_packet_validate_room_uid_patch.install();

    // Fix crash if room does not exist in glass_kill packet
    process_glass_kill_packet_check_room_exists_patch.install();

    // Make sure tracker packets come from configured tracker
    net_get_tracker_hook.install();

    // Add Dash Faction signature to game_info, join_req, join_accept packets
    send_game_info_packet_hook.install();
    send_join_req_packet_hook.install();
    send_join_accept_packet_hook.install();
    process_join_accept_injection.install();
    process_join_accept_send_game_info_req_injection.install();
    multi_stop_hook.install();

    // Use port 7755 when hosting a server without 'Force port' option
    multi_start_hook.install();

    // Use UPnP for port forwarding if server is not in LAN-only mode
    tracker_do_broadcast_server_hook.install();

    // Allow changing client and server update rate
    client_update_rate_injection.install();
    server_update_rate_injection.install();
    update_rate_cmd.register_cmd();

    // Fix rotation interpolation (Y axis) when it goes from 360 to 0 degrees
    obj_interp_rotation_fix.install();

    // Fix object interpolation playing too fast causing a possible jitter
    obj_interp_too_fast_fix.install();

    // Send trigger_activate packets for late joiners
    send_state_info_injection.install();

    // Send more packets after reliable connection has been established
    send_players_packet_hook.install();

    // Use spawnpoint team property in TeamDM game (PF compatible)
    write_mem<u8>(0x00470395 + 4, 0); // change cmp argument: CTF -> DM
    write_mem<u8>(0x0047039A, asm_opcodes::jz_rel_short);  // invert jump condition: jnz -> jz

    // Preserve password case when processing rcon_request command
    write_mem<i8>(0x0046C85A + 1, 1);

    // Fix reliable sockets being incorrectly assigned to clients server-side if two clients with the same IP address
    // connect at the same time. Unpatched game assigns newly connected reliable sockets to a player with the same
    // IP address that the reliable socket uses. This change ensures that the port is also compared.
    write_mem<i8>(0x0046E8DA + 1, 1);

    // Support custom packet types
    AsmWriter{0x0047916D}.nop(2);
    multi_io_process_packets_injection.install();
    multi_io_stats_add_hook.install();
    process_unreliable_game_packets_hook.install();

    // Fix rejecting reliable packets from non-connected clients
    // Fixes players randomly losing connection to the server when some player sends double left game packets
    // when leaving because of missing level file
    net_rel_work_injection.install();

    // Ignore browsers when calculating player count for info requests
    game_info_num_players_hook.install();
}
