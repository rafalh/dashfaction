#include "console.h"
#include "../main.h"
#include "../rf/player.h"
#include "../rf/game_seq.h"
#include <common/BuildConfig.h>
#include <common/version.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/FunHook.h>
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

FunHook<int()> MenuUpdate_hook{
    0x00434230,
    []() {
        int menu_id = MenuUpdate_hook.CallTarget();
        if (menu_id == rf::GS_MP_LIMBO) // hide cursor when changing level - hackfixed in RF by changing rendering logic
            rf::SetCursorVisible(false);
        else if (menu_id == rf::GS_MAIN_MENU)
            rf::SetCursorVisible(true);
        return menu_id;
    },
};

CodeInjection DcCommand_Init_limit_check_patch{
    0x00509A7E,
    [](auto& regs) {
        if (regs.eax >= CMD_LIMIT) {
            regs.eip = 0x00509ACD;
        }
    },
};

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
    // Console init string
    WriteMemPtr(0x004B2534, "-- " PRODUCT_NAME " Initializing --\n");

    // Console background color
    WriteMem<u32>(0x005098D1, CONSOLE_BG_A); // Alpha
    WriteMem<u8>(0x005098D6, CONSOLE_BG_B);  // Blue
    WriteMem<u8>(0x005098D8, CONSOLE_BG_G);  // Green
    WriteMem<u8>(0x005098DA, CONSOLE_BG_R);  // Red

    // Fix console rendering when changing level
    AsmWriter(0x0047C490).ret();
    AsmWriter(0x0047C4AA).ret();
    AsmWriter(0x004B2E15).nop(2);
    MenuUpdate_hook.Install();

    // Change limit of commands
    ASSERT(rf::dc_num_commands == 0);
    WriteMemPtr(0x005099AC + 1, g_commands_buffer);
    WriteMemPtr(0x00509A8A + 1, g_commands_buffer);
    WriteMemPtr(0x00509AB0 + 3, g_commands_buffer);
    WriteMemPtr(0x00509AE1 + 3, g_commands_buffer);
    WriteMemPtr(0x00509AF5 + 3, g_commands_buffer);
    WriteMemPtr(0x00509C8F + 1, g_commands_buffer);
    WriteMemPtr(0x00509DB4 + 3, g_commands_buffer);
    WriteMemPtr(0x00509E6F + 1, g_commands_buffer);
    WriteMemPtr(0x0050A648 + 4, g_commands_buffer);
    WriteMemPtr(0x0050A6A0 + 3, g_commands_buffer);
    AsmWriter(0x00509A7E).nop(2);
    DcCommand_Init_limit_check_patch.Install();

    DcRunCmd_CallHandlerPatch.Install();

    ConsoleCommandsApplyPatches();
    ConsoleAutoCompleteApplyPatch();
}

void ConsoleInit()
{
    ConsoleCommandsInit();
}
