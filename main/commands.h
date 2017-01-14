#pragma once

struct CCmd;

void CommandsInit();
void CommandsAfterGameInit();
void CommandRegister(CCmd *pCmd);
