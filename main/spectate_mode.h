#pragma once

#include "rf.h"

void SpectateModeSetTargetPlayer(rf::CPlayer *pPlayer);
void SpectateModeInit();
void SpectateModeDrawUI();
void SpectateModeOnDestroyPlayer(rf::CPlayer *pPlayer);
bool SpectateModeIsActive();
