#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>
#include "../rf/misc.h"
#include "../rf/object.h"
#include "../rf/entity.h"
#include "../rf/multi.h"
#include "../multi/multi.h"

FunHook<void()> do_quick_save_hook{
    0x004B5E20,
    []() {
        if (rf::can_save())
            do_quick_save_hook.call_target();
    },
};

template<typename T>
T validate_save_file_num(T value, T limit, const char* what)
{
    if (value > limit) {
        xlog::warn("Save file is corrupted: expected up to %d %s but got %d", limit, what, value);
        return limit;
    }
    return value;
}

void validate_level_save_data(rf::LevelSaveData* data)
{
    data->num_goal_create_events = validate_save_file_num<uint8_t>(data->num_goal_create_events, 8, "goal_create events");
    data->num_alarm_siren_events = validate_save_file_num<uint8_t>(data->num_alarm_siren_events, 8, "alarm_siren events");
    data->num_when_dead_events = validate_save_file_num<uint8_t>(data->num_when_dead_events, 20, "when_dead events");
    data->num_cyclic_timer_events = validate_save_file_num<uint8_t>(data->num_cyclic_timer_events, 12, "cyclic_timer events");
    data->num_make_invulnerable_events = validate_save_file_num<uint8_t>(data->num_make_invulnerable_events, 8, "make_invulnerable events");
    data->num_other_events = validate_save_file_num<uint8_t>(data->num_other_events, 32, "other events");
    data->num_emitters = validate_save_file_num<uint8_t>(data->num_emitters, 16, "emitters");
    data->num_decals = validate_save_file_num<uint8_t>(data->num_decals, 64, "decals");
    data->num_entities = validate_save_file_num<uint8_t>(data->num_entities, 64, "entities");
    data->num_items = validate_save_file_num<uint8_t>(data->num_items, 64, "items");
    data->num_clutter = validate_save_file_num<uint16_t>(data->num_clutter, 512, "clutters");
    data->num_triggers = validate_save_file_num<uint8_t>(data->num_triggers, 96, "triggers");
    data->num_keyframes = validate_save_file_num<uint8_t>(data->num_keyframes, 128, "keyframes");
    data->num_push_regions = validate_save_file_num<uint8_t>(data->num_push_regions, 32, "push regions");
    data->num_persistent_goals = validate_save_file_num<uint8_t>(data->num_persistent_goals, 10, "persistent goals");
    data->num_weapons = validate_save_file_num<uint8_t>(data->num_weapons, 8, "weapons");
    data->num_blood_smears = validate_save_file_num<uint8_t>(data->num_blood_smears, 16, "blood smears");
    data->num_corpse = validate_save_file_num<uint8_t>(data->num_corpse, 32, "corpses");
    data->num_geomod_craters = validate_save_file_num<uint8_t>(data->num_geomod_craters, 128, "geomod craters");
    data->num_killed_room_ids = validate_save_file_num<uint8_t>(data->num_killed_room_ids, 128, "killed rooms");
    data->num_dead_entities = validate_save_file_num<uint8_t>(data->num_dead_entities, 64, "dead entities");
    data->num_deleted_events = validate_save_file_num<uint8_t>(data->num_deleted_events, 32, "deleted events");
}

FunHook<void(rf::LevelSaveData*)> sr_deserialize_all_objects_hook{
    0x004B4FB0,
    [](rf::LevelSaveData *data) {
        validate_level_save_data(data);
        sr_deserialize_all_objects_hook.call_target(data);
    },
};

FunHook<void(rf::LevelSaveData*)> sr_serialize_all_objects_hook{
    0x004B4450,
    [](rf::LevelSaveData *data) {
        sr_serialize_all_objects_hook.call_target(data);
        validate_level_save_data(data);
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
    AsmWriter{0x004B3CAF, 0x004B3CB2}.xor_(asm_regs::ebx, asm_regs::ebx);
}
