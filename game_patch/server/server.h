#pragma once

#include "../rf.h"

void ServerInit();
void ServerCleanup();
bool CheckServerChatCommand(const char* msg, rf::Player* sender);
