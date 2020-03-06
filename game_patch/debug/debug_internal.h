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
        current_y += rf::GrGetFontHeight(-1) + 2;
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

        current_y += rf::GrGetFontHeight(-1) + 2;
    }

    void Print(const char* name, const rf::Vector3& val)
    {
        auto str = StringFormat("< %.2f, %.2f, %.2f >", val.x, val.y, val.z);
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
        rf::GrSetColor(0, 0, 0, 255);
        rf::GrDrawText(x + 1, y + 1, str, -1, rf::gr_text_material);
        rf::GrSetColor(255, 255, 255, 255);
        rf::GrDrawText(x, y, str, -1, rf::gr_text_material);
    }
};

void DebugCmdApplyPatches();
void DebugCmdInit();
void DebugCmdRender();
void DebugCmdRenderUI();
void ProfilerInit();
void ProfilerDrawUI();
void RegisterObjDebugCommands();
void RenderObjDebugUI();
