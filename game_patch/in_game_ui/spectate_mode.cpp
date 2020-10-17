#include "spectate_mode.h"
#include "hud.h"
#include "scoreboard.h"
#include "../console/console.h"
#include "../rf/entity.h"
#include "../rf/player.h"
#include "../rf/network.h"
#include "../rf/gameseq.h"
#include "../rf/weapon.h"
#include "../rf/graphics.h"
#include "../rf/hud.h"
#include "../main.h"
#include <common/BuildConfig.h>
#include <xlog/xlog.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/CodeInjection.h>
#include <shlwapi.h>
#include <windows.h>

namespace rf
{

static auto& IsEntityLoopFire = AddrAsRef<bool(int entity_handle, signed int weapon_type)>(0x0041A830);
static auto& EntityIsSwimming = AddrAsRef<bool(Entity* entity)>(0x0042A0A0);
static auto& EntityIsFalling = AddrAsRef<bool(Entity* entit)>(0x0042A020);

static auto& PlayerFpgunRender = AddrAsRef<void(Player*)>(0x004A2B30);
static auto& PlayerFpgunUpdate = AddrAsRef<void(Player*)>(0x004A2700);
static auto& PlayerFpgunSetupMesh = AddrAsRef<void(Player*, int weapon_type)>(0x004AA230);
static auto& PlayerFpgunUpdateMesh = AddrAsRef<void(Player*)>(0x004AA6D0);
static auto& PlayerRenderRocketLauncherScannerView = AddrAsRef<void(Player* player)>(0x004AEEF0);
static auto& PlayerFpgunSetState = AddrAsRef<void(Player* player, int state)>(0x004AA560);
static auto& PlayerFpgunHasState = AddrAsRef<bool(Player* player, int state)>(0x004A9520);

} // namespace rf

static rf::Player* g_spectate_mode_target;
static rf::Camera* g_old_target_camera = nullptr;
static bool g_spectate_mode_enabled = false;

static void SetCameraTarget(rf::Player* player)
{
    // Based on function SetCamera1View
    if (!rf::local_player || !rf::local_player->cam || !player)
        return;

    rf::Camera* camera = rf::local_player->cam;
    camera->mode = rf::CAM_FIRST_PERSON;
    camera->player = player;

    g_old_target_camera = player->cam;
    player->cam = camera; // fix crash 0040D744

    rf::CameraSetFirstPerson(camera);
}

void SpectateModeSetTargetPlayer(rf::Player* player)
{
    if (!player)
        player = rf::local_player;

    if (!rf::local_player || !rf::local_player->cam || !g_spectate_mode_target || g_spectate_mode_target == player)
        return;

    if (rf::multi_game_options & rf::MGO_FORCE_RESPAWN) {
        rf::String msg{"You cannot use Spectate Mode because Force Respawn option is enabled on this server!"};
        rf::String prefix;
        rf::ChatPrint(msg, rf::ChatMsgColor::white_white, prefix);
        return;
    }

    // fix old target
    if (g_spectate_mode_target && g_spectate_mode_target != rf::local_player) {
        g_spectate_mode_target->cam = g_old_target_camera;
        g_old_target_camera = nullptr;

#if SPECTATE_MODE_SHOW_WEAPON
        g_spectate_mode_target->flags &= ~(1 << 4);
        rf::Entity* entity = rf::EntityFromHandle(g_spectate_mode_target->entity_handle);
        if (entity)
            entity->local_player = nullptr;
#endif // SPECTATE_MODE_SHOW_WEAPON
    }

    g_spectate_mode_enabled = (player != rf::local_player);
    g_spectate_mode_target = player;

    rf::MultiKillLocalPlayer();
    SetCameraTarget(player);

#if SPECTATE_MODE_SHOW_WEAPON
    player->flags |= 1 << 4;
    rf::Entity* entity = rf::EntityFromHandle(player->entity_handle);
    if (entity) {
        // make sure weapon mesh is loaded now
        rf::PlayerFpgunSetupMesh(player, entity->ai_info.current_primary_weapon);
        xlog::trace("FpgunMesh %p", player->fpgun_mesh);

        // Hide target player from camera
        entity->local_player = player;
    }
#endif // SPECTATE_MODE_SHOW_WEAPON
}

static void SpectateNextPlayer(bool dir, bool try_alive_players_first = false)
{
    rf::Player* new_target;
    if (g_spectate_mode_enabled)
        new_target = g_spectate_mode_target;
    else
        new_target = rf::local_player;
    while (true) {
        new_target = dir ? new_target->next : new_target->prev;
        if (!new_target || new_target == g_spectate_mode_target)
            break; // nothing found
        if (try_alive_players_first && rf::PlayerIsDead(new_target))
            continue;
        if (new_target != rf::local_player) {
            SpectateModeSetTargetPlayer(new_target);
            return;
        }
    }

    if (try_alive_players_first)
        SpectateNextPlayer(dir, false);
}

void SpectateModeEnterFreeLook()
{
    if (!rf::local_player || !rf::local_player->cam || !rf::is_multi)
        return;

    rf::MultiKillLocalPlayer();
    rf::CameraSetFreelook(rf::local_player->cam);
}

bool SpectateModeIsFreeLook()
{
    if (!rf::local_player || !rf::local_player->cam || !rf::is_multi)
        return false;

    auto camera_mode = rf::local_player->cam->mode;
    return camera_mode == rf::CAM_FREELOOK;
}

bool SpectateModeIsActive()
{
    return g_spectate_mode_enabled || SpectateModeIsFreeLook();
}

void SpectateModeLeave()
{
    if (g_spectate_mode_enabled)
        SpectateModeSetTargetPlayer(nullptr);
    else
        SetCameraTarget(rf::local_player);
}

bool SpectateModeHandleCtrlInGame(rf::ControlAction key_id, bool was_pressed)
{
    if (!rf::is_multi) {
        return false;
    }

    if (g_spectate_mode_enabled) {
        if (key_id == rf::CA_PRIMARY_ATTACK || key_id == rf::CA_SLIDE_RIGHT) {
            if (was_pressed)
                SpectateNextPlayer(true);
            return true; // dont allow spawn
        }
        else if (key_id == rf::CA_SECONDARY_ATTACK || key_id == rf::CA_SLIDE_LEFT) {
            if (was_pressed)
                SpectateNextPlayer(false);
            return true;
        }
        else if (key_id == rf::CA_JUMP) {
            if (was_pressed)
                SpectateModeLeave();
            return true;
        }
    }
    else if (SpectateModeIsFreeLook()) {
        // don't allow respawn in freelook spectate
        if (key_id == rf::CA_PRIMARY_ATTACK || key_id == rf::CA_SECONDARY_ATTACK) {
            if (was_pressed)
                SpectateModeLeave();
            return true;
        }
    }
    else if (!g_spectate_mode_enabled) {
        if (key_id == rf::CA_JUMP && was_pressed && rf::PlayerIsDead(rf::local_player)) {
            SpectateModeSetTargetPlayer(rf::local_player);
            SpectateNextPlayer(true, true);
            return true;
        }
    }

    return false;
}

FunHook<void(rf::Player*, rf::ControlAction, bool)> HandleCtrlInGame_hook{
    0x004A6210,
    [](rf::Player* player, rf::ControlAction ctrl, bool was_pressed) {
        if (!SpectateModeHandleCtrlInGame(ctrl, was_pressed)) {
            HandleCtrlInGame_hook.CallTarget(player, ctrl, was_pressed);
        }
    },
};

bool IsPlayerEntityInvalid_New(rf::Player* player)
{
    if (SpectateModeIsActive())
        return false;
    else
        return rf::PlayerIsDead(player);
}

CallHook<bool(rf::Player*)> IsPlayerEntityInvalid_RedBars_hook{0x00432A52, IsPlayerEntityInvalid_New};
CallHook<bool(rf::Player*)> IsPlayerEntityInvalid_Scoreboard_hook{0x00437BEE, IsPlayerEntityInvalid_New};
CallHook<bool(rf::Player*)> IsPlayerEntityInvalid_Scoreboard2_hook{0x00437C25, IsPlayerEntityInvalid_New};

static bool IsPlayerDying_New(rf::Player* player)
{
    if (SpectateModeIsActive())
        return false;
    else
        return rf::PlayerIsDying(player);
}

CallHook IsPlayerDying_RedBars_hook{0x00432A5F, IsPlayerDying_New};
CallHook IsPlayerDying_Scoreboard_hook{0x00437C01, IsPlayerDying_New};
CallHook IsPlayerDying_Scoreboard2_hook{0x00437C36, IsPlayerDying_New};

void SpectateModeOnDestroyPlayer(rf::Player* player)
{
    if (g_spectate_mode_target == player)
        SpectateNextPlayer(true);
    if (g_spectate_mode_target == player)
        SpectateModeSetTargetPlayer(nullptr);
}

FunHook<void(rf::Player*)> RenderReticle_hook{
    0x0043A2C0,
    [](rf::Player* player) {
        if (rf::GameSeqGetState() == rf::GS_MULTI_LIMBO)
            return;
        if (g_spectate_mode_enabled)
            RenderReticle_hook.CallTarget(g_spectate_mode_target);
        else
            RenderReticle_hook.CallTarget(player);
    },
};

FunHook<rf::Entity*(rf::Player*, int, const rf::Vector3*, const rf::Matrix3*, int)> PlayerCreateEntity_hook{
    0x004A4130,
    [](rf::Player* player, int cls_id, const rf::Vector3* pos, const rf::Matrix3* orient, int mp_character) {
        // hide target player from camera after respawn
        rf::Entity* entity = PlayerCreateEntity_hook.CallTarget(player, cls_id, pos, orient, mp_character);
        if (entity && player == g_spectate_mode_target)
            entity->local_player = player;

        return entity;
    },
};

CodeInjection render_scanner_view_for_spectated_player_injection{
    0x00431890,
    []() {
        if (g_spectate_mode_enabled)
            rf::PlayerRenderRocketLauncherScannerView(g_spectate_mode_target);
    },
};

FunHook<bool(rf::Player*)> CanPlayerFire_hook{
    0x004A68D0,
    [](rf::Player* player) {
        if (SpectateModeIsActive()) {
            return false;
        }
        return CanPlayerFire_hook.CallTarget(player);
    }
};

DcCommand2 spectate_cmd{
    "spectate",
    [](std::optional<std::string> player_name) {
        if (!rf::is_multi) {
            rf::ConsoleOutput("Spectate mode is only supported in multiplayer game!", nullptr);
            return;
        }

        if (rf::multi_game_options & rf::MGO_FORCE_RESPAWN) {
            rf::ConsoleOutput("Spectate mode is disabled because of Force Respawn server option!", nullptr);
            return;
        }

        if (player_name) {
            // spectate player using 1st person view
            rf::Player* player = FindBestMatchingPlayer(player_name.value().c_str());
            if (!player) {
                // player not found
                return;
            }
            // player found - spectate
            SpectateModeSetTargetPlayer(player);
        }
        else if (g_spectate_mode_enabled || SpectateModeIsFreeLook()) {
            // leave spectate mode
            SpectateModeLeave();
        }
        else {
            // enter freelook spectate mode
            SpectateModeEnterFreeLook();
        }
    },
    "Toggles spectate mode (first person or free-look depending on the argument)",
    "spectate [<player_name>]",
};

#if SPECTATE_MODE_SHOW_WEAPON

static void PlayerFpgunRender_New(rf::Player* player)
{
    if (g_spectate_mode_enabled) {
        rf::Entity* entity = rf::EntityFromHandle(g_spectate_mode_target->entity_handle);

        // HACKFIX: RF uses function PlayerSetRemoteChargeVisible for local player only
        g_spectate_mode_target->fpgun_data.remote_charge_visible =
            (entity && entity->ai_info.current_primary_weapon == rf::remote_charge_weapon_type);

        if (g_spectate_mode_target != rf::local_player && entity) {
            static rf::Vector3 old_vel;
            rf::Vector3 vel_diff = entity->p_data.vel - old_vel;
            old_vel = entity->p_data.vel;

            if (vel_diff.y > 0.1f)
                entity->entity_flags |= 2; // jump
        }

        if (g_spectate_mode_target->fpgun_data.in_scope_view)
            g_spectate_mode_target->fpgun_data.scope_zoom = 2.0f;
        rf::local_player->fpgun_data.in_scope_view = g_spectate_mode_target->fpgun_data.in_scope_view;
        rf::local_player->fpgun_data.scope_zoom = g_spectate_mode_target->fpgun_data.scope_zoom;

        rf::PlayerFpgunUpdateMesh(g_spectate_mode_target);
        rf::PlayerFpgunRender(g_spectate_mode_target);
    }
    else
        rf::PlayerFpgunRender(player);
}

FunHook<void(rf::Player*)> PlayerFpgunUpdateState_hook{
    0x004AA3A0,
    [](rf::Player* player) {
        PlayerFpgunUpdateState_hook.CallTarget(player);
        if (player != rf::local_player) {
            rf::Entity* entity = rf::EntityFromHandle(player->entity_handle);
            if (entity) {
                float horz_speed_pow2 = entity->p_data.vel.x * entity->p_data.vel.x +
                                          entity->p_data.vel.z * entity->p_data.vel.z;
                int state = 0;
                if (rf::IsEntityLoopFire(entity->handle, entity->ai_info.current_primary_weapon))
                    state = 2;
                else if (rf::EntityIsSwimming(entity) || rf::EntityIsFalling(entity))
                    state = 0;
                else if (horz_speed_pow2 > 0.2f)
                    state = 1;
                if (!rf::PlayerFpgunHasState(player, state))
                    rf::PlayerFpgunSetState(player, state);
            }
        }
    },
};

#endif // SPECTATE_MODE_SHOW_WEAPON

void SpectateModeInit()
{
    IsPlayerDying_RedBars_hook.Install();
    IsPlayerDying_Scoreboard_hook.Install();
    IsPlayerDying_Scoreboard2_hook.Install();

    IsPlayerEntityInvalid_RedBars_hook.Install();
    IsPlayerEntityInvalid_Scoreboard_hook.Install();
    IsPlayerEntityInvalid_Scoreboard2_hook.Install();

    HandleCtrlInGame_hook.Install();
    RenderReticle_hook.Install();
    PlayerCreateEntity_hook.Install();
    CanPlayerFire_hook.Install();

    spectate_cmd.Register();

    // Note: HUD rendering doesn't make sense because life and armor isn't synced

#if SPECTATE_MODE_SHOW_WEAPON

    AsmWriter(0x0043285D).call(PlayerFpgunRender_New);
    AsmWriter(0x004AB1B8).nop(6); // PlayerFpgunRenderInternal
    AsmWriter(0x004AA23E).nop(6); // PlayerFpgunSetupMesh
    AsmWriter(0x004AE0DF).nop(2); // PlayerFpgunLoadMesh

    AsmWriter(0x004A938F).nop(6);               // PlayerFpgunSetAction
    WriteMem<u8>(0x004A952C, asm_opcodes::jmp_rel_short); // PlayerFpgunHasState
    AsmWriter(0x004AA56D).nop(6);               // PlayerFpgunSetState
    AsmWriter(0x004AA6E7).nop(6);               // PlayerFpgunUpdateMesh
    AsmWriter(0x004AE384).nop(6);               // PlayerFpgunPrepareWeapon
    WriteMem<u8>(0x004ACE2C, asm_opcodes::jmp_rel_short); // GetZoomValue

    WriteMemPtr(0x0048857E + 2, &g_spectate_mode_target); // RenderObjects
    WriteMemPtr(0x00488598 + 1, &g_spectate_mode_target); // RenderObjects
    WriteMemPtr(0x00421889 + 2, &g_spectate_mode_target); // EntityRender
    WriteMemPtr(0x004218A2 + 2, &g_spectate_mode_target); // EntityRender

    render_scanner_view_for_spectated_player_injection.Install();
    PlayerFpgunUpdateState_hook.Install();
#endif // SPECTATE_MODE_SHOW_WEAPON
}

void SpectateModeAfterFullGameInit()
{
    g_spectate_mode_target = rf::local_player;
}

template<typename F>
static void DrawWithShadow(int x, int y, int shadow_dx, int shadow_dy, rf::Color clr, rf::Color shadow_clr, F fun)
{
    rf::GrSetColor(shadow_clr);
    fun(x + shadow_dx, y + shadow_dy);
    rf::GrSetColor(clr);
    fun(x, y);
}

void SpectateModeDrawUI()
{
    if (rf::is_hud_hidden || rf::GameSeqGetState() != rf::GS_GAMEPLAY || SpectateModeIsFreeLook()) {
        return;
    }

    int large_font = HudGetLargeFont();
    int large_font_h = rf::GrGetFontHeight(large_font);
    int medium_font = HudGetDefaultFont();
    int medium_font_h = rf::GrGetFontHeight(medium_font);

    int hints_x = 20;
    int hints_y = g_game_config.big_hud ? 350 : 200;
    int hints_line_spacing = medium_font_h + 3;
    if (!g_spectate_mode_enabled) {
        if (rf::PlayerIsDead(rf::local_player)) {
            rf::GrSetColorRgba(0xFF, 0xFF, 0xFF, 0xFF);
            rf::GrString(hints_x, hints_y, "Press JUMP key to enter Spectate Mode", medium_font);
        }
        return;
    }

    int scr_w = rf::GrScreenWidth();
    int scr_h = rf::GrScreenHeight();

    int title_x = scr_w / 2;
    int title_y = g_game_config.big_hud ? 250 : 150;
    rf::Color white_clr{255, 255, 255, 255};
    rf::Color shadow_clr{0, 0, 0, 128};
    DrawWithShadow(title_x, title_y, 2, 2, white_clr, shadow_clr, [=](int x, int y) {
        rf::GrStringAligned(rf::GR_ALIGN_CENTER, x, y, "SPECTATE MODE", large_font);
    });

    rf::GrSetColorRgba(0xFF, 0xFF, 0xFF, 0xFF);
    rf::GrString(hints_x, hints_y, "Press JUMP key to exit Spectate Mode", medium_font);
    hints_y += hints_line_spacing;
    rf::GrString(hints_x, hints_y, "Press PRIMARY ATTACK key to switch to the next player", medium_font);
    hints_y += hints_line_spacing;
    rf::GrString(hints_x, hints_y, "Press SECONDARY ATTACK key to switch to the previous player", medium_font);

    const int bar_w = g_game_config.big_hud ? 800 : 500;
    const int bar_h = 50;
    int bar_x = (scr_w - bar_w) / 2;
    int bar_y = scr_h - 100;
    rf::GrSetColorRgba(0, 0, 0x00, 0x60);
    rf::GrRect(bar_x, bar_y, bar_w, bar_h);

    rf::GrSetColorRgba(0xFF, 0xFF, 0, 0x80);
    auto str = StringFormat("Spectating: %s", g_spectate_mode_target->name.CStr());
    rf::GrStringAligned(rf::GR_ALIGN_CENTER, bar_x + bar_w / 2, bar_y + bar_h / 2 - large_font_h / 2, str.c_str(), large_font);

    rf::Entity* entity = rf::EntityFromHandle(g_spectate_mode_target->entity_handle);
    if (!entity) {
        rf::GrSetColorRgba(0xFF, 0xFF, 0xFF, 0xFF);
        static int blood_bm = rf::BmLoad("bloodsmear07_A.tga", -1, true);
        int blood_w, blood_h;
        rf::BmGetDimension(blood_bm, &blood_w, &blood_h);
        rf::GrBitmapStretched(blood_bm, (scr_w - blood_w * 2) / 2, (scr_h - blood_h * 2) / 2, blood_w * 2,
                                  blood_h * 2, 0, 0, blood_w, blood_h, 0.0f, 0.0f, rf::gr_bitmap_clamp_mode);

        rf::Color dead_clr{0xF0, 0x20, 0x10, 0xC0};
        DrawWithShadow(scr_w / 2, scr_h / 2, 2, 2, dead_clr, shadow_clr, [=](int x, int y) {
            rf::GrStringAligned(rf::GR_ALIGN_CENTER, x, y, "DEAD", large_font);
        });
    }
}
