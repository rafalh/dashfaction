#pragma once

#include "math/vector.h"
#include "gr/gr.h"

namespace rf
{
    struct VMesh;
    struct GRoom;

    enum VMeshType
    {
        MESH_TYPE_UNINITIALIZED = 0,
        MESH_TYPE_STATIC = 1,
        MESH_TYPE_CHARACTER = 2,
        MESH_TYPE_ANIM_FX = 3,
    };

    struct VMeshCollisionInput
    {
        Vector3 mesh_pos;
        Matrix3 mesh_orient;
        Vector3 start_pos;
        Vector3 dir;
        float radius;
        int flags;
        Vector3 transformed_start_pos;
        Vector3 transformed_pos_diff;

        VMeshCollisionInput()
        {
            AddrCaller{0x00416190}.this_call(this);
        }
    };
    static_assert(sizeof(VMeshCollisionInput) == 0x68);

    struct VMeshCollisionOutput
    {
        float fraction;
        Vector3 hit_point;
        Vector3 hit_normal;
        short* triangle_indices;

        VMeshCollisionOutput()
        {
            AddrCaller{0x004161D0}.this_call(this);
        }
    };
    static_assert(sizeof(VMeshCollisionOutput) == 0x20);

    struct TexMap
    {
        int tex_handle;
        char name[33];
        int start_frame;
        float playback_rate;
        int anim_type;
    };
    static_assert(sizeof(TexMap) == 0x34);

    struct MeshMaterial
    {
        int material_type;
        int flags;
        bool use_additive_blending;
        Color diffuse_color;
        TexMap texture_maps[2];
        int framerate;
        int num_mix_frames;
        int* mix;
        float specular_level;
        float glossiness;
        float reflection_amount;
        char refl_tex_name[36];
        int refl_tex_handle;
        int num_self_illumination_frames;
        float* self_illumination;
        int num_opacity_frames;
        int* opacity;
    };
    static_assert(sizeof(MeshMaterial) == 0xC8);

    static auto& vmesh_get_type = addr_as_ref<VMeshType(VMesh* vmesh)>(0x00502B00);
    static auto& vmesh_get_name = addr_as_ref<const char*(VMesh* vmesh)>(0x00503470);
    static auto& vmesh_get_num_cspheres = addr_as_ref<int(VMesh* vmesh)>(0x00503250);
    static auto& vmesh_get_csphere =
        addr_as_ref<bool(VMesh* vmesh, int index, Vector3* pos, float* radius)>(0x00503270);
    static auto& vmesh_collide =
        addr_as_ref<bool(VMesh* vmesh, VMeshCollisionInput* in, VMeshCollisionOutput* out, bool clear)>(0x005031F0);
    static auto& vmesh_calc_lighting_data_size = addr_as_ref<int(VMesh* vmesh)>(0x00503F50);
    static auto& vmesh_update_lighting_data = addr_as_ref<
        int(VMesh* vmesh, GRoom* room, const Vector3& pos, const Matrix3& orient, void* mesh_lighting_data)>(0x00504000
    );
    static auto& vmesh_stop_all_actions = addr_as_ref<void(VMesh* vmesh)>(0x00503400);
    static auto& vmesh_get_materials_array =
        addr_as_ref<void(VMesh* vmesh, int* num_materials_out, MeshMaterial** materials_array_out)>(0x00503650);
}
