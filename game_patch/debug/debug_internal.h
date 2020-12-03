#pragma once

#include "../rf/graphics.h"

class DebugNameValueBox
{
    int name_x;
    int value_x;
    int initial_y;
    int current_y;

public:
    DebugNameValueBox(int x, int y, int name_column_width = -1) :
        name_x(x), value_x(x + name_column_width), initial_y(y), current_y(y)
    {
        value_x = name_column_width > 0 ? name_x + name_column_width : -1;
    }

    void Section(const char* name)
    {
        if (current_y > initial_y) {
            current_y += 5;
        }
        PrintInternal(name_x, current_y, name);
        current_y += rf::gr_get_font_height(-1) + 2;
    }

    void Print(const char* name, const char* val)
    {
        bool two_column_layout = value_x > 0;
        if (two_column_layout) {
            PrintInternal(name_x, current_y, name);
            PrintInternal(value_x, current_y, val);
        }
        else {
            std::string buf;
            buf.assign(name);
            buf.append(": ");
            buf.append(val);
            PrintInternal(name_x, current_y, buf.c_str());
        }

        current_y += rf::gr_get_font_height(-1) + 2;
    }

    void Print(const char* name, const rf::Vector3& val)
    {
        auto str = string_format("< %.2f, %.2f, %.2f >", val.x, val.y, val.z);
        Print(name, str.c_str());
    }

    void Printf(const char* name, const char* fmt, ...)
    {
        char buffer[256];
        va_list args;
        va_start(args, fmt);
        vsprintf(buffer, fmt, args);
        va_end(args);
        Print(name, buffer);
    }

    void Reset()
    {
        current_y = initial_y;
    }

private:
    void PrintInternal(int x, int y, const char* str)
    {
        rf::gr_set_color_rgba(0, 0, 0, 255);
        rf::gr_string(x + 1, y + 1, str);
        rf::gr_set_color_rgba(255, 255, 255, 255);
        rf::gr_string(x, y, str);
    }
};

void debug_cmd_apply_patches();
void debug_cmd_init();
void debug_cmd_render();
void debug_cmd_render_ui();
void profiler_init();
void profiler_draw_ui();
void register_obj_debug_commands();
void render_obj_debug_ui();
void debug_unresponsive_apply_patches();
void debug_unresponsive_init();
void debug_unresponsive_cleanup();
void debug_unresponsive_do_update();
