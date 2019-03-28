#pragma once

#include "rf.h"

void SpectateModeSetTargetPlayer(rf::Player *Player);
void SpectateModeInit();
void SpectateModeAfterFullGameInit();
void SpectateModeDrawUI();
void SpectateModeOnDestroyPlayer(rf::Player *Player);
bool SpectateModeIsActive();
