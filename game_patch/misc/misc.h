#pragma once

#include <cstddef>

void misc_init();
void misc_after_full_game_init();
void misc_after_level_load(const char* level_filename);
void set_play_sound_events_volume_scale(float volume_scale);
void set_jump_to_multi_server_list(bool jump);
void clear_triggers_for_late_joiners();

constexpr size_t old_obj_limit = 1024;
constexpr size_t obj_limit = 65536;
