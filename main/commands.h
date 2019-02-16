#pragma once

namespace rf
{
    struct DcCommand;
    struct CPlayer;
}

void CommandsInit();
void CommandsAfterGameInit();
void CommandRegister(rf::DcCommand *pCmd);
rf::CPlayer *FindBestMatchingPlayer(const char *pszName);
void DebugRender3d();
void DebugRender2d();
