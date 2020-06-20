#include <common/BuildConfig.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <windows.h>
#include <thread>
#include <algorithm>
#include "../rf/debug_console.h"
#include "../rf/network.h"

#if SERVER_WIN32_CONSOLE

bool g_win32_console = false;

static auto& KeyProcessEvent = AddrAsRef<void(int ScanCode, int KeyDown, int DeltaT)>(0x0051E6C0);

void ResetConsoleCursorColumn(bool clear)
{
    HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO scr_buf_info;
    GetConsoleScreenBufferInfo(output_handle, &scr_buf_info);
    if (scr_buf_info.dwCursorPosition.X == 0)
        return;
    COORD NewPos = scr_buf_info.dwCursorPosition;
    NewPos.X = 0;
    SetConsoleCursorPosition(output_handle, NewPos);
    if (clear) {
        for (int i = 0; i < scr_buf_info.dwCursorPosition.X; ++i) {
            WriteConsoleA(output_handle, " ", 1, nullptr, nullptr);
        }
        SetConsoleCursorPosition(output_handle, NewPos);
    }
}

void PrintCmdInputLine()
{
    HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO scr_buf_info;
    GetConsoleScreenBufferInfo(output_handle, &scr_buf_info);
    WriteConsoleA(output_handle, "] ", 2, nullptr, nullptr);
    unsigned Offset = std::max(0, static_cast<int>(rf::dc_cmd_line_len) - scr_buf_info.dwSize.X + 3);
    WriteConsoleA(output_handle, rf::dc_cmd_line + Offset, rf::dc_cmd_line_len - Offset, nullptr, nullptr);
}

BOOL WINAPI ConsoleCtrlHandler([[maybe_unused]] DWORD ctrl_type)
{
    xlog::info("Quiting after Console CTRL");
    static auto& close = AddrAsRef<int32_t>(0x01B0D758);
    close = 1;
    return TRUE;
}

// void InputThreadProc()
// {
//     while (true) {
//         INPUT_RECORD input_record;
//         DWORD num_read = 0;
//         ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &input_record, 1, &num_read);
//     }
// }

CallHook<void()> OsInitWindow_Server_hook{
    0x004B27C5,
    []() {
        AllocConsole();
        SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);
        DWORD mode;
        GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
        SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode & ~ ENABLE_ECHO_INPUT);

        // std::thread InputThread(InputThreadProc);
        // InputThread.detach();
    },
};

FunHook<void(const char*, const int*)> DcPrint_hook{
    reinterpret_cast<uintptr_t>(rf::DcPrint),
    [](const char* text, [[maybe_unused]] const int* color) {
        HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
        constexpr WORD red_attr = FOREGROUND_RED | FOREGROUND_INTENSITY;
        constexpr WORD blue_attr = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        constexpr WORD white_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
        constexpr WORD gray_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
        WORD current_attr = 0;

        ResetConsoleCursorColumn(true);

        const char* ptr = text;
        while (*ptr) {
            std::string color;
            if (ptr[0] == '[' && ptr[1] == '$') {
                const char* color_end_ptr = strchr(ptr + 2, ']');
                if (color_end_ptr) {
                    color.assign(ptr + 2, color_end_ptr - ptr - 2);
                    ptr = color_end_ptr + 1;
                }
            }

            const char* end_ptr = strstr(ptr, "[$");
            if (!end_ptr)
                end_ptr = ptr + strlen(ptr);

            WORD attr;
            if (color == "Red")
                attr = red_attr;
            else if (color == "Blue")
                attr = blue_attr;
            else if (color == "White")
                attr = white_attr;
            else {
                if (!color.empty())
                    xlog::error("unknown color %s", color.c_str());
                attr = gray_attr;
            }

            if (current_attr != attr) {
                current_attr = attr;
                SetConsoleTextAttribute(output_handle, attr);
            }

            DWORD num_chars = end_ptr - ptr;
            WriteFile(output_handle, ptr, num_chars, nullptr, nullptr);
            ptr = end_ptr;
        }

        if (ptr > text && ptr[-1] != '\n')
            WriteFile(output_handle, "\n", 1, nullptr, nullptr);

        if (current_attr != gray_attr)
            SetConsoleTextAttribute(output_handle, gray_attr);

        PrintCmdInputLine();
    },
};

CallHook<void()> DcPutChar_NewLine_hook{
    0x0050A081,
    [] {
        HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
        WriteConsoleA(output_handle, "\r\n", 2, nullptr, nullptr);
        PrintCmdInputLine();
    },
};

FunHook<void()> DcDrawServerConsole_hook{
    0x0050A770,
    []() {
        static char prev_cmd_line[sizeof(rf::dc_cmd_line)];
        if (strncmp(rf::dc_cmd_line, prev_cmd_line, std::size(prev_cmd_line)) != 0) {
            ResetConsoleCursorColumn(true);
            PrintCmdInputLine();
            std::strcpy(prev_cmd_line, rf::dc_cmd_line);
        }
    },
};

FunHook<int()> KeyGetFromQueue_hook{
    0x0051F000,
    []() {
        if (!rf::is_dedicated_server)
            return KeyGetFromQueue_hook.CallTarget();

        HANDLE input_handle = GetStdHandle(STD_INPUT_HANDLE);
        INPUT_RECORD input_record;
        DWORD num_read = 0;
        while (true) {
            if (!PeekConsoleInput(input_handle, &input_record, 1, &num_read) || num_read == 0)
                break;
            if (!ReadConsoleInput(input_handle, &input_record, 1, &num_read) || num_read == 0)
                break;
            if (input_record.EventType == KEY_EVENT) {
                KeyProcessEvent(input_record.Event.KeyEvent.wVirtualScanCode, input_record.Event.KeyEvent.bKeyDown, 0);
            }
        }

        return KeyGetFromQueue_hook.CallTarget();
    },
};

void InitWin32ServerConsole()
{
    g_win32_console = stristr(GetCommandLineA(), "-win32-console") != nullptr;
    if (g_win32_console) {
        OsInitWindow_Server_hook.Install();
        DcPrint_hook.Install();
        DcDrawServerConsole_hook.Install();
        KeyGetFromQueue_hook.Install();
        DcPutChar_NewLine_hook.Install();
    }
}

void CleanupWin32ServerConsole()
{
    if (g_win32_console)
        FreeConsole();
}

#endif // SERVER_WIN32_CONSOLE
