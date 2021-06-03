#pragma once

#include "os/vtypes.h"
#include "math/matrix.h"
#include "v3d.h"

namespace rf
{
    struct MvfAnimTrigger
    {
        char name[16] = {0};
        float value = 0.0f;
    };
    static_assert(sizeof(MvfAnimTrigger) == 0x14);

    struct Skeleton
    {
        char mvf_filename[64] = {0};
        MvfAnimTrigger triggers[2];
        int data_size = 0;
        bool internally_allocated = 0;
        int base_usage_count = 0;
        int instance_usage_count = 0;
        void *animation_data = nullptr;
    };
    static_assert(sizeof(Skeleton) == 0x7C);

    struct CiAnimInfo
    {
        int anim_index;
        int cur_time;
        float mix_percent;
    };

    struct CiBoneInfo
    {
        short combined_update_iteration;
        short final_update_iteration;
        Matrix3 field_4;
        int field_28;
        float field_2C;
    };

    struct CharacterTag
    {
        const char *name;
        Matrix43 trans;
        int parent_index;
    };

    struct Bone
    {
        char name[24];
        Matrix43 transform;
        int parent;
    };

    struct CharacterMesh
    {
        V3d v3d_file;
        V3dMesh *mesh;
    };

    struct Character
    {
        char name[65];
        int flags;
        int num_bones;
        Bone bones[50];
        ubyte bone_order_list[50];
        int num_anims;
        Skeleton *animations[172];
        bool anim_is_state[172];
        int num_tags;
        CharacterTag tags[32];
        int num_character_meshes;
        CharacterMesh character_meshes[1];
        int root;
    };
    static_assert(sizeof(Character) == 0x1A58);

    static auto& skeleton_link_base = addr_as_ref<Skeleton *__cdecl(const char *filename)>(0x00539D00);
    static auto& skeleton_unlink_base = addr_as_ref<void __cdecl(Skeleton *s, bool force_unload)>(0x00539D20);
    static auto& skeleton_page_in = addr_as_ref<bool __cdecl(const char *filename, void *data_block)>(0x0053A980);
}
