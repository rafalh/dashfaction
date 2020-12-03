#pragma once

// Forward declarations
namespace rf
{
    struct Player;
}

void spectate_mode_set_target_player(rf::Player* player);
void spectate_mode_init();
void spectate_mode_after_full_game_init();
void spectate_mode_level_init();
void spectate_mode_draw_ui();
void spectate_mode_on_destroy_player(rf::Player* player);
void spectate_mode_player_create_entity_post(rf::Player* player);
bool spectate_mode_is_active();
