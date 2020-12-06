#pragma once

#include "common.h"

namespace rf
{

struct GRoom;
struct ParticleEmitter;

enum ParticleEmitterFlags
{
    PEF_IMMEDIATE = 0x2,
    PEF_CONTINOUS = 0x4,
    PEF_DIRDEPEND = 0x8,
    PEF_INITIALLY_ON = 0x10,
    PEF_ALTERNATE_STATES = 0x20,
    PEF_DONT_MOVE_WITH_PARENT = 0x40,
    PEF_ACCEL_WITH_PARENT = 0x80,
};

struct ParticleEmitterType
{
    int uid;
    Vector3 pos;
    Vector3 dir;
    float dir_rand;
    float min_vel;
    float max_vel;
    float min_spawn_delay;
    float max_spawn_delay;
    float spawn_radius;
    int16_t flags;
    float min_life_secs;
    float max_life_secs;
    float min_pradius;
    float max_pradius;
    float growth_rate;
    float acceleration;
    float gravity_scale;
    float alt_stateon_time;
    float alt_stateon_time_variance;
    float alt_stateoff_time;
    float alt_stateoff_time_variance;
    int bitmap;
    int num_frames;
    Color particle_clr;
    Color particle_clr_dest;
    int particle_flags;
    int particle_flags2;
    float age_pct_to_finish_vbm;
    float active_distance;
};
static_assert(sizeof(ParticleEmitterType) == 0x84);

struct ParticleCreateInfo
{
    Vector3 pos;
    Vector3 vel;
    float radius;
    float growth_rate;
    float acceleration;
    float gravity_scale;
    float lifetime_seconds;
    int bitmap_handle;
    int num_frames;
    Color clr;
    Color clr_dest;
    int flags;
    int flags2;
    int age_pct_to_finish_vbm;
    int hit_callback;
};
static_assert(sizeof(ParticleCreateInfo) == 0x4C);

struct Particle
{
    Particle *next;
    Particle *prev;
    int parent_hobj;
    Vector3 pos;
    Vector3 velocity;
    float current_life_time;
    Color particle_clr;
    Color particle_clr_dest;
    Color particle_clr2;
    float life_secs;
    float p_radius;
    float growth_rate;
    float acceleration;
    float gravity;
    int bitmap;
    short word_4C;
    short particle_flags2;
    unsigned char pool_id;
    float unk_orient;
    int particle_flags;
    int field_5C;
    void (*unk_callback)(Vector3 *hit_pos, const Vector3 *vel, Vector3 *normal, float *, int *is_liquid, int *hit_obj_uid);
    GRoom *room;
    ParticleEmitter *emitter;
    Vector3 prev_pos;
};
static_assert(sizeof(Particle) == 0x78);

struct ParticleEmitter
{
    int uid;
    int parent_handle;
    Vector3 pos;
    Vector3 dir;
    float dir_rand;
    float min_vel;
    float max_vel;
    float spawn_radius;
    float min_spawn_delay;
    float max_spawn_delay;
    int emitter_flags;
    float min_life_secs;
    float max_life_secs;
    float min_pradius;
    float max_pradius;
    GRoom *room;
    ParticleCreateInfo pci;
    float cull_radius;
    float field_A0;
    Vector3 field_A4;
    Particle particle_list;
    float on_time;
    float on_time_variance;
    float off_time;
    float off_time_variance;
    float time_to_change;
    float current_state_time;
    bool active;
    int field_144;
    ParticleEmitter *next;
    ParticleEmitter *prev;
    ParticleEmitter *next_entity_emitter;
    Timestamp spawn_timer;
};
static_assert(sizeof(ParticleEmitter) == 0x158);

}
