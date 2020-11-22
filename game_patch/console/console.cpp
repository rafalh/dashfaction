#include "console.h"
#include "../main.h"
#include "../rf/player.h"
#include "../rf/gameseq.h"
#include "../rf/input.h"
#include <common/BuildConfig.h>
#include <common/version.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/ShortTypes.h>
#include <algorithm>
#include <cassert>

// ConsoleDrawClientConsole uses 200 bytes long buffer for: "] ", user input and '\0'
constexpr int max_cmd_line_len = 200 - 2 - 1;

rf::ConsoleCommand* g_commands_buffer[CMD_LIMIT];

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
        rf::ConsolePrintf("Found %d players matching '%s'!", num_found, name);
    else
        rf::ConsolePrintf("Cannot find player matching '%s'", name);
    return nullptr;
}

FunHook<int()> GameseqProcess_hook{
    0x00434230,
    []() {
        int menu_id = GameseqProcess_hook.CallTarget();
        if (menu_id == rf::GS_MULTI_LIMBO) // hide cursor when changing level - hackfixed in RF by changing rendering logic
            rf::MouseSetVisible(false);
        else if (menu_id == rf::GS_MAIN_MENU)
            rf::MouseSetVisible(true);
        return menu_id;
    },
};

CodeInjection ConsoleCommand_Init_limit_check_patch{
    0x00509A7E,
    [](auto& regs) {
        if (regs.eax >= CMD_LIMIT) {
            regs.eip = 0x00509ACD;
        }
    },
};

CodeInjection ConsoleRunCmd_CallHandlerPatch{
    0x00509DBB,
    [](auto& regs) {
        // Make sure command pointer is in ecx register to support thiscall handlers
        regs.ecx = regs.eax;
    },
};

CallHook<void(char*, int)> ConsoleProcessKbd_GetTextFromClipboard_hook{
    0x0050A2FD,
    [](char *buf, int max_len) {
        max_len = std::min(max_len, max_cmd_line_len - rf::console_cmd_line_len);
        ConsoleProcessKbd_GetTextFromClipboard_hook.CallTarget(buf, max_len);
    },
};

void ConsoleRegisterCommand(rf::ConsoleCommand* cmd)
{
    if (rf::console_num_commands < CMD_LIMIT)
        rf::ConsoleCommand::Init(cmd, cmd->name, cmd->help, cmd->func);
    else
        assert(false);
}

void ConsoleCommandsApplyPatches();
void ConsoleAutoCompleteApplyPatch();
void ConsoleCommandsInit();

void ConsoleApplyPatches()
{
    // Console init string
    WriteMemPtr(0x004B2534, "-- " PRODUCT_NAME " Initializing --\n");

    // Console background color
    constexpr rf::Color console_color{0x00, 0x00, 0x40, 0xC0};
    WriteMem<u32>(0x005098D1, console_color.alpha);
    WriteMem<u8>(0x005098D6, console_color.blue);
    WriteMem<u8>(0x005098D8, console_color.green);
    WriteMem<u8>(0x005098DA, console_color.red);

    // Fix console rendering when changing level
    AsmWriter(0x0047C490).ret();
    AsmWriter(0x0047C4AA).ret();
    AsmWriter(0x004B2E15).nop(2);
    GameseqProcess_hook.Install();

    // Change limit of commands
    assert(rf::console_num_commands == 0);
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
    ConsoleCommand_Init_limit_check_patch.Install();

    ConsoleRunCmd_CallHandlerPatch.Install();

    // Fix possible input buffer overflow
    ConsoleProcessKbd_GetTextFromClipboard_hook.Install();
    WriteMem<u32>(0x0050A2D0 + 2, max_cmd_line_len);

    ConsoleCommandsApplyPatches();
    ConsoleAutoCompleteApplyPatch();
}

void ConsoleInit()
{
    ConsoleCommandsInit();
}
