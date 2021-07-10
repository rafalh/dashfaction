#include <patch_common/AsmWriter.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/FunHook.h>
#include "../../rf/gr/gr.h"
#include "../../rf/os/os.h"
#include "../../rf/v3d.h"
#include "../../bmpman/bmpman.h"
#include "../../main/main.h"
#include "gr_d3d11.h"

namespace df::gr::d3d11
{
    static std::optional<Renderer> renderer;

    void msg_handler(UINT msg, WPARAM w_param, LPARAM l_param)
    {
        switch (msg) {
        case WM_ACTIVATEAPP:
            if (w_param) {
                xlog::trace("active %x %lx", w_param, l_param);
                renderer->window_active();
            }
            else {
                xlog::trace("inactive %x %lx", w_param, l_param);
                renderer->window_inactive();
            }
        }
    }

    void flip()
    {
        renderer->flip();
    }

    void close()
    {
        xlog::info("Cleaning up D3D11");
        renderer.reset();
    }

    void init(HWND hwnd)
    {
        xlog::info("Initializing D3D11");
        renderer.emplace(hwnd);
        rf::os_add_msg_handler(msg_handler);
    }

    rf::bm::Format read_back_buffer(int x, int y, int w, int h, rf::ubyte *data)
    {
        return renderer->read_back_buffer(x, y, w, h, data);
    }

    void page_in(int bm_handle)
    {
        renderer->page_in(bm_handle);
    }

    void clear()
    {
        renderer->clear();
    }

    void bitmap(int bitmap_handle, int x, int y, int w, int h, int sx, int sy, int sw, int sh, bool flip_x, bool flip_y, rf::gr::Mode mode)
    {
        renderer->bitmap(bitmap_handle, x, y, w, h, sx, sy, sw, sh, flip_x, flip_y, mode);
    }

    void set_clip()
    {
        renderer->set_clip();
    }

    void zbuffer_clear()
    {
        renderer->zbuffer_clear();
    }

    void tmapper(int nv, const rf::gr::Vertex **vertices, int vertex_attributes, rf::gr::Mode mode)
    {
        renderer->tmapper(nv, vertices, vertex_attributes, mode);
    }

    void line(float x1, float y1, float x2, float y2, rf::gr::Mode mode)
    {
        renderer->line(x1, y1, x2, y2, mode);
    }

    void line_3d(const rf::gr::Vertex& v0, const rf::gr::Vertex& v1, rf::gr::Mode mode)
    {
        renderer->line(v0, v1, mode);
    }

    void texture_save_cache()
    {
        renderer->texture_save_cache();
    }

    void texture_flush_cache(bool force)
    {
        renderer->texture_flush_cache(force);
    }

    void texture_mark_dirty(int bm_handle)
    {
        renderer->texture_mark_dirty(bm_handle);
    }

    bool lock(int bm_handle, int section, rf::gr::LockInfo *lock)
    {
        return renderer->lock(bm_handle, section, lock);
    }

    void unlock(rf::gr::LockInfo *lock)
    {
        renderer->unlock(lock);
    }

    void get_texel(int bm_handle, float u, float v, rf::gr::Color *clr)
    {
        renderer->get_texel(bm_handle, u, v, clr);
    }

    void texture_add_ref(int bm_handle)
    {
        renderer->texture_add_ref(bm_handle);
    }

    void texture_remove_ref(int bm_handle)
    {
        renderer->texture_remove_ref(bm_handle);
    }

    void render_solid(rf::GSolid* solid, rf::GRoom** rooms, int num_rooms)
    {
        renderer->render_solid(solid, rooms, num_rooms);
    }

    void render_movable_solid(rf::GSolid* solid, const rf::Vector3& pos, const rf::Matrix3& orient)
    {
        renderer->render_movable_solid(solid, pos, orient);
    }

    void render_alpha_detail_room(rf::GRoom *room, rf::GSolid *solid)
    {
        if (renderer) {
            renderer->render_alpha_detail_room(room, solid);
        }
    }

    void render_sky_room(rf::GRoom *room)
    {
        renderer->render_sky_room(room);
    }

    void render_v3d_vif(rf::VifLodMesh *lod_mesh, rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, [[maybe_unused]] int lod_index, const rf::MeshRenderParams& params)
    {
        renderer->render_v3d_vif(lod_mesh, mesh, pos, orient, params);
    }

    void render_character_vif(rf::VifLodMesh *lod_mesh, rf::VifMesh *mesh, const rf::Vector3& pos, const rf::Matrix3& orient, const rf::CharacterInstance *ci, [[maybe_unused]] int lod_index, const rf::MeshRenderParams& params)
    {
        renderer->render_character_vif(lod_mesh, mesh, pos, orient, ci, params);
    }

    void fog_set()
    {
        renderer->fog_set();
    }

    bool set_render_target(int bm_handle)
    {
        return renderer->set_render_target(bm_handle);
    }

    void clear_solid_render_cache()
    {
        if (renderer) {
            renderer->clear_solid_cache();
        }
    }

    void delete_texture(int bm_handle)
    {
        renderer->texture_mark_dirty(bm_handle);
    }

    void update_window_mode()
    {
        renderer->set_fullscreen_state(rf::gr::screen.window_mode == rf::gr::FULLSCREEN);
    }

    static CodeInjection g_render_room_objects_render_liquid_injection{
        0x004D4106,
        [](auto& regs) {
            rf::GRoom* room = regs.edi;
            rf::GSolid* solid = regs.ebx;
            renderer->render_room_liquid_surface(solid, room);
            regs.eip = 0x004D414F;
        },
    };

    static CodeInjection gr_d3d_setup_3d_injection{
        0x005473E4,
        []() {
            renderer->setup_3d();
        },
    };

    static CodeInjection vif_lod_mesh_ctor_injection{
        0x00569D00,
        [](auto& regs) {
            rf::VifLodMesh* lod_mesh = regs.ecx;
            lod_mesh->render_cache = nullptr;
        },
    };

    static CodeInjection vif_lod_mesh_dtor_injection{
        0x005695D0,
        [](auto& regs) {
            rf::VifLodMesh* lod_mesh = regs.ecx;
            if (renderer) {
                renderer->clear_vif_cache(lod_mesh);
            }
        },
    };
}

void gr_d3d11_apply_patch()
{
    using namespace df::gr::d3d11;

    g_render_room_objects_render_liquid_injection.install();
    gr_d3d_setup_3d_injection.install();
    vif_lod_mesh_ctor_injection.install();
    vif_lod_mesh_dtor_injection.install();

    AsmWriter{0x004F0B90}.jmp(clear_solid_render_cache); // g_render_cache_clear
    AsmWriter{0x004F0B20}.ret(); // g_render_cache_init

    // AsmWriter{0x00545960}.jmp(init);

    // write_mem<ubyte>(0x0050CBE0 + 6, GR_DIRECT3D11);
    // AsmWriter{0x0050CBE9}.call(close);

    // write_mem<ubyte>(0x0050CE2A + 6, GR_DIRECT3D11);
    // AsmWriter{0x0050CE33}.jmp(flip);

    // write_mem<ubyte>(0x0050DF80 + 6, GR_DIRECT3D11);
    // AsmWriter{0x0050DF9D}.call(tmapper);

    using namespace asm_regs;
    AsmWriter{0x00544FC0}.jmp(flip); // gr_d3d_flip
    AsmWriter{0x00545230}.jmp(close); // gr_d3d_close
    AsmWriter{0x00545960}.jmp(init); // gr_d3d_init
    AsmWriter{0x00546730}.jmp(read_back_buffer); // gr_d3d_read_backbuffer
    AsmWriter{0x005468C0}.jmp(fog_set); // gr_d3d_fog_set
    AsmWriter{0x00546A00}.mov(al, 1).ret(); // gr_d3d_is_mode_supported
    //AsmWriter{0x00546A40}.ret(); // gr_d3d_setup_frustum
    //AsmWriter{0x00546F60}.ret(); // gr_d3d_change_frustum
    //AsmWriter{0x00547150}.ret(); // gr_d3d_setup_3d
    //AsmWriter{0x005473F0}.ret(); // gr_d3d_start_instance
    //AsmWriter{0x00547540}.ret(); // gr_d3d_stop_instance
    //AsmWriter{0x005477A0}.ret(); // gr_d3d_project_vertex
    //AsmWriter{0x005478F0}.ret(); // gr_d3d_is_normal_facing
    //AsmWriter{0x00547960}.ret(); // gr_d3d_is_normal_facing_plane
    //AsmWriter{0x005479B0}.ret(); // gr_d3d_get_apparent_distance_from_camera
    //AsmWriter{0x005479D0}.ret(); // gr_d3d_screen_coords_from_world_coords
    AsmWriter{0x00547A60}.ret(); // gr_d3d_update_gamma_ramp
    AsmWriter{0x00547AC0}.ret(); // gr_d3d_set_texture_mip_filter
    AsmWriter{0x00550820}.jmp(page_in); // gr_d3d_page_in
    AsmWriter{0x005508C0}.jmp(clear); // gr_d3d_clear
    AsmWriter{0x00550980}.jmp(zbuffer_clear); // gr_d3d_zbuffer_clear
    AsmWriter{0x00550A30}.jmp(set_clip); // gr_d3d_set_clip
    AsmWriter{0x00550AA0}.jmp(bitmap); // gr_d3d_bitmap
    AsmWriter{0x00551450}.ret(); // gr_d3d_flush_after_color_change
    AsmWriter{0x00551460}.jmp(line); // gr_d3d_line
    AsmWriter{0x00551900}.jmp(tmapper); // gr_d3d_tmapper
    AsmWriter{0x005536C0}.jmp(render_sky_room);
    AsmWriter{0x00553C60}.jmp(render_movable_solid); // gr_d3d_render_movable_solid - uses gr_d3d_render_face_list
    //AsmWriter{0x00553EE0}.ret(); // gr_d3d_vfx - uses gr_poly
    //AsmWriter{0x00554BF0}.ret(); // gr_d3d_vfx_facing - uses gr_d3d_3d_bitmap_angle, gr_d3d_render_volumetric_light
    //AsmWriter{0x00555080}.ret(); // gr_d3d_vfx_glow - uses gr_d3d_3d_bitmap_angle
    //AsmWriter{0x00555100}.ret(); // gr_d3d_line_vertex
    AsmWriter{0x005516E0}.jmp(line_3d); // gr_d3d_line_vertex_internal
    //AsmWriter{0x005551E0}.ret(); // gr_d3d_line_vec - uses gr_d3d_line_vertex
    //AsmWriter{0x00555790}.ret(); // gr_d3d_3d_bitmap - uses gr_poly
    //AsmWriter{0x00555AC0}.ret(); // gr_d3d_3d_bitmap_angle - uses gr_poly
    //AsmWriter{0x00555B20}.ret(); // gr_d3d_3d_bitmap_angle_wh - uses gr_poly
    //AsmWriter{0x00555B80}.ret(); // gr_d3d_render_volumetric_light - uses gr_poly
    //AsmWriter{0x00555DC0}.ret(); // gr_d3d_laser - uses gr_tmapper
    //AsmWriter{0x005563F0}.ret(); // gr_d3d_cylinder - uses gr_line
    //AsmWriter{0x005565D0}.ret(); // gr_d3d_cone - uses gr_line
    //AsmWriter{0x005566E0}.ret(); // gr_d3d_sphere - uses gr_line
    //AsmWriter{0x00556AB0}.ret(); // gr_d3d_chain - uses gr_poly
    //AsmWriter{0x00556F50}.ret(); // gr_d3d_line_directed - uses gr_line_vertex
    //AsmWriter{0x005571F0}.ret(); // gr_d3d_line_arrow - uses gr_line_vertex
    //AsmWriter{0x00557460}.ret(); // gr_d3d_render_particle_sys_particle - uses gr_poly, gr_3d_bitmap_angle
    //AsmWriter{0x00557D40}.ret(); // gr_d3d_render_bolts - uses gr_poly, gr_line
    //AsmWriter{0x00558320}.ret(); // gr_d3d_render_geomod_debris - uses gr_poly
    //AsmWriter{0x00558450}.ret(); // gr_d3d_render_glass_shard - uses gr_poly
    AsmWriter{0x00558550}.ret(); // gr_d3d_render_face_wireframe
    //AsmWriter{0x005585F0}.ret(); // gr_d3d_render_weapon_tracer - uses gr_poly
    //AsmWriter{0x005587C0}.ret(); // gr_d3d_poly - uses gr_d3d_tmapper
    AsmWriter{0x00558920}.ret(); // gr_d3d_render_geometry_wireframe
    AsmWriter{0x00558960}.ret(); // gr_d3d_render_geometry_in_editor
    AsmWriter{0x00558C40}.ret(); // gr_d3d_render_sel_face_in_editor
    //AsmWriter{0x00558D40}.ret(); // gr_d3d_world_poly - uses gr_d3d_poly
    //AsmWriter{0x00558E30}.ret(); // gr_d3d_3d_bitmap_stretched_square - uses gr_d3d_world_poly
    //AsmWriter{0x005590F0}.ret(); // gr_d3d_rod - uses gr_d3d_world_poly
    AsmWriter{0x005596C0}.ret(); // gr_d3d_render_face_list_colored
    AsmWriter{0x0055B520}.jmp(texture_save_cache); // gr_d3d_texture_save_cache
    AsmWriter{0x0055B550}.jmp(texture_flush_cache); // gr_d3d_texture_flush_cache
    AsmWriter{0x0055CDC0}.jmp(texture_mark_dirty); // gr_d3d_texture_mark_dirty
    AsmWriter{0x0055CE00}.jmp(lock); // gr_d3d_lock
    AsmWriter{0x0055CF60}.jmp(unlock); // gr_d3d_unlock
    AsmWriter{0x0055CFA0}.jmp(get_texel); // gr_d3d_get_texel
    AsmWriter{0x0055D160}.jmp(texture_add_ref); // gr_d3d_texture_add_ref
    AsmWriter{0x0055D190}.jmp(texture_remove_ref); // gr_d3d_texture_remove_ref
    AsmWriter{0x0055F5E0}.jmp(render_solid); // gr_d3d_render_static_solid
    AsmWriter{0x00561650}.ret(); // gr_d3d_render_face_list
    //AsmWriter{0x0052FA40}.jmp(render_lod_vif); // gr_d3d_render_vif_mesh
    AsmWriter{0x0052DE10}.jmp(render_v3d_vif); // gr_d3d_render_v3d_vif
    AsmWriter{0x0052E9E0}.jmp(render_character_vif); // gr_d3d_render_character_vif
    AsmWriter{0x004D34D0}.jmp(render_alpha_detail_room); // room_render_alpha_detail

    // Change size of standard structures
    write_mem<int8_t>(0x00569884 + 1, sizeof(rf::VifMesh));
    write_mem<int8_t>(0x00569732 + 1, sizeof(rf::VifLodMesh));
}
