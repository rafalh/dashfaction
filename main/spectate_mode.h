#pragma once

#include "rf.h"

void SpectateModeSetTargetPlayer(rf::Player *pPlayer);
void SpectateModeInit();
void SpectateModeAfterFullGameInit();
void SpectateModeDrawUI();
void SpectateModeOnDestroyPlayer(rf::Player *pPlayer);
bool SpectateModeIsActive();
