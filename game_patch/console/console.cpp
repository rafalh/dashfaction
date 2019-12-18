#include "console.h"
#include "../main.h"
#include "../rf.h"
#include <common/BuildConfig.h>
#include <patch_common/CodeInjection.h>
#include <algorithm>

rf::DcCommand* g_commands_buffer[CMD_LIMIT];

rf::Player* FindBestMatchingPlayer(const char* name)
{
    rf::Player* found_player;
    int num_found = 0;
    FindPlayer(StringMatcher().Exact(name), [&](rf::Player* player) {
        found_player = player;
        ++num_found;
    });
    if (num_found == 1)
        return found_player;

    num_found = 0;
    FindPlayer(StringMatcher().Infix(name), [&](rf::Player* player) {
        found_player = player;
        ++num_found;
    });

    if (num_found == 1)
        return found_player;
    else if (num_found > 1)
        rf::DcPrintf("Found %d players matching '%s'!", num_found, name);
    else
        rf::DcPrintf("Cannot find player matching '%s'", name);
    return nullptr;
}

CodeInjection DcRunCmd_CallHandlerPatch{
    0x00509DBB,
    [](auto& regs) {
        // Make sure command pointer is in ecx register to support thiscall handlers
        regs.ecx = regs.eax;
    },
};

void ConsoleRegisterCommand(rf::DcCommand* cmd)
{
    if (rf::dc_num_commands < CMD_LIMIT)
        rf::DcCommand::Init(cmd, cmd->cmd_name, cmd->descr, cmd->func);
    else
        ASSERT(false);
}

void ConsoleCommandsApplyPatches();
void ConsoleAutoCompleteApplyPatch();
void ConsoleCommandsInit();

void ConsoleApplyPatches()
{
    // Change limit of commands
    ASSERT(rf::dc_num_commands == 0);
    WriteMemPtr(0x005099AC + 1, g_commands_buffer);
    WriteMem<u8>(0x00509A78 + 2, CMD_LIMIT);
    WriteMemPtr(0x00509A8A + 1, g_commands_buffer);
    WriteMemPtr(0x00509AB0 + 3, g_commands_buffer);
    WriteMemPtr(0x00509AE1 + 3, g_commands_buffer);
    WriteMemPtr(0x00509AF5 + 3, g_commands_buffer);
    WriteMemPtr(0x00509C8F + 1, g_commands_buffer);
    WriteMemPtr(0x00509DB4 + 3, g_commands_buffer);
    WriteMemPtr(0x00509E6F + 1, g_commands_buffer);
    WriteMemPtr(0x0050A648 + 4, g_commands_buffer);
    WriteMemPtr(0x0050A6A0 + 3, g_commands_buffer);

    DcRunCmd_CallHandlerPatch.Install();

    ConsoleCommandsApplyPatches();
    ConsoleAutoCompleteApplyPatch();
}

void ConsoleInit()
{
    ConsoleCommandsInit();
}
