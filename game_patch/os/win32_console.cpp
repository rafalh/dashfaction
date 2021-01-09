#include <common/config/BuildConfig.h>
#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmWriter.h>
#include <xlog/xlog.h>
#include <windows.h>
#include <thread>
#include <algorithm>
#include <cstring>
#include "../rf/console.h"
#include "../rf/multi.h"
#include "../rf/input.h"

static bool win32_console_enabled = false;

bool win32_console_is_enabled()
{
    return rf::is_dedicated_server && win32_console_enabled;
}

static void reset_console_cursor_column(bool clear)
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

static void print_cmd_input_line()
{
    HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    CONSOLE_SCREEN_BUFFER_INFO scr_buf_info;
    GetConsoleScreenBufferInfo(output_handle, &scr_buf_info);
    WriteConsoleA(output_handle, "] ", 2, nullptr, nullptr);
    unsigned Offset = std::max(0, static_cast<int>(rf::console_cmd_line_len) - scr_buf_info.dwSize.X + 3);
    WriteConsoleA(output_handle, rf::console_cmd_line + Offset, rf::console_cmd_line_len - Offset, nullptr, nullptr);
}

static BOOL WINAPI console_ctrl_handler([[maybe_unused]] DWORD ctrl_type)
{
    xlog::info("Quiting after Console CTRL");
    static auto& close = addr_as_ref<int32_t>(0x01B0D758);
    close = 1;
    return TRUE;
}

// void input_thread_proc()
// {
//     while (true) {
//         INPUT_RECORD input_record;
//         DWORD num_read = 0;
//         ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &input_record, 1, &num_read);
//     }
// }

void win32_console_init()
{
    win32_console_enabled = string_contains_ignore_case(GetCommandLineA(), "-win32-console");
    if (!win32_console_enabled) {
        return;
    }

    AllocConsole();
    SetConsoleCtrlHandler(console_ctrl_handler, TRUE);
    DWORD mode;
    GetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), &mode);
    SetConsoleMode(GetStdHandle(STD_INPUT_HANDLE), mode & ~ ENABLE_ECHO_INPUT);

    // std::thread input_thread(input_thread_proc);
    // input_thread.detach();
}

void win32_console_output(const char* text, [[maybe_unused]] const rf::Color* color)
{
    HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    constexpr WORD red_attr = FOREGROUND_RED | FOREGROUND_INTENSITY;
    constexpr WORD blue_attr = FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    constexpr WORD white_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_INTENSITY;
    constexpr WORD gray_attr = FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE;
    WORD current_attr = 0;

    reset_console_cursor_column(true);

    constexpr std::string_view color_prefix{"[$"};
    constexpr std::string_view color_suffix{"]"};
    std::string_view text_sv{text};
    size_t pos = 0;
    while (pos < text_sv.size()) {
        std::string color;
        if (text_sv.substr(pos, color_prefix.size()) == color_prefix) {
            size_t color_name_pos = pos + color_prefix.size();
            size_t color_suffix_pos = text_sv.find(color_suffix, color_name_pos);
            if (color_suffix_pos != std::string_view::npos) {
                color = text_sv.substr(color_name_pos, color_suffix_pos - color_name_pos);
                pos = color_suffix_pos + color_suffix.size();
            }
        }
        size_t end_pos = text_sv.find(color_prefix, pos);
        if (end_pos == std::string_view::npos) {
            end_pos = text_sv.size();
        }
        std::string_view text_part = text_sv.substr(pos, end_pos - pos);

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

        WriteFile(output_handle, text_part.data(), text_part.size(), nullptr, nullptr);
        pos = end_pos;
    }

    if (pos > 0 && text_sv[-1] != '\n')
        WriteFile(output_handle, "\n", 1, nullptr, nullptr);

    if (current_attr != gray_attr)
        SetConsoleTextAttribute(output_handle, gray_attr);

    print_cmd_input_line();
}

void win32_console_new_line()
{
    HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
    WriteConsoleA(output_handle, "\r\n", 2, nullptr, nullptr);
    print_cmd_input_line();
}

void win32_console_update()
{
    static char prev_cmd_line[sizeof(rf::console_cmd_line)];
    if (std::strncmp(rf::console_cmd_line, prev_cmd_line, std::size(prev_cmd_line)) != 0) {
        reset_console_cursor_column(true);
        print_cmd_input_line();
        std::strcpy(prev_cmd_line, rf::console_cmd_line);
    }
}

void win32_console_poll_input()
{
    HANDLE input_handle = GetStdHandle(STD_INPUT_HANDLE);
    INPUT_RECORD input_record;
    DWORD num_read = 0;
    while (true) {
        if (!PeekConsoleInput(input_handle, &input_record, 1, &num_read) || num_read == 0)
            break;
        if (!ReadConsoleInput(input_handle, &input_record, 1, &num_read) || num_read == 0)
            break;
        if (input_record.EventType == KEY_EVENT) {
            rf::key_process_event(input_record.Event.KeyEvent.wVirtualScanCode, input_record.Event.KeyEvent.bKeyDown, 0);
        }
    }
}

void win32_console_close()
{
    if (win32_console_enabled) {
        FreeConsole();
    }
}
