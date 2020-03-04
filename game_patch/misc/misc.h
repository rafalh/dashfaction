#pragma once

void MiscInit();
void MiscAfterFullGameInit();
void MiscAfterLevelLoad(const char* level_filename);
void SetPlaySoundEventsVolumeScale(float volume_scale);
void SetJumpToMultiServerList(bool jump);

constexpr int old_obj_limit = 1024;
constexpr int obj_limit = 65536;
