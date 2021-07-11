#pragma once

#include <cstring>
#include <cstdio>
#include <cstdarg>
#include "../rf/gr/gr_font.h"

template<std::size_t N>
class StringBuffer
{
public:
    void append(char c)
    {
        if (pos_ + 1 < N) {
            buf_[pos_++] = c;
            buf_[pos_] = '\0';
        }
    }

    void append(const char* str)
    {
        std::size_t len = std::strlen(str);
        if (pos_ + len < N) {
            std::memcpy(&buf_[pos_], str, len);
            pos_ += len;
            buf_[pos_] = '\0';
        }
    }

    void vappendf(const char* format, va_list args)
    {
        std::size_t n = std::vsnprintf(buf_ + pos_, N - pos_, format, args);
        pos_ += std::min(n, N - pos_ - 1);
    }

    const char* get() const
    {
        return buf_;
    }

private:
    char buf_[N] = "";
    unsigned pos_ = 0;
};

class DebugNameValueBox
{
public:
    DebugNameValueBox(int x, int y) :
        x_(x), y_(y)
    {}

    void section(const char* name)
    {
        buffer_.append(name);
        buffer_.append('\n');
    }

    void print(const char* name, const char* val)
    {
        buffer_.append(name);
        buffer_.append(": ");
        buffer_.append(val);
        buffer_.append('\n');
    }

    void printf(const char* name, const char* format, ...)
    {
        std::va_list args;
        va_start(args, format);
        buffer_.append(name);
        buffer_.append(": ");
        buffer_.vappendf(format, args);
        buffer_.append('\n');
        va_end(args);
    }

    void print(const char* name, const rf::Vector3& val)
    {
        printf(name, "< %.2f, %.2f, %.2f >", val.x, val.y, val.z);
    }

    void render()
    {
        rf::gr::set_color(0, 0, 0, 255);
        rf::gr::string(x_ + 1, y_ + 1, buffer_.get());
        rf::gr::set_color(255, 255, 255, 255);
        rf::gr::string(x_, y_, buffer_.get());
    }

private:
    int x_;
    int y_;
    StringBuffer<2048> buffer_;
};

class DebugNameValueBoxTwoColumns
{
public:
    DebugNameValueBoxTwoColumns(int x, int y, int name_column_width) :
        name_x_(x), value_x_(x + name_column_width), y_(y)
    {}

    void section(const char* name)
    {
        name_buffer_.append(name);
        name_buffer_.append('\n');
        value_buffer_.append('\n');
    }

    void print(const char* name, const char* val)
    {
        name_buffer_.append(name);
        name_buffer_.append('\n');
        value_buffer_.append(val);
        value_buffer_.append('\n');
    }

    void printf(const char* name, const char* format, ...)
    {
        std::va_list args;
        va_start(args, format);
        name_buffer_.append(name);
        name_buffer_.append('\n');
        value_buffer_.vappendf(format, args);
        value_buffer_.append('\n');
        va_end(args);
    }

    void print(const char* name, const rf::Vector3& val)
    {
        printf(name, "< %.2f, %.2f, %.2f >", val.x, val.y, val.z);
    }

    void render()
    {
        rf::gr::set_color(0, 0, 0, 255);
        rf::gr::string(name_x_ + 1, y_ + 1, name_buffer_.get());
        rf::gr::string(value_x_ + 1, y_ + 1, value_buffer_.get());
        rf::gr::set_color(255, 255, 255, 255);
        rf::gr::string(name_x_, y_, name_buffer_.get());
        rf::gr::string(value_x_, y_, value_buffer_.get());
    }

private:
    int name_x_;
    int value_x_;
    int y_;
    StringBuffer<2048> name_buffer_;
    StringBuffer<2048> value_buffer_;
};

void debug_cmd_init();
void debug_cmd_render();
void debug_cmd_render_ui();
void profiler_init();
void profiler_do_frame_post();
void profiler_draw_ui();
void profiler_multi_init();
void register_obj_debug_commands();
void render_obj_debug_ui();
void debug_unresponsive_apply_patches();
void debug_unresponsive_init();
void debug_unresponsive_cleanup();
void debug_unresponsive_do_update();
void debug_cmd_multi_init();
