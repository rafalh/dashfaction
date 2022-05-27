#pragma once

#include <patch_common/MemUtils.h>

namespace rf {
    struct Player;
}

namespace rf::sr {
    static auto& save_game = addr_as_ref<bool(const char *filename, Player *pp)>(0x004B3B30);
    static auto& can_save_now = addr_as_ref<bool()>(0x004B61A0);

    static auto& savegame_path = addr_as_ref<char[260]>(0x007DB3EC);
}
