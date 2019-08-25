#pragma once

#include "../rf.h"

void ServerInit();
void ServerCleanup();
void ServerDoFrame();
bool CheckServerChatCommand(const char* msg, rf::Player* sender);
void ServerOnLimboStateEnter();
