#pragma once

#include "common.h"

namespace rf
{
    struct GeometryCollideInput;
    struct GeometryCollideOutput;
    struct RflFace;
    struct GeoCache;
    struct BBox;

    struct FaceList
    {
        RflFace *first;
        int count;
    };

    struct RflGeometry
    {
        BBox *bbox;
        char field_4[64];
        int field_44;
        Vector3 aabb_min;
        Vector3 aabb_max;
        float bounding_sphere_radius_not_sure;
        Vector3 bounding_sphere_center_not_sure;
        FaceList faces;
        DynamicArray<> vertices;
        DynamicArray<> field_84_geometryList;
        DynamicArray<> rooms;
        DynamicArray<> normal_rooms;
        DynamicArray<> subrooms;
        DynamicArray<> Portals;
        DynamicArray<> lighting_data;
        DynamicArray<> field_CC_vertices;
        DynamicArray<> field_D8_faces;
        DynamicArray<> field_E4;
        int decals_array[129];
        DynamicArray<> face_scroll_list;
        DynamicArray<> navpoint_ptr_array;
        int field_30C;
        int field_310[16];
        int field_350[3];
        int transform_cache_key;
        DynamicArray<> cached_lights;
        int num_total_lights;
        int field_370;
        int field_374;

        void TestLineCollision(GeometryCollideInput *in, GeometryCollideOutput *out, bool clear_fraction)
        {
            AddrCaller{0x004DF1C0}.this_call(this, in, out, clear_fraction);
        }
    };
    static_assert(sizeof(RflGeometry) == 0x378);

    struct RflRoom
    {
        bool is_subroom;
        bool is_skyroom;
        bool all_faces_invisible;
        char field_3;
        GeoCache *geo_cache;
        Vector3 aabb_min;
        Vector3 aabb_max;
        int room_idx;
        int room_id;
        FaceList face_list;
        DynamicArray<> portals;
        BBox *bbox;
        char field_40;
        bool is_cold;
        bool is_outside;
        bool is_airlock;
        char field_44;
        bool is_ambient_room;
        Color ambient_color;
        char eax_effect[30];
        char field_68;
        char field_69;
        bool has_textures_with_alpha;
        char field_6B;
        DynamicArray<RflRoom*> subrooms;
        struct RflRoom *parent_room_unk;
        Plane field_7C;
        int field_8C;
        int field_90;
        float life;
        bool unkillable;
        char field_99;
        char field_9A;
        char field_9b;
        int decals_array[49];
        char is_room_active_unk;
        char field_161;
        int room_search_depth;
        int field_168;
        float screen_bbox[4];
        int field_17C;
        int liquid_type;
        bool is_liquid_room;
        char field_185;
        char field_186;
        char field_187;
        float liquid_depth;
        Color liquid_color;
        float liquid_visibility;
        int liquid_surface_texture;
        int liquid_waveform_generator_id;
        int liquid_texture_pixels_per_meter_u;
        int liquid_texture_pixels_per_meter_v;
        float liquid_texture_angle_radians;
        int liquid_alpha;
        bool liquid_contains_plankton;
        char field_1AD;
        __int16 field_1AE;
        int liquid_waveform;
        float liquid_surface_texture_scroll_u;
        float liquid_surface_texture_scroll_v;
        DynamicArray<> cached_lights;
        int num_total_lights;
    };
    static_assert(sizeof(RflRoom) == 0x1CC);

    struct GeometryCollideInput
    {
        RflFace *face;
        Vector3 geometry_pos;
        Matrix3 geometry_orient;
        Vector3 start_pos;
        Vector3 len;
        float radius;
        int flags;
        Vector3 start_pos_transformed;
        Vector3 len_transformed;

        GeometryCollideInput()
        {
            AddrCaller{0x004161F0}.this_call(this);
        }
    };
    static_assert(sizeof(GeometryCollideInput) == 0x6C);

    struct GeometryCollideOutput
    {
        int num_hits;
        float fraction;
        Vector3 hit_point;
        Vector3 normal;
        int field_20;
        RflFace *face;

        GeometryCollideOutput()
        {
            AddrCaller{0x00416230}.this_call(this);
        }
    };
    static_assert(sizeof(GeometryCollideOutput) == 0x28);

    struct RflLightmap
    {
        uint8_t *unk;
        int w;
        int h;
        uint8_t *buf;
        int bm_handle;
        int lightmap_idx;
    };
    static_assert(sizeof(RflLightmap) == 0x18);

    static auto& GeomClearCache = AddrAsRef<void()>(0x004F0B90);

    static auto& rfl_static_geometry = AddrAsRef<RflGeometry*>(0x006460E8);
}
