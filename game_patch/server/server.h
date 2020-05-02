#pragma once

// Forward declarations
namespace rf
{
    struct Player;
}

void ServerInit();
void ServerCleanup();
void ServerDoFrame();
bool CheckServerChatCommand(const char* msg, rf::Player* sender);
void ServerOnLimboStateEnter();
bool ServerIsSavingEnabled();
