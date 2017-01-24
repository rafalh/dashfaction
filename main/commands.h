#pragma once

namespace rf
{
    struct CCmd;
}

void CommandsInit();
void CommandsAfterGameInit();
void CommandRegister(rf::CCmd *pCmd);
