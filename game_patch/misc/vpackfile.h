#pragma once

#include <functional>
#include "../utils/string-utils.h"

enum GameLang
{
    LANG_EN = 0,
    LANG_GR = 1,
    LANG_FR = 2,
};

void vpackfile_apply_patches();
GameLang get_installed_game_lang();
bool is_modded_game();
void vpackfile_find_matching_files(const StringMatcher& query, std::function<void(const char*)> result_consumer);
void vpackfile_disable_overriding();
