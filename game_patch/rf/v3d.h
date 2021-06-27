#pragma once

#include "math/plane.h"
#include "math/quaternion.h"
#include "gr/gr.h"

namespace rf
{
    struct V3dMesh;
    struct LodMesh;
    struct V3DCSphere;
    struct MaterialClass;

    struct V3d
    {
        char v3d_filename[65];
        int version;
        int num_meshes;
        V3dMesh *meshes;
        int num_lod_meshes;
        LodMesh *lod_meshes;
        int num_navpoints;
        void *navpoints;
        int num_cspheres;
        V3DCSphere *cspheres;
        int total_vertices_count_ccrunch_zeroed;
        int unk_vertices_array_ccrunch_zeroed;
        int total_triangles_count_ccrunch_zeroed;
        int unk_triangle_array_ccrunch_zeroed;
        int num_mesh_materials;
        MaterialClass *mesh_materials;
        int hdr_field_14_ccrunch_zeroed;
        int unk_array_hdr_field_14_ccrunch_zeroed;
        int field_88;
        int flags;
    };
    static_assert(sizeof(V3d) == 0x90);

    struct WeightIndexArray
    {
        ubyte weights[4];
        ubyte indices[4];
    };

    struct VifFace
    {
        ushort vindex1;
        ushort vindex2;
        ushort vindex3;
        ushort flags;
    };

    struct VifChunk
    {
        gr::Mode mode;
        Vector3 *vecs;
        Vector3 *norms;
        Vector2 *uvs;
        Plane *face_planes;
        VifFace *faces;
        short *same_vertex_offsets;
        WeightIndexArray *wi;
        int texture_idx;
        short *morph_vertices_map;
        ushort num_vecs;
        ushort num_faces;
        ushort vecs_alloc;
        ushort faces_alloc;
        ushort uvs_alloc;
        ushort wi_alloc;
        ushort same_vertex_offsets_alloc;
    };

    struct VifPropPoint
    {
        char name[68];
        Quaternion orient;
        Vector3 pos;
        int parent_index;
    };

    struct VifMesh
    {
        int data_block_size;
        void *data_block;
        VifChunk *chunks;
        ushort num_chunks;
        VifPropPoint *prop_points;
        int num_prop_points;
        ubyte tex_ids[7];
        int tex_handles[7];
        int num_textures_handles;
        int flags;
        int num_vertices;
        int unk_field_from_v3d_file;
#ifdef DASH_FACTION
        void *render_cache;
#endif
    };

    constexpr int VIF_FACE_DOUBLE_SIDED = 0x20;

    struct VifLodMesh
    {
        int num_levels;
        VifMesh *meshes[3];
        float distances[3];
        Vector3 center;
        float radius;
        Vector3 bbox_min;
        Vector3 bbox_max;
#ifdef DASH_FACTION
        void *render_cache;
#endif
    };

    struct MeshRenderParams
    {
        int flags;
        int lod_level;
        int alpha;
        gr::Color self_illum; // unused
        int *alt_tex;
        float field_14_unk_fire;
        gr::Color field_18;
        ubyte *vertex_colors;
        int powerup_bitmaps[2];
        gr::Color ambient_color;
        Matrix3 orient;
    };
}
