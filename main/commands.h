#pragma once

namespace rf
{
    struct DcCommand;
    struct Player;
}

void CommandsInit();
void CommandsAfterGameInit();
void CommandRegister(rf::DcCommand *pCmd);
rf::Player *FindBestMatchingPlayer(const char *pszName);
void DebugRender3d();
void DebugRender2d();
