#pragma once

// Forward declarations
namespace rf
{
    struct Player;
}

void SpectateModeSetTargetPlayer(rf::Player* player);
void SpectateModeInit();
void SpectateModeAfterFullGameInit();
void SpectateModeDrawUI();
void SpectateModeOnDestroyPlayer(rf::Player* player);
bool SpectateModeIsActive();
