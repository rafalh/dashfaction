#pragma once

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
}
