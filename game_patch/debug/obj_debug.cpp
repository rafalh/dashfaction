#include "../console/console.h"
#include "../rf/object.h"
#include "../rf/entity.h"
#include "../rf/player.h"
#include "../rf/multi.h"
#include "debug_internal.h"

std::optional<float> g_target_rotate_speed;


namespace rf
{

struct GSolid;
struct GFace;

struct PersonasTbl
{
    String name;
    int alert_snd;
    int under_alert_snd;
    int cower_snd;
    int flee_snd;
    int use_key_snd;
    int healing_gone_snd;
    int heal_ignore_snd;
    int timeout_snd;
    int humming_snd;
    int fight_single_snd;
    int fight_multi_snd;
    int panic_snd;
};

auto& personas = addr_as_ref<PersonasTbl[0x10]>(0x0062F998);
auto& entity_state_names = addr_as_ref<String[17]>(0x0062F208);
auto& entity_action_names = addr_as_ref<String[0x2D]>(0x005CAEE0);
auto& object_list = addr_as_ref<Object>(0x0073D880);
auto& target_obj_handle = addr_as_ref<int>(0x007C7190);

auto entity_is_playing_action_animation = addr_as_ref<bool(Entity* entity, int action_idx)>(0x00428D10);
auto ai_get_attack_range = addr_as_ref<float(AiInfo& ai)>(0x004077A0);
auto entity_set_next_state_anim = addr_as_ref<void(Entity* entity, int state, float transition_time)>(0x0042A580);
auto entity_play_action_animation = addr_as_ref<void(Entity* entity, int action, float transition_time, bool hold_last_frame, bool with_sound)>(0x00428C90);
auto vmesh_stop_all_actions = addr_as_ref<void(VMesh* vmesh)>(0x00503400);

}

rf::Object* FindClosestObject()
{
    if (!rf::local_player->cam)
        return nullptr;
    rf::Vector3 cam_pos;
    rf::Matrix3 cam_orient;
    rf::camera_get_pos(&cam_pos, rf::local_player->cam);
    rf::camera_get_orient(&cam_orient, rf::local_player->cam);
    auto obj = rf::object_list.next_obj;
    rf::Object* best_obj = nullptr;
    float best_dist = 100.0f; // max dist
    rf::Vector3 fw = cam_orient.fvec;
    while (obj != &rf::object_list) {
        auto dir = obj->pos - cam_pos;
        float dist = dir.len();
        float dotp = dir.dot_prod(fw);
        if (obj->handle != rf::local_player->entity_handle && dotp > 0.0f && dist < best_dist) {
            best_dist = dist;
            best_obj = obj;
        }
        obj = obj->next_obj;
    }
    return best_obj;
}

rf::Object* FindObjectInReticle()
{
    struct LevelCollisionOut
    {
        rf::Vector3 hit_point;
        float distance;
        int obj_handle;
        void* face;
    };
    auto& CollideLineSegmentLevel =
        addr_as_ref<bool(rf::Vector3& p0, rf::Vector3& p1, rf::Object *ignore1, rf::Object *ignore2,
        LevelCollisionOut *out, float collision_radius, char use_mesh_collide, float bbox_size_factor)>(0x0049C690);

    if (!rf::local_player->cam)
        return nullptr;
    LevelCollisionOut col_info;
    col_info.face = 0;
    col_info.obj_handle = -1;
    rf::Vector3 p0;
    rf::Matrix3 orient;
    rf::camera_get_pos(&p0, rf::local_player->cam);
    rf::camera_get_orient(&orient, rf::local_player->cam);
    rf::Vector3 p1 = p0 + orient.fvec * 100.0f;
    rf::Entity* entity = rf::entity_from_handle(rf::local_player->entity_handle);
    bool hit = CollideLineSegmentLevel(p0, p1, entity, nullptr, &col_info, 0.0f, false, 1.0f);
    if (hit && col_info.obj_handle != -1)
        return rf::obj_from_handle(col_info.obj_handle);
    else
        return nullptr;
}

ConsoleCommand2 dbg_target_uid_cmd{
    "d_target_uid",
    [](std::optional<int> uid_opt) {
        // uid -999 is used by local entity
        if (uid_opt) {
            int uid = uid_opt.value();
            rf::Object* obj = rf::obj_lookup_from_uid(uid);
            if (!obj) {
                rf::console_printf("UID not found!");
                return;
            }
            rf::target_obj_handle = obj->handle;
        }
        else {
            rf::target_obj_handle = rf::local_player->entity_handle;
        }
        auto obj = rf::obj_from_handle(rf::target_obj_handle);
        if (obj)
            rf::console_printf("Target object: uid %d, name '%s'", obj->uid, obj->name.c_str());
        else
            rf::console_printf("Target object not found");
    },
};

ConsoleCommand2 dbg_target_closest_cmd{
    "d_target_closest",
    []() {
        auto obj = FindClosestObject();
        rf::target_obj_handle = obj ? obj->handle : 0;
        if (obj)
            rf::console_printf("Target object: uid %d, name '%s'", obj->uid, obj->name.c_str());
        else
            rf::console_printf("Target object not found");
    },
};

ConsoleCommand2 dbg_target_reticle_cmd{
    "d_target_reticle",
    []() {
        auto obj = FindObjectInReticle();
        rf::target_obj_handle = obj ? obj->handle : 0;
        if (obj)
            rf::console_printf("Target object: uid %d, name '%s'", obj->uid, obj->name.c_str());
        else
            rf::console_printf("Target object not found");
    },
};

ConsoleCommand2 dbg_entity_state_cmd{
    "d_entity_state",
    [](std::optional<int> state_opt) {
        auto entity = rf::entity_from_handle(rf::target_obj_handle);
        if (!entity) {
            return;
        }
        int num_states = std::size(rf::entity_state_names);
        if (!state_opt) {
            entity->force_state_anim =  (entity->force_state_anim + 1) % num_states;
        }
        else if (state_opt.value() >= 0 && state_opt.value() < num_states) {
            entity->force_state_anim = state_opt.value();
        }
        rf::console_printf("Entity state: %s (%d)", rf::entity_action_names[entity->force_state_anim].c_str(), entity->force_state_anim);
    },
};

ConsoleCommand2 dbg_entity_action_cmd{
    "d_entity_action",
    [](std::optional<int> action_opt) {
        auto entity = rf::entity_from_handle(rf::target_obj_handle);
        if (!entity) {
            return;
        }
        static int last_action = -1;
        int num_actions = std::size(rf::entity_action_names);
        if (!action_opt) {
            last_action = (last_action + 1) % num_actions;
        }
        else if (action_opt.value() >= 0 && action_opt.value() < num_actions) {
            last_action = action_opt.value();
        }
        rf::vmesh_stop_all_actions(entity->vmesh);
        rf::entity_play_action_animation(entity, last_action, 1.0f, true, true);

        rf::console_printf("Entity action: %s (%d)", rf::entity_action_names[last_action].c_str(), last_action);
    },
};

ConsoleCommand2 dbg_spin_cmd{
    "d_spin",
    [](float speed) {
        g_target_rotate_speed = {speed};
    },
};

ConsoleCommand2 dbg_ai_pause_cmd{
    "d_ai_pause",
    []() {
        auto& ai_pause = addr_as_ref<bool>(0x005AF46D);
        ai_pause = !ai_pause;
        rf::console_printf("AI pause: %d", ai_pause);
    },
};

const char* get_obj_type_name(rf::Object& obj)
{
    switch (obj.type) {
        case rf::OT_ENTITY: return "entity";
        case rf::OT_ITEM: return "item";
        case rf::OT_WEAPON: return "weapon";
        case rf::OT_DEBRIS: return "debris";
        case rf::OT_CLUTTER: return "clutter";
        case rf::OT_TRIGGER: return "trigger";
        case rf::OT_EVENT: return "event";
        case rf::OT_CORPSE: return "corpse";
        case rf::OT_MOVER: return "mover";
        case rf::OT_MOVER_BRUSH: return "mover_brash";
        case rf::OT_GLARE: return "glare";
        default: return "-";
    }
}

const char* get_obj_class_name(rf::Object& obj)
{
    if (obj.type == rf::OT_ENTITY)
        return reinterpret_cast<rf::Entity&>(obj).info->name.c_str();
    else
        return "-";
}

const char* get_ai_mode_name(rf::AiMode ai_mode)
{
    switch (ai_mode) {
        case rf::AIM_NONE: return "NONE";
        case rf::AIM_CATATONIC: return "CATATONIC";
        case rf::AIM_WAITING: return "WAITING";
        case rf::AIM_ATTACK: return "ATTACK";
        case rf::AIM_WAYPOINTS: return "WAYPOINTS";
        case rf::AIM_COLLECTING: return "COLLECTING";
        case rf::AIM_AFTER_NOISE: return "AFTER_NOISE";
        case rf::AIM_FLEE: return "FLEE";
        case rf::AIM_LOOK_AT: return "LOOK_AT";
        case rf::AIM_SHOOT_AT: return "SHOOT_AT";
        case rf::AIM_WATCHFUL: return "WATCHFUL";
        case rf::AIM_MOTION_DETECTION: return "MOTION_DETECTION";
        case rf::AIM_C: return "C";
        case rf::AIM_TURRET_UNK: return "TURRET_UNK";
        case rf::AIM_HEALING: return "HEALING";
        case rf::AIM_CAMERA_UNK: return "CAMERA_UNK";
        case rf::AIM_ACTIVATE_ALARM: return "ACTIVATE_ALARM";
        case rf::AIM_PANIC: return "PANIC";
        default: return "?";
    }
}

const char* get_ai_attack_style_name(rf::AiAttackStyle attack_style)
{
    switch (attack_style) {
        case rf::AIAS_DEFAULT: return "DEFAULT";
        case rf::AIAS_EVASIVE: return "EVASIVE";
        case rf::AIAS_STAND_GROUND: return "STAND_GROUND";
        case rf::AIAS_DIRECT: return "DIRECT";
        default: return "?";
    }
}

const char* get_friendliness_name(rf::Friendliness friendliness)
{
    switch (friendliness) {
        case rf::UNFRIENDLY: return "UNFRIENDLY";
        case rf::NEUTRAL: return "NEUTRAL";
        case rf::FRIENDLY: return "FRIENDLY";
        case rf::OUTCAST: return "OUTCAST";
        default: return "?";
    }
}

void register_obj_debug_commands()
{
    dbg_target_uid_cmd.register_cmd();
    dbg_target_closest_cmd.register_cmd();
    dbg_target_reticle_cmd.register_cmd();
    dbg_entity_state_cmd.register_cmd();
    dbg_entity_action_cmd.register_cmd();
    dbg_spin_cmd.register_cmd();
    dbg_ai_pause_cmd.register_cmd();
}

void render_obj_debug_ui()
{
    auto object = rf::obj_from_handle(rf::target_obj_handle);
    if (!object) {
        return;
    }

    DebugNameValueBox dbg_hud{rf::gr_screen_width() - 300, 200};

    if (!rf::local_player || !rf::local_player->cam) {
        return;
    }

    rf::Vector3 cam_pos;
    rf::camera_get_pos(&cam_pos, rf::local_player->cam);

    auto entity = object->type == rf::OT_ENTITY ? reinterpret_cast<rf::Entity*>(object) : nullptr;
    auto move_mode_names = addr_as_ref<const char*[16]>(0x00596384);

    dbg_hud.Print("name", object->name.c_str());
    dbg_hud.Printf("uid", "%d", object->uid);
    dbg_hud.Print("type", get_obj_type_name(*object));
    dbg_hud.Print("class", get_obj_class_name(*object));
    dbg_hud.Printf("dist", "%.3f", (cam_pos - object->pos).len());
    dbg_hud.Printf("atck_dist", "%.0f", entity ? rf::ai_get_attack_range(entity->ai) : 0.0f);
    dbg_hud.Printf("life", "%.0f", object->life);
    dbg_hud.Printf("room", "%d", object->room ? struct_field_ref<int>(object->room, 0x20) : -1);
    dbg_hud.Print("pos", object->pos);
    if (entity) {
        dbg_hud.Print("eye_pos", entity->view_pos);
        dbg_hud.Printf("envsuit", "%.0f", object->armor);
        dbg_hud.Print("mode", get_ai_mode_name(entity->ai.mode));
        if (entity->ai.submode)
            dbg_hud.Printf("submode", "%d", entity->ai.submode);
        else
            dbg_hud.Print("submode", "NONE");
        dbg_hud.Print("style", get_ai_attack_style_name(entity->ai.ai_attack_style));
        dbg_hud.Print("friend", get_friendliness_name(object->friendliness));
        auto target_obj = rf::obj_from_handle(entity->ai.target_obj_handle);
        dbg_hud.Print("target", target_obj ? target_obj->name.c_str() : "none");
        dbg_hud.Printf("accel", "%.1f", entity->info->acceleration);
        dbg_hud.Printf("mvmode", "%s", move_mode_names[entity->movement_mode->id]);
        dbg_hud.Print("deaf", (entity->ai.flags & 0x800) ? "yes" : "no");
        dbg_hud.Print("pos", object->pos);
        auto feet = object->pos;
        feet.y = object->p_data.bbox_min.y;
        dbg_hud.Print("feet", feet);
        dbg_hud.Print("state", rf::entity_state_names[entity->current_state]);

        const char* action_name = "none";
        for (size_t action_idx = 0; action_idx < std::size(rf::entity_action_names); ++action_idx) {
            if (rf::entity_is_playing_action_animation(entity, action_idx)) {
                action_name = rf::entity_action_names[action_idx];
            }
        }
        dbg_hud.Print("action", action_name);
        dbg_hud.Printf("persona", entity->info->persona >= 0 ? rf::personas[entity->info->persona].name.c_str() : "none");
    }

    if (g_target_rotate_speed) {
        object->p_data.rotvel.y = g_target_rotate_speed.value();
    }
}
