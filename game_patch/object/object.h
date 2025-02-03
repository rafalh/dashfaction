#pragma once

namespace rf
{
    struct Object;
    struct Player;
}

void object_do_patch();
void obj_light_free_one(rf::Object *objp);
void obj_light_init_object(rf::Object *objp);
void trigger_send_state_info(rf::Player* player);

constexpr size_t old_obj_limit = 1024;
constexpr size_t obj_limit = 65536;
