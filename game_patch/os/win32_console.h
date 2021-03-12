#pragma once

namespace rf::gr
{
    struct Color;
}

void win32_console_pre_init();
void win32_console_init();
void win32_console_close();
bool win32_console_is_enabled();
void win32_console_update();
void win32_console_poll_input();
void win32_console_output(const char* text, const rf::gr::Color* color);
void win32_console_new_line();
