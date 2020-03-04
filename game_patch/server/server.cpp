#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <common/BuildConfig.h>
#include <common/rfproto.h>
#include <algorithm>
#include "server.h"
#include "server_internal.h"
#include "../console/console.h"
#include "../main.h"
#include "../rf/player.h"
#include "../rf/network.h"
#include "../rf/parse.h"
#include "../rf/weapon.h"
#include "../rf/entity.h"

const char* g_rcon_cmd_whitelist[] = {
    "kick",
    "level",
    "ban",
    "ban_ip",
    "map_ext",
    "map_rest",
    "map_next",
    "map_prev",
};

ServerAdditionalConfig g_additional_server_config;
std::string g_prev_level;

void SendChatLinePacket(const char* msg, rf::Player* target, rf::Player* sender, bool is_team_msg)
{
    uint8_t buf[512];
    rfMessage& packet = *reinterpret_cast<rfMessage*>(buf);
    packet.type = RF_MESSAGE;
    packet.size = static_cast<uint16_t>(std::strlen(msg) + 3);
    packet.player_id = sender ? sender->nw_data->player_id : 0xFF;
    packet.is_team_msg = is_team_msg;
    std::strncpy(packet.message, msg, 255);
    packet.message[255] = 0;
    if (target == nullptr) {
        rf::NwSendReliablePacketToAll(buf, packet.size + 3, 0);
        rf::DcPrintf("Server: %s", msg);
    }
    else {
        rf::NwSendReliablePacket(target, buf, packet.size + 3, 0);
    }
}

void ParseVoteConfig(const char* vote_name, VoteConfig& config, rf::StrParser& parser)
{
    std::string vote_option_name = StringFormat("$DF %s:", vote_name);
    if (parser.OptionalString(vote_option_name.c_str())) {
        config.enabled = parser.GetBool();
        rf::DcPrintf("DF %s: %s", vote_name, config.enabled ? "true" : "false");

        // if (parser.OptionalString("+Min Voters:")) {
        //     config.min_voters = parser.GetUInt();
        // }

        // if (parser.OptionalString("+Min Percentage:")) {
        //     config.min_percentage = parser.GetUInt();
        // }

        if (parser.OptionalString("+Time Limit:")) {
            config.time_limit_seconds = parser.GetUInt();
        }
    }
}

void LoadAdditionalServerConfig(rf::StrParser& parser)
{
    ParseVoteConfig("Vote Kick", g_additional_server_config.vote_kick, parser);
    ParseVoteConfig("Vote Level", g_additional_server_config.vote_level, parser);
    ParseVoteConfig("Vote Extend", g_additional_server_config.vote_extend, parser);
    ParseVoteConfig("Vote Restart", g_additional_server_config.vote_restart, parser);
    ParseVoteConfig("Vote Next", g_additional_server_config.vote_next, parser);
    ParseVoteConfig("Vote Previous", g_additional_server_config.vote_previous, parser);
    if (parser.OptionalString("$DF Spawn Protection Duration:")) {
        g_additional_server_config.spawn_protection_duration_ms = parser.GetUInt();
    }
    if (parser.OptionalString("$DF Hitsounds:")) {
        g_additional_server_config.hit_sounds.enabled = parser.GetBool();
        if (parser.OptionalString("+Sound ID:")) {
            g_additional_server_config.hit_sounds.sound_id = parser.GetUInt();
        }
        if (parser.OptionalString("+Rate Limit:")) {
            g_additional_server_config.hit_sounds.rate_limit = parser.GetUInt();
        }
    }

    while (parser.OptionalString("$DF Item Replacement:")) {
        rf::String old_item, new_item;
        parser.GetString(&old_item);
        parser.GetString(&new_item);
        g_additional_server_config.item_replacements.insert({old_item.CStr(), new_item.CStr()});
    }

    if (parser.OptionalString("$DF Default Player Weapon:")) {
        rf::String default_weapon;
        parser.GetString(&default_weapon);
        g_additional_server_config.default_player_weapon = default_weapon.CStr();

        if (parser.OptionalString("+Initial Ammo:")) {
            auto ammo = parser.GetUInt();
            g_additional_server_config.default_player_weapon_ammo = {ammo};

            auto WeaponClsFind = AddrAsRef<int(const char*)>(0x004C81F0);
            auto weapon_cls_id = WeaponClsFind(g_additional_server_config.default_player_weapon.c_str());
            if (weapon_cls_id >= 0) {
                auto& weapon_cls = rf::weapon_classes[weapon_cls_id];
                weapon_cls.max_ammo_mp = std::max<int>(weapon_cls.max_ammo_mp, ammo);
            }
        }
    }

    if (parser.OptionalString("$DF Require Client Mod:")) {
        g_additional_server_config.require_client_mod = parser.GetBool();
    }

    if (parser.OptionalString("$DF Player Damage Modifier:")) {
        g_additional_server_config.player_damage_modifier = parser.GetFloat();
    }

    if (!parser.OptionalString("$Name:") && !parser.OptionalString("#End")) {
        parser.Error("end of server configuration");
    }
}

CodeInjection dedicated_server_load_config_patch{
    0x0046E216,
    [](auto& regs) {
        auto& parser = *reinterpret_cast<rf::StrParser*>(regs.esp - 4 + 0x4C0 - 0x470);
        LoadAdditionalServerConfig(parser);
    },
};

std::pair<std::string_view, std::string_view> StripBySpace(std::string_view str)
{
    auto space_pos = str.find(' ');
    if (space_pos == std::string_view::npos)
        return {str, {}};
    else
        return {str.substr(0, space_pos), str.substr(space_pos + 1)};
}

void HandleNextMapCommand(rf::Player* source)
{
    int next_idx = (rf::level_rotation_idx + 1) % rf::server_level_list.Size();
    auto msg = StringFormat("Next level: %s", rf::server_level_list.Get(next_idx).CStr());
    SendChatLinePacket(msg.c_str(), source);
}

void HandleServerChatCommand(std::string_view server_command, rf::Player* sender)
{
    auto [cmd_name, cmd_arg] = StripBySpace(server_command);

    if (cmd_name == "info")
        SendChatLinePacket("Server powered by Dash Faction", sender);
    else if (cmd_name == "vote") {
        auto [vote_name, vote_arg] = StripBySpace(cmd_arg);
        HandleVoteCommand(vote_name, vote_arg, sender);
    }
    else if (cmd_name == "nextmap" || cmd_name == "nextlevel") {
        HandleNextMapCommand(sender);
    }
    else
        SendChatLinePacket("Unrecognized server command!", sender);
}

bool CheckServerChatCommand(const char* msg, rf::Player* sender)
{
    if (msg[0] == '/') {
        HandleServerChatCommand(msg + 1, sender);
        return true;
    }

    auto [cmd, rest] = StripBySpace(msg);
    if (cmd == "server")
        HandleServerChatCommand(rest, sender);
    else if (cmd == "vote")
        HandleServerChatCommand(msg, sender);
    else
        return false;

    return true;
}

CodeInjection spawn_protection_duration_patch{
    0x0048089A,
    [](auto& regs) {
        *reinterpret_cast<int*>(regs.esp) = g_additional_server_config.spawn_protection_duration_ms;
    },
};

CodeInjection detect_browser_player_patch{
    0x0047AFFB,
    [](auto& regs) {
        rf::Player* player = reinterpret_cast<rf::Player*>(regs.esi);
        int conn_rate = regs.eax;
        if (conn_rate == 1) {
            auto& pdata = GetPlayerAdditionalData(player);
            pdata.is_browser = true;
        }
    },
};

void SendHitSoundPacket(rf::Player* target)
{
    // rate limiting - max 5 per second
    int now = rf::TimerGet(1000);
    auto& pdata = GetPlayerAdditionalData(target);
    if (now - pdata.last_hitsound_sent_ms < 1000 / g_additional_server_config.hit_sounds.rate_limit) {
        return;
    }
    pdata.last_hitsound_sent_ms = now;

    // Send sound packet
    rfSound packet;
    packet.type = RF_SOUND;
    packet.size = sizeof(packet) - sizeof(rfPacketHeader);
    packet.sound_id = g_additional_server_config.hit_sounds.sound_id;
    // FIXME: it does not work on RF 1.21
    memset(&packet.x, 0xFF, 12);
    rf::NwSendNotReliablePacket(target->nw_data->addr, &packet, sizeof(packet));
}

FunHook<float(rf::EntityObj*, float, int, int, int)> EntityTakeDamage_hook{
    0x0041A350,
    [](rf::EntityObj* entity, float damage, int responsible_entity_handle, int dmg_type, int responsible_entity_uid) {
        auto damaged_player = rf::GetPlayerFromEntityHandle(entity->_super.handle);
        auto responsible_player = rf::GetPlayerFromEntityHandle(responsible_entity_handle);
        if (damaged_player && responsible_player) {
            damage *= g_additional_server_config.player_damage_modifier;
            if (damage == 0.0f) {
                return 0.0f;
            }
        }

        float dmg = EntityTakeDamage_hook.CallTarget(entity, damage, responsible_entity_handle, dmg_type, responsible_entity_uid);
        if (g_additional_server_config.hit_sounds.enabled && responsible_player) {
            SendHitSoundPacket(responsible_player);
        }

        return dmg;
    },
};

CallHook<int(const char*)> find_rfl_item_class_hook{
    0x00465102,
    [](const char* cls_name) {
        if (rf::is_dedicated_server) {
            // support item replacement mapping
            auto it = g_additional_server_config.item_replacements.find(cls_name);
            if (it != g_additional_server_config.item_replacements.end())
                cls_name = it->second.c_str();
        }
        return find_rfl_item_class_hook.CallTarget(cls_name);
    },
};

CallHook<int(const char*)> find_default_weapon_for_entity_hook{
    0x004A43DA,
    [](const char* weapon_name) {
        if (rf::is_dedicated_server && !g_additional_server_config.default_player_weapon.empty()) {
            weapon_name = g_additional_server_config.default_player_weapon.c_str();
        }
        return find_default_weapon_for_entity_hook.CallTarget(weapon_name);
    },
};

CallHook<void(rf::Player*, int, int)> give_default_weapon_ammo_hook{
    0x004A4414,
    [](rf::Player* player, int weapon_cls_id, int ammo) {
        if (g_additional_server_config.default_player_weapon_ammo) {
            ammo = g_additional_server_config.default_player_weapon_ammo.value();
        }
        give_default_weapon_ammo_hook.CallTarget(player, weapon_cls_id, ammo);
    },
};

FunHook<bool (const char*, int)> MpIsLevelForGameMode_hook{
    0x00445050,
    [](const char *filename, int game_mode) {
        if (game_mode == RF_CTF) {
            return _strnicmp(filename, "ctf", 3) == 0 || _strnicmp(filename, "pctf", 4) == 0;
        }
        else {
            return _strnicmp(filename, "dm", 2) == 0 || _strnicmp(filename, "pdm", 3) == 0;
        }
    },
};

FunHook<void(rf::Player*)> spawn_player_sync_ammo_hook{
    0x00480820,
    [](rf::Player* player) {
        spawn_player_sync_ammo_hook.CallTarget(player);
        // if default player weapon has ammo override sync ammo using additional reload packet
        if (g_additional_server_config.default_player_weapon_ammo && !rf::IsPlayerEntityInvalid(player)) {
            rf::EntityObj* entity = rf::EntityGetFromHandle(player->entity_handle);
            rfReload packet;
            packet.type = RF_RELOAD;
            packet.size = sizeof(packet) - sizeof(rfPacketHeader);
            packet.entity_handle = entity->_super.handle;
            int weapon_cls_id = entity->ai_info.weapon_cls_id;
            packet.weapon = weapon_cls_id;
            packet.ammo = entity->ai_info.weapons_ammo[weapon_cls_id];
            int ammo_type = rf::weapon_classes[weapon_cls_id].ammo_type;
            packet.clip_ammo = entity->ai_info.clip_ammo[ammo_type];
            rf::NwSendReliablePacket(player, reinterpret_cast<uint8_t*>(&packet), sizeof(packet), 0);
        }
    },
};

struct ParticleCreateData
{
    rf::Vector3 pos;
    rf::Vector3 velocity;
    float pradius;
    float growth_rate;
    float acceleration;
    float gravity_scale;
    float life_secs;
    int bitmap;
    int field_30;
    rf::Color particle_clr;
    rf::Color particle_clr_dest;
    int particle_flags;
    int particle_flags2;
    int field_44;
    int field_48;
};
static_assert(sizeof(ParticleCreateData) == 0x4C);

FunHook<void(int, ParticleCreateData&, void*, rf::Vector3*, int, void**, void*)> ParticleCreate_hook{
    0x00496840,
    [](int pool_id, ParticleCreateData& create_data, void* room, rf::Vector3 *a4, int parent_obj, void** result, void* emitter) {
        bool damages_flag = create_data.particle_flags2 & 1;
        // Do not create not damaging particles on a dedicated server
        if (damages_flag || !rf::is_dedicated_server)
            ParticleCreate_hook.CallTarget(pool_id, create_data, room, a4, parent_obj, result, emitter);
    },
};

CallHook<void(char*)> get_mod_name_for_game_info_packet_patch{
    0x0047B1E0,
    [](char* mod_name) {
        if (g_additional_server_config.require_client_mod) {
            get_mod_name_for_game_info_packet_patch.CallTarget(mod_name);
        }
        else {
            mod_name[0] = '\0';
        }
    },
};

CodeInjection send_ping_after_time_wrap_fix{
    0x0047CCF3,
    [](auto& regs) {
        regs.ebx = -1;
    },
};

void ServerInit()
{
    // Override rcon command whitelist
    WriteMemPtr(0x0046C794 + 1, g_rcon_cmd_whitelist);
    WriteMemPtr(0x0046C7D1 + 2, g_rcon_cmd_whitelist + std::size(g_rcon_cmd_whitelist));

    // Additional server config
    dedicated_server_load_config_patch.Install();

    // Apply customized spawn protection duration
    spawn_protection_duration_patch.Install();

    // Detect if player joining to the server is a browser
    detect_browser_player_patch.Install();

    // Hit sounds
    EntityTakeDamage_hook.Install();

    // Do not strip '%' characters from chat messages
    WriteMem<u8>(0x004785FD, asm_opcodes::jmp_rel_short);

    // Item replacements
    find_rfl_item_class_hook.Install();

    // Default player weapon class and ammo override
    find_default_weapon_for_entity_hook.Install();
    give_default_weapon_ammo_hook.Install();
    spawn_player_sync_ammo_hook.Install();

#if SERVER_WIN32_CONSOLE // win32 console
    InitWin32ServerConsole();
#endif

    InitLazyban();
    InitServerCommands();

    // Remove level prefix restriction (dm/ctf) for 'level' command and dedicated_server.txt
    AsmWriter(0x004350FE).nop(2);
    AsmWriter(0x0046E179).nop(2);

    // In Multi -> Create game fix level filtering so 'pdm' and 'pctf' is supported
    MpIsLevelForGameMode_hook.Install();

    // Do not create not damaging particles on a dedicated server
    ParticleCreate_hook.Install();

    // Allow disabling mod name announcement
    get_mod_name_for_game_info_packet_patch.Install();

    // Fix items not being respawned after time in ms wraps around (~25 days)
    AsmWriter(0x004599DB).nop(2);

    // Fix sending ping packets after time in ms wraps around (~25 days)
    send_ping_after_time_wrap_fix.Install();
    WriteMem<u8>(0x0047CCF9, asm_opcodes::jz_rel_short);
    WriteMem<u8>(0x0047CD01, asm_opcodes::jz_rel_short);
}

void ServerCleanup()
{
#if SERVER_WIN32_CONSOLE // win32 console
    CleanupWin32ServerConsole();
#endif
}

void ServerDoFrame()
{
    ServerVoteDoFrame();
}

void ServerOnLimboStateEnter()
{
    g_prev_level = rf::level_filename.CStr();
    ServerVoteOnLimboStateEnter();
}
