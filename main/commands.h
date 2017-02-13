#pragma once

namespace rf
{
    struct DcCommand;
}

void CommandsInit();
void CommandsAfterGameInit();
void CommandRegister(rf::DcCommand *pCmd);
