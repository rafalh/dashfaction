#pragma once

#include "rf.h"

void SpectateModeSetTargetPlayer(rf::Player* player);
void SpectateModeInit();
void SpectateModeAfterFullGameInit();
void SpectateModeDrawUI();
void SpectateModeOnDestroyPlayer(rf::Player* player);
bool SpectateModeIsActive();
