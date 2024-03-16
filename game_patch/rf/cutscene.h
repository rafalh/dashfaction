#pragma once

#include "os/timer.h"
#include "os/timestamp.h"
#include "os/string.h"
#include "math/vector.h"
#include "math/matrix.h"

namespace rf
{
    struct CutscenePath;

    struct CutsceneScript
    {
        int cam_uid;
        float pre_wait_time;
        float path_time;
        float post_wait_time;
        int lookat_uid;
        int trigger_uid;
        String cam_pathname;
    };

    struct Cutscene
    {
        int uid;
        int num_cam_scripts;
        CutsceneScript cam_scripts[64];
        int current_script_index;
        int current_cam_index;
        Timestamp next_stage_timestamp;
        Timestamp start_path_timestamp;
        Timestamp stop_path_timestamp;
        CutsceneScript *current_script;
        CutscenePath *current_path;
        Vector3 current_cam_pos;
        Matrix3 current_cam_orient;
        float path_travel_time;
        bool moving_on_path;
        bool hide_player;
        float fov;
    };
    static_assert(sizeof(Cutscene) == 0x860);

    static auto& active_cutscene = addr_as_ref<Cutscene*>(0x00645320);

    static auto& cutscene_is_playing = addr_as_ref<bool()>(0x0045BE80);
}
