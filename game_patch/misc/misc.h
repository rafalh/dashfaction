#pragma once

void MiscInit();
void MiscAfterFullGameInit();
void MiscAfterLevelLoad(const char* level_filename);
void SetPlaySoundEventsVolumeScale(float volume_scale);
void SetJumpToMultiServerList(bool jump);
void ClearTriggersForLateJoiners();

constexpr size_t old_obj_limit = 1024;
constexpr size_t obj_limit = 65536;
