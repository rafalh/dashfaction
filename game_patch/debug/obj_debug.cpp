#include "../console/console.h"
#include "../rf/object.h"
#include "../rf/entity.h"
#include "../rf/player.h"
#include "../rf/network.h"
#include "debug_internal.h"

std::optional<float> g_target_rotate_speed;


namespace rf
{

class RflGeometry;
class RflFace;

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

auto& personas = AddrAsRef<PersonasTbl[0x10]>(0x0062F998);
auto& entity_state_names = AddrAsRef<String[17]>(0x0062F208);
auto& entity_action_names = AddrAsRef<String[0x2D]>(0x005CAEE0);
auto& object_list = AddrAsRef<Object>(0x0073D880);
auto& target_obj_handle = AddrAsRef<int>(0x007C7190);

auto EntityIsActionAnimPlaying = AddrAsRef<bool(EntityObj* entity, int action_idx)>(0x00428D10);
auto AiGetAttackRange = AddrAsRef<float(AiInfo& ai)>(0x004077A0);
auto EntityEnterState = AddrAsRef<void(EntityObj* entity, int state, float transition_time)>(0x0042A580);
auto EntityStartAction = AddrAsRef<void(EntityObj* entity, int action, float transition_time, bool hold_last_frame, bool with_sound)>(0x00428C90);
auto AnimMeshResetActionAnim = AddrAsRef<void(AnimMesh* anim_mesh)>(0x00503400);

}

rf::Object* FindClosestObject()
{
    if (!rf::local_player->camera)
        return nullptr;
    rf::Vector3 cam_pos;
    rf::Matrix3 cam_orient;
    rf::CameraGetPos(&cam_pos, rf::local_player->camera);
    rf::CameraGetOrient(&cam_orient, rf::local_player->camera);
    auto obj = rf::object_list.next_obj;
    rf::Object* best_obj = nullptr;
    float best_dist = 100.0f; // max dist
    rf::Vector3 fw = cam_orient.fvec;
    while (obj != &rf::object_list) {
        auto dir = obj->pos - cam_pos;
        float dist = dir.Len();
        float dotp = dir.DotProd(fw);
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
    struct CollisionInfo2
    {
        rf::Vector3 hit_point;
        float distance;
        int obj_handle;
        void* face;
    };
    auto& CollideLineSegmentLevel =
        AddrAsRef<bool(rf::Vector3& p0, rf::Vector3& p1, rf::Object *ignore1, rf::Object *ignore2,
        CollisionInfo2 *out, float collision_radius, char use_mesh_collide, float bbox_size_factor)>(0x0049C690);

    if (!rf::local_player->camera)
        return nullptr;
    CollisionInfo2 col_info;
    col_info.face = 0;
    col_info.obj_handle = -1;
    rf::Vector3 p0;
    rf::Matrix3 orient;
    rf::CameraGetPos(&p0, rf::local_player->camera);
    rf::CameraGetOrient(&orient, rf::local_player->camera);
    rf::Vector3 p1 = p0 + orient.fvec * 100.0f;
    rf::EntityObj* local_entity = rf::EntityGetByHandle(rf::local_player->entity_handle);
    bool hit = CollideLineSegmentLevel(p0, p1, local_entity, nullptr, &col_info, 0.0f, false, 1.0f);
    if (hit && col_info.obj_handle != -1)
        return rf::ObjGetByHandle(col_info.obj_handle);
    else
        return nullptr;
}

DcCommand2 dbg_target_uid_cmd{
    "d_target_uid",
    [](std::optional<int> uid_opt) {
        // uid -999 is used by local entity
        if (uid_opt) {
            int uid = uid_opt.value();
            rf::Object* obj = rf::ObjGetByUid(uid);
            if (!obj) {
                rf::DcPrintf("UID not found!");
                return;
            }
            rf::target_obj_handle = obj->handle;
        }
        else {
            rf::target_obj_handle = rf::local_player->entity_handle;
        }
        auto obj = rf::ObjGetByHandle(rf::target_obj_handle);
        if (obj)
            rf::DcPrintf("Target object: uid %d, name '%s'", obj->uid, obj->name.CStr());
        else
            rf::DcPrintf("Target object not found");
    },
};

DcCommand2 dbg_target_closest_cmd{
    "d_target_closest",
    []() {
        auto obj = FindClosestObject();
        rf::target_obj_handle = obj ? obj->handle : 0;
        if (obj)
            rf::DcPrintf("Target object: uid %d, name '%s'", obj->uid, obj->name.CStr());
        else
            rf::DcPrintf("Target object not found");
    },
};

DcCommand2 dbg_target_reticle_cmd{
    "d_target_reticle",
    []() {
        auto obj = FindObjectInReticle();
        rf::target_obj_handle = obj ? obj->handle : 0;
        if (obj)
            rf::DcPrintf("Target object: uid %d, name '%s'", obj->uid, obj->name.CStr());
        else
            rf::DcPrintf("Target object not found");
    },
};

DcCommand2 dbg_entity_state_cmd{
    "d_entity_state",
    [](std::optional<int> state_opt) {
        auto entity = rf::EntityGetByHandle(rf::target_obj_handle);
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
        rf::DcPrintf("Entity state: %s (%d)", rf::entity_action_names[entity->force_state_anim].CStr(), entity->force_state_anim);
    },
};

DcCommand2 dbg_entity_action_cmd{
    "d_entity_action",
    [](std::optional<int> action_opt) {
        auto entity = rf::EntityGetByHandle(rf::target_obj_handle);
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
        rf::AnimMeshResetActionAnim(entity->anim_mesh);
        rf::EntityStartAction(entity, last_action, 1.0f, true, true);

        rf::DcPrintf("Entity action: %s (%d)", rf::entity_action_names[last_action].CStr(), last_action);
    },
};

DcCommand2 dbg_spin_cmd{
    "d_spin",
    [](float speed) {
        g_target_rotate_speed = {speed};
    },
};

DcCommand2 dbg_ai_pause_cmd{
    "d_ai_pause",
    []() {
        auto& ai_pause = AddrAsRef<bool>(0x005AF46D);
        ai_pause = !ai_pause;
        rf::DcPrintf("AI pause: %d", ai_pause);
    },
};

const char* GetObjTypeName(rf::Object& obj)
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
        case rf::OT_MOVING_GROUP: return "moving group";
        case rf::OT_MOVER: return "mover";
        case rf::OT_GLARE: return "glare";
        default: return "-";
    }
}

const char* GetObjClassName(rf::Object& obj)
{
    if (obj.type == rf::OT_ENTITY)
        return reinterpret_cast<rf::EntityObj&>(obj).cls->name.CStr();
    else
        return "-";
}

const char* GetAiModeName(rf::AiMode ai_mode)
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

const char* GetAiAttackStyleName(rf::AiAttackStyle attack_style)
{
    switch (attack_style) {
        case rf::AIAS_DEFAULT: return "DEFAULT";
        case rf::AIAS_EVASIVE: return "EVASIVE";
        case rf::AIAS_STAND_GROUND: return "STAND_GROUND";
        case rf::AIAS_DIRECT: return "DIRECT";
        default: return "?";
    }
}

const char* GetFriendlinessName(rf::Friendliness friendliness)
{
    switch (friendliness) {
        case rf::UNFRIENDLY: return "UNFRIENDLY";
        case rf::NEUTRAL: return "NEUTRAL";
        case rf::FRIENDLY: return "FRIENDLY";
        case rf::OUTCAST: return "OUTCAST";
        default: return "?";
    }
}

void RegisterObjDebugCommands()
{
    dbg_target_uid_cmd.Register();
    dbg_target_closest_cmd.Register();
    dbg_target_reticle_cmd.Register();
    dbg_entity_state_cmd.Register();
    dbg_entity_action_cmd.Register();
    dbg_spin_cmd.Register();
    dbg_ai_pause_cmd.Register();
}

void RenderObjDebugUI()
{
    auto object = rf::ObjGetByHandle(rf::target_obj_handle);
    if (!object) {
        return;
    }

    DebugNameValueBox dbg_hud{rf::GrGetMaxWidth() - 300, 200};

    if (!rf::local_player || !rf::local_player->camera) {
        return;
    }

    rf::Vector3 cam_pos;
    rf::CameraGetPos(&cam_pos, rf::local_player->camera);

    auto entity = object->type == rf::OT_ENTITY ? reinterpret_cast<rf::EntityObj*>(object) : nullptr;
    auto move_mode_names = AddrAsRef<const char*[16]>(0x00596384);

    dbg_hud.Print("name", object->name.CStr());
    dbg_hud.Printf("uid", "%d", object->uid);
    dbg_hud.Print("type", GetObjTypeName(*object));
    dbg_hud.Print("class", GetObjClassName(*object));
    dbg_hud.Printf("dist", "%.3f", (cam_pos - object->pos).Len());
    dbg_hud.Printf("atck_dist", "%.0f", entity ? rf::AiGetAttackRange(entity->ai_info) : 0.0f);
    dbg_hud.Printf("life", "%.0f", object->life);
    dbg_hud.Printf("room", "%d", object->room ? StructFieldRef<int>(object->room, 0x20) : -1);
    dbg_hud.Print("pos", object->pos);
    if (entity) {
        dbg_hud.Print("eye_pos", entity->view_pos);
        dbg_hud.Printf("envsuit", "%.0f", object->armor);
        dbg_hud.Print("mode", GetAiModeName(entity->ai_info.ai_mode));
        if (entity->ai_info.ai_submode)
            dbg_hud.Printf("submode", "%d", entity->ai_info.ai_submode);
        else
            dbg_hud.Print("submode", "NONE");
        dbg_hud.Print("style", GetAiAttackStyleName(entity->ai_info.ai_attack_style));
        dbg_hud.Print("friend", GetFriendlinessName(object->friendliness));
        auto target_obj = rf::ObjGetByHandle(entity->ai_info.target_obj_handle);
        dbg_hud.Print("target", target_obj ? target_obj->name.CStr() : "none");
        dbg_hud.Printf("accel", "%.1f", entity->cls->acceleration);
        dbg_hud.Printf("mvmode", "%s", move_mode_names[entity->movement_mode->id]);
        dbg_hud.Print("deaf", (entity->ai_info.flags & 0x800) ? "yes" : "no");
        dbg_hud.Print("pos", object->pos);
        auto feet = object->pos;
        feet.y = object->phys_info.aabb_min.y;
        dbg_hud.Print("feet", feet);
        dbg_hud.Print("state", rf::entity_state_names[entity->current_state]);

        const char* action_name = "none";
        for (size_t action_idx = 0; action_idx < std::size(rf::entity_action_names); ++action_idx) {
            if (rf::EntityIsActionAnimPlaying(entity, action_idx)) {
                action_name = rf::entity_action_names[action_idx];
            }
        }
        dbg_hud.Print("action", action_name);
        dbg_hud.Printf("persona", entity->cls->persona >= 0 ? rf::personas[entity->cls->persona].name.CStr() : "none");
    }

    if (g_target_rotate_speed) {
        object->phys_info.rot_change_unk.y = g_target_rotate_speed.value();
    }
}
