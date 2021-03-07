#pragma once

#include "../rf/player.h"

void multi_spectate_set_target_player(rf::Player* player);
rf::Player* multi_spectate_get_target_player();
void multi_spectate_appy_patch();
void multi_spectate_after_full_game_init();
void multi_spectate_level_init();
void multi_spectate_render();
void multi_spectate_on_destroy_player(rf::Player* player);
void multi_spectate_player_create_entity_post(rf::Player* player, rf::Entity* entity);
bool multi_spectate_is_spectating();
bool multi_spectate_execute_action(rf::ControlConfigAction action, bool was_pressed);
