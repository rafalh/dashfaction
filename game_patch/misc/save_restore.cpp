#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>
#include "../rf/misc.h"
#include "../rf/object.h"
#include "../rf/entity.h"
#include "../rf/multi.h"
#include "../rf/save_restore.h"
#include "../multi/multi.h"

FunHook<void()> do_quick_save_hook{
    0x004B5E20,
    []() {
        if (rf::sr::can_save_now())
            do_quick_save_hook.call_target();
    },
};

template<typename T>
void validate_save_file_num(T& value, T limit, const char* what)
{
    if (value > limit) {
        xlog::warn("Save file is corrupted: expected up to %d %s but got %d", limit, what, value);
        value = limit;
    }
}

void validate_level_save_data(rf::sr::LevelData* data)
{
    validate_save_file_num<uint8_t>(data->num_goal_create_events, rf::sr::MAX_GOAL_CREATE_EVENTS, "goal_create events");
    validate_save_file_num<uint8_t>(data->num_alarm_siren_events, rf::sr::MAX_ALARM_SIREN_EVENTS, "alarm_siren events");
    validate_save_file_num<uint8_t>(data->num_when_dead_events, rf::sr::MAX_WHEN_DEAD_EVENTS, "when_dead events");
    validate_save_file_num<uint8_t>(data->num_cyclic_timer_events, rf::sr::MAX_CYCLIC_TIMER_EVENTS, "cyclic_timer events");
    validate_save_file_num<uint8_t>(data->num_make_invulnerable_events, rf::sr::MAX_MAKE_INVULNERABLE_EVENTS, "make_invulnerable events");
    validate_save_file_num<uint8_t>(data->num_other_events, rf::sr::MAX_OTHER_EVENTS, "other events");
    validate_save_file_num<uint8_t>(data->num_emitters, rf::sr::MAX_EMITTERS, "emitters");
    validate_save_file_num<uint8_t>(data->num_decals, rf::sr::MAX_DECALS, "decals");
    validate_save_file_num<uint8_t>(data->num_entities, rf::sr::MAX_ENTITIES, "entities");
    validate_save_file_num<uint8_t>(data->num_items, rf::sr::MAX_ITEMS, "items");
    validate_save_file_num<uint16_t>(data->num_clutter, rf::sr::MAX_CLUTTERS, "clutters");
    validate_save_file_num<uint8_t>(data->num_triggers, rf::sr::MAX_TRIGGERS, "triggers");
    validate_save_file_num<uint8_t>(data->num_keyframes, rf::sr::MAX_MOVERS, "keyframes");
    validate_save_file_num<uint8_t>(data->num_push_regions, rf::sr::MAX_PUSH_REGIONS, "push regions");
    validate_save_file_num<uint8_t>(data->num_persistent_goals, rf::sr::MAX_PERSISTENT_GOALS, "persistent goals");
    validate_save_file_num<uint8_t>(data->num_weapons, rf::sr::MAX_WEAPONS, "weapons");
    validate_save_file_num<uint8_t>(data->num_blood_pools, rf::sr::MAX_BLOOD_POOLS, "blood pools");
    validate_save_file_num<uint8_t>(data->num_corpse, rf::sr::MAX_CORPSES, "corpses");
    validate_save_file_num<uint8_t>(data->num_geomod_craters, rf::sr::MAX_GEOMOD_CRATERS, "geomod craters");
    validate_save_file_num<uint8_t>(data->num_killed_room_ids, rf::sr::MAX_KILLED_ROOMS, "killed rooms");
    validate_save_file_num<uint8_t>(data->num_dead_entities, rf::sr::MAX_ENTITIES, "dead entities");
    validate_save_file_num<uint8_t>(data->num_deleted_events, rf::sr::MAX_DELETED_EVENTS, "deleted events");
}

FunHook<void(rf::sr::LevelData*)> sr_deserialize_all_objects_hook{
    0x004B4FB0,
    [](rf::sr::LevelData *data) {
        validate_level_save_data(data);
        sr_deserialize_all_objects_hook.call_target(data);
    },
};

FunHook<void(rf::sr::LevelData*)> sr_serialize_all_objects_hook{
    0x004B4450,
    [](rf::sr::LevelData *data) {
        sr_serialize_all_objects_hook.call_target(data);
        validate_level_save_data(data);
    },
};

CodeInjection entity_serialize_all_state_oob_fix{
    0x0042B340,
    [](auto& regs) {
        auto num_entities = addr_as_ref<int>(regs.esp + 0x78 - 0x64);
        auto num_dead_entities = addr_as_ref<int>(regs.esp + 0x78 - 0x60);
        if (num_entities >= rf::sr::MAX_ENTITIES || num_dead_entities >= rf::sr::MAX_ENTITIES) {
            xlog::warn("Too many entities to save (limit: %d)", rf::sr::MAX_ENTITIES);
            regs.eip = 0x0042BA0E;
        }
    },
};

CodeInjection item_serialize_all_state_oob_fix{
    0x00459C9A,
    [](auto& regs) {
        int num_items = regs.ebx;
        if (num_items >= rf::sr::MAX_ITEMS) {
            xlog::warn("Too many items to save (limit: %d)", rf::sr::MAX_ITEMS);
            regs.eip = 0x00459CF4;
        }
    },
};

CodeInjection clutter_serialize_all_state_oob_fix{
    0x00410F8E,
    [](auto& regs) {
        int num_clutters = regs.ebx;
        if (num_clutters >= rf::sr::MAX_CLUTTERS) {
            xlog::warn("Too many clutters to save (limit: %d)", rf::sr::MAX_CLUTTERS);
            regs.eip = 0x00411000;
        }
    },
};

CodeInjection corpse_serialize_all_state_oob_fix{
    0x004176C7,
    [](auto& regs) {
        int num_corpses = regs.eax;
        if (num_corpses >= rf::sr::MAX_CORPSES) {
            xlog::warn("Too many corpses to save (limit: %d)", rf::sr::MAX_CORPSES);
            regs.eip = 0x00417849;
        }
    },
};

CodeInjection trigger_serialize_all_state_oob_fix{
    0x004C0E4E,
    [](auto& regs) {
        int num_triggers = regs.ebx;
        if (num_triggers >= rf::sr::MAX_TRIGGERS) {
            xlog::warn("Too many triggers to save (limit: %d)", rf::sr::MAX_TRIGGERS);
            regs.eip = 0x004C0EC6;
        }
    },
};

CodeInjection mover_serialize_all_state_oob_fix{
    0x0046B3EE,
    [](auto& regs) {
        int num_movers = regs.ebx;
        if (num_movers >= rf::sr::MAX_MOVERS) {
            xlog::warn("Too many movers to save (limit: %d)", rf::sr::MAX_MOVERS);
            regs.eip = 0x0046B482;
        }
    },
};

CodeInjection sr_serialize_decals_oob_fix{
    0x004B4692,
    [](auto& regs) {
        auto num_decals = addr_as_ref<int>(regs.esp + 0x34 - 0x24);
        if (num_decals >= rf::sr::MAX_DECALS) {
            xlog::warn("Too many decals to save (limit: %d)", rf::sr::MAX_DECALS);
            regs.eip = 0x004B4756;
        }
    },
};

CodeInjection weapon_serialize_all_state_oob_fix{
    0x004C8E41,
    [](auto& regs) {
        int num_weapons = regs.ebx;
        if (num_weapons >= rf::sr::MAX_WEAPONS) {
            xlog::warn("Too many weapons to save (limit: %d)", rf::sr::MAX_WEAPONS);
            regs.eip = 0x004C8F0A;
        }
    },
};

FunHook<void(int, int*)> sr_add_handle_for_delayed_resolution_hook{
    0x004B5630,
    [](int uid, int *obj_handle_ptr) {
        auto& sr_num_delayed_handle_resolution = addr_as_ref<int>(0x007DE5A0);
        if (sr_num_delayed_handle_resolution < 1500) {
            sr_add_handle_for_delayed_resolution_hook.call_target(uid, obj_handle_ptr);
        }
        else {
            xlog::warn("Too many handles in restored data");
        }
    },
};

FunHook<void(rf::Object*)> remember_level_transition_objects_meshes_hook{
    0x004B5660,
    [](rf::Object *obj) {
        auto& num_level_transition_objects = addr_as_ref<int>(0x00856058);
        // Note: uid -999 belongs to local player entity and it must be preserved
        if (num_level_transition_objects < 23 || obj->uid == -999) {
            remember_level_transition_objects_meshes_hook.call_target(obj);
        }
        else {
            xlog::warn("Cannot bring object %d to next level", obj->uid);
            rf::obj_flag_dead(obj);
        }
    },
};

CodeInjection quick_save_mem_leak_fix{
    0x004B603F,
    []() {
        auto& saved_label_inited = addr_as_ref<bool>(0x00856054);
        saved_label_inited = true;
    },
};

CodeInjection corpse_deserialize_all_obj_create_patch{
    0x004179E2,
    [](auto& regs) {
        auto save_data = regs.edi;
        auto stack_frame = regs.esp + 0xD0;
        auto create_info = addr_as_ref<rf::ObjectCreateInfo>(stack_frame - 0xA4);
        auto entity_cls_id = addr_as_ref<int>(save_data + 0x144);
        // Create entity before creating the corpse to make sure entity action animations are fully loaded
        // This is needed to make sure pose_action_anim points to a valid animation
        auto entity = rf::entity_create(entity_cls_id, "", -1, create_info.pos, create_info.orient, 0, -1);
        rf::obj_flag_dead(entity);
    },
};

FunHook<void()> quick_save_hook{
    0x004B6160,
    []() {
        quick_save_hook.call_target();
        bool server_side_saving_enabled = rf::is_multi && !rf::is_server && get_df_server_info()
            && get_df_server_info().value().saving_enabled;
        if (server_side_saving_enabled) {
            send_chat_line_packet("/save", nullptr);
        }
    },
};

FunHook<void()> quick_load_hook{
    0x004B6180,
    []() {
        quick_load_hook.call_target();
        bool server_side_saving_enabled = rf::is_multi && !rf::is_server && get_df_server_info()
            && get_df_server_info().value().saving_enabled;
        if (server_side_saving_enabled) {
            send_chat_line_packet("/load", nullptr);
        }
    },
};

void apply_save_restore_patches()
{
    // Dont overwrite player name and prefered weapons when loading saved game
    AsmWriter(0x004B4D99, 0x004B4DA5).nop();
    AsmWriter(0x004B4E0A, 0x004B4E22).nop();

    // Fix crash when loading savegame with missing player entity data
    AsmWriter(0x004B4B47).jmp(0x004B4B7B);

    // Fix creating corrupted saves if cutscene starts in the same frame as quick save button is pressed
    do_quick_save_hook.install();

    // Fix crash caused by buffer overflows in level save/load code
    sr_deserialize_all_objects_hook.install();
    sr_serialize_all_objects_hook.install();
    entity_serialize_all_state_oob_fix.install();
    item_serialize_all_state_oob_fix.install();
    clutter_serialize_all_state_oob_fix.install();
    corpse_serialize_all_state_oob_fix.install();
    trigger_serialize_all_state_oob_fix.install();
    mover_serialize_all_state_oob_fix.install();
    sr_serialize_decals_oob_fix.install();
    weapon_serialize_all_state_oob_fix.install();

    // Fix buffer overflow during level load if there are more than 24 objects in current room
    remember_level_transition_objects_meshes_hook.install();

    // Fix possible buffer overflow in sr_add_handle_for_delayed_resolution
    sr_add_handle_for_delayed_resolution_hook.install();

    // Fix memory leak on quick save
    quick_save_mem_leak_fix.install();

    // Fix crash caused by corpse pose pointing to not loaded entity action animation
    // It only affects corpses there are taken from the previous level
    corpse_deserialize_all_obj_create_patch.install();

    // Save-restore in multi
    quick_save_hook.install();
    quick_load_hook.install();

    // Fix memory corruption when transitioning to 5th level in a sequence and the level has no entry in ponr.tbl
    AsmWriter{0x004B3CAF, 0x004B3CB2}.xor_(asm_regs::ebx, asm_regs::ebx);  // save
    AsmWriter{0x004B5374, 0x004B5377}.xor_(asm_regs::edx, asm_regs::edx);  // transition
}
