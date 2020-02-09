#pragma once

// Forward declarations
namespace rf
{
    class Player;
}

void ServerInit();
void ServerCleanup();
void ServerDoFrame();
bool CheckServerChatCommand(const char* msg, rf::Player* sender);
void ServerOnLimboStateEnter();
