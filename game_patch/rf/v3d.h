#pragma once

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
}
