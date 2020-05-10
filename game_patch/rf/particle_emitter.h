#pragma once

#include "common.h"

namespace rf
{

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

class ParticleEmitter {};

}
