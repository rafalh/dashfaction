#pragma once

#include "rf.h"

void SpectateModeSetTargetPlayer(rf::CPlayer *pPlayer);
void SpectateModeInit();
void SpectateModeAfterFullGameInit();
void SpectateModeDrawUI();
void SpectateModeOnDestroyPlayer(rf::CPlayer *pPlayer);
bool SpectateModeIsActive();
