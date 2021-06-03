#include <unordered_map>
#include <memory>
#include <cstring>
#include <patch_common/FunHook.h>
#include <common/utils/string-utils.h>
#include <xlog/xlog.h>
#include "../rf/character.h"
#include "../rf/crt.h"

static std::unordered_map<std::string, std::unique_ptr<rf::Skeleton>> skeletons;

static FunHook<rf::Skeleton*(const char*)> skeleton_find_hook{
    0x00539BE0,
    [](const char *filename) {
        std::string key{string_to_lower(get_filename_without_ext(filename))};
        auto it = skeletons.find(key);
        if (it != skeletons.end() && !it->second->mvf_filename[0]) {
            it = skeletons.end();
        }
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
            if (skeleton->internally_allocated && skeleton->animation_data) {
                rf::rf_free(skeleton->animation_data);
            }
        }
    },
};

void character_apply_patch()
{
    skeleton_find_hook.install();
    skeleton_init_hook.install();
    skeleton_close_hook.install();
}
