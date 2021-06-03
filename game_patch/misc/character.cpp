#include <unordered_map>
#include <memory>
#include <cstring>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <common/utils/string-utils.h>
#include <xlog/xlog.h>
#include "../rf/character.h"
#include "../rf/crt.h"
#include "../rf/os/os.h"

static std::unordered_map<std::string, std::unique_ptr<rf::Skeleton>> skeletons;

static void skeleton_cleanup_one(rf::Skeleton* sp)
{
    if (sp->internally_allocated && sp->animation_data) {
        rf::rf_free(sp->animation_data);
        sp->animation_data = nullptr;
    }
}

static FunHook<rf::Skeleton*(const char*)> skeleton_find_hook{
    0x00539BE0,
    [](const char *filename) {
        std::string key = string_to_lower(get_filename_without_ext(filename));
        auto it = skeletons.find(key);
        if (it == skeletons.end()) {
            auto p = skeletons.insert({key, std::make_unique<rf::Skeleton>()});
            it = p.first;
            auto& skeleton = it->second;
            std::strncpy(skeleton->mvf_filename, filename, std::size(skeleton->mvf_filename) - 1);
            skeleton->mvf_filename[std::size(skeleton->mvf_filename) - 1] = '\0';
            xlog::trace("Allocated skeleton: %s (total %u)", filename, skeletons.size());
        }
        return it->second.get();
    },
};

static FunHook<void(rf::Skeleton*, bool)> skeleton_unlink_base_hook{
    0x00539D20,
    [](rf::Skeleton* sp, bool force_unload) {
        sp->base_usage_count = std::max(sp->base_usage_count - 1, 0);
        if (sp->base_usage_count == 0 || force_unload) {
            if (sp->base_usage_count != 0) {
                xlog::warn("Expected 0 base usages for skeleton '%s' but got %d", sp->mvf_filename, sp->base_usage_count);
            }
            xlog::trace("Unloading skeleton: %s (total %u)", sp->mvf_filename, skeletons.size());
            skeleton_cleanup_one(sp);
            std::string key = string_to_lower(get_filename_without_ext(sp->mvf_filename));
            skeletons.erase(key);
            // Note: skeleton is deallocated here
        }
    },
};

static FunHook<void()> skeleton_init_hook{
    0x00539D90,
    []() {
        skeletons.reserve(800);
    },
};

static FunHook<void()> skeleton_close_hook{
    0x00539DB0,
    []() {
        for (auto& p : skeletons) {
            auto& skeleton = p.second;
            if (skeleton->base_usage_count > 0) {
                xlog::warn("Expected 0 base usages for skeleton '%s' but got %d",
                    skeleton->mvf_filename, skeleton->base_usage_count);
            }
            if (skeleton->instance_usage_count > 0) {
                xlog::warn("Expected 0 instance usages for skeleton '%s' but got %d",
                    skeleton->mvf_filename, skeleton->instance_usage_count);
            }
            skeleton_cleanup_one(skeleton.get());
        }
        skeletons.clear();
    },
};

void skeleton_flush()
{
    auto it = skeletons.begin();
    while (it != skeletons.end()) {
        rf::Skeleton* sp = it->second.get();
        if (sp->base_usage_count == 0) {
            xlog::trace("Unloading unused skeleton '%s'", sp->mvf_filename);
            skeleton_cleanup_one(sp);
            it = skeletons.erase(it);
        }
        else {
            ++it;
        }
    }
}

static int __fastcall character_load_animation(rf::Character *this_, int, const char *anim_filename, bool is_state, [[maybe_unused]] bool unused)
{
    rf::Skeleton* sp = rf::skeleton_link_base(anim_filename);
    if (!sp) {
        RF_DEBUG_ERROR("Couldn't load skeleton!");
    }
    for (int i = 0; i < this_->num_anims; ++i) {
        if (this_->animations[i] == sp && this_->anim_is_state[i] == is_state) {
            // Unlink call is missing in oryginal code (RF bug)
            rf::skeleton_unlink_base(sp, false);
            xlog::trace("Animation '%s' already used by character '%s' (%d base usages)", anim_filename, this_->name, sp->base_usage_count);
            return i;
        }
    }
    // Animation not found - check if we can add it
    if (this_->num_anims >= static_cast<int>(std::size(this_->animations))) {
        // Protect from buffer overflow
        RF_DEBUG_ERROR("Too many animations!");
    }
    // Add animation skeleton
    int anim_index = this_->num_anims++;
    this_->animations[anim_index] = sp;
    this_->anim_is_state[anim_index] = is_state;
    rf::skeleton_page_in(anim_filename, 0);
    xlog::trace("Animation '%s' loaded for character '%s' (%d base usages) this %p sp %p anim_index %d is_state %d",
        anim_filename, this_->name, sp->base_usage_count, this_, sp, anim_index, is_state);
    return anim_index;
}

static FunHook character_load_animation_hook{0x0051CC10, character_load_animation};

static CodeInjection character_delete_character_injection{
    0x0051C981,
    [](auto& regs) {
        rf::Character* cp = regs.ebx;
        for (int i = 0; i < cp->num_anims; ++i) {
            rf::skeleton_unlink_base(cp->animations[i], false);
        }
        regs.eip = 0x0051CA28;
    },
};

FunHook<void()> character_level_init_hook{
    0x0051D980,
    []() {
        character_level_init_hook.call_target();
        // Destroy unused skeletons (they may have been loaded by page in rfl chunk)
        skeleton_flush();
    },
};

void character_apply_patch()
{
    skeleton_find_hook.install();
    skeleton_unlink_base_hook.install();
    skeleton_init_hook.install();
    skeleton_close_hook.install();
    character_load_animation_hook.install();
    character_delete_character_injection.install();
    character_level_init_hook.install();
}
