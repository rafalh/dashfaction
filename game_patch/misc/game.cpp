#include <windows.h>
#include <ctime>
#include <patch_common/CodeInjection.h>
#include <patch_common/FunHook.h>
#include <patch_common/CallHook.h>
#include <xlog/xlog.h>
#include "../os/console.h"
#include "../rf/misc.h"
#include "../rf/file/file.h"
#include "../rf/gr/gr.h"
#include "../rf/ui.h"
#include "../rf/gameseq.h"
#include "../rf/level.h"
#include "../multi/multi.h"

static std::unique_ptr<byte* []> g_screenshot_scanlines_buf;

static std::optional<std::string> get_screenshots_dir()
{
    auto full_path = string_format("%s\\screenshots", rf::root_path);
    if (full_path.size() > rf::max_path_len - 20) {
        xlog::error("Screenshots directory path is too long!");
        return {};
    }
    if (CreateDirectoryA(full_path.c_str(), nullptr))
        xlog::info("Created screenshots directory");
    else if (GetLastError() != ERROR_ALREADY_EXISTS) {
        xlog::error("Failed to create screenshots directory %lu", GetLastError());
        return {};
    }
    return {full_path};
}

FunHook<void(char*)> game_print_screen_hook{
    0x004366E0,
    [](char *filename_used) {
        static auto& pending_screenshot_filename = addr_as_ref<char[256]>(0x00636F14);
        static auto& dump_tga = addr_as_ref<bool>(0x0063708C);
        static auto& screenshot_pending = addr_as_ref<char>(0x00637085);

        auto now = std::time(nullptr);
        auto* tm = std::localtime(&now);
        char time_str[32];
        std::strftime(time_str, sizeof(time_str), "%Y%m%d_%H%M%S", tm);
        auto level_filename = std::string{get_filename_without_ext(rf::level.filename.c_str())};
        auto* dot_ext = dump_tga ? ".tga" : ".jpg";
        auto screenshots_dir = get_screenshots_dir();
        if (!screenshots_dir) {
            return;
        }
        int counter = 1;
        rf::File file;

        while (true) {
            if (counter == 1) {
                // Don't include the counter here
                std::snprintf(pending_screenshot_filename, sizeof(pending_screenshot_filename) - 4,
                    "%s\\%s_%s", screenshots_dir.value().c_str(), time_str, level_filename.c_str());
            } else {
                // Filename conflict - include a counter
                std::snprintf(pending_screenshot_filename, sizeof(pending_screenshot_filename) - 4,
                    "%s\\%s_%d_%s", screenshots_dir.value().c_str(), time_str, counter, level_filename.c_str());
            }
            auto filename_with_ext = std::string{pending_screenshot_filename} + dot_ext;
            bool exists = file.find(filename_with_ext.c_str());
            if (!exists) {
                break;
            }
            counter++;
            if (counter == 1000) {
                xlog::error("Cannot find a free filename for a screenshot");
                return;
            }
        }

        screenshot_pending = true;

        if (filename_used) {
            std::strcpy(filename_used, pending_screenshot_filename);
        }
    },
};

CodeInjection jpeg_write_bitmap_overflow_fix1{
    0x0055A066,
    [](auto& regs) {
        g_screenshot_scanlines_buf = std::make_unique<byte* []>(rf::gr::screen.max_h);
        regs.ecx = g_screenshot_scanlines_buf.get();
        regs.eip = 0x0055A06D;
    },
};

CodeInjection jpeg_write_bitmap_overflow_fix2{
    0x0055A0DF,
    [](auto& regs) {
        regs.eax = g_screenshot_scanlines_buf.get();
        regs.eip = 0x0055A0E6;
    },
};

int bm_load_if_exists(const char* name, int unk, bool generate_mipmaps)
{
    if (rf::file_exists(name)) {
        return rf::bm::load(name, unk, generate_mipmaps);
    }
    return -1;
}

CallHook<void(int, int, int, rf::gr::Mode)> game_render_cursor_gr_bitmap_hook{
    0x004354E4,
    [](int bm_handle, int x, int y, rf::gr::Mode mode) {
        if (rf::ui::scale_y >= 2.0f) {
            static int cursor_1_bmh = bm_load_if_exists("cursor_1.tga", -1, false);
            if (cursor_1_bmh != -1) {
                bm_handle = cursor_1_bmh;
            }
        }
        game_render_cursor_gr_bitmap_hook.call_target(bm_handle, x, y, mode);
    },
};

static ConsoleCommand2 screenshot_cmd{
    "screenshot",
    []() {
        auto game_print_screen = addr_as_ref<void(char* filename)>(0x004366E0);

        char buf[MAX_PATH];
        game_print_screen(buf);
        rf::console::printf("Screenshot saved in %s", buf);
    },
};

CodeInjection gameplay_render_frame_display_full_screen_image_injection{
    0x00432CAF,
    [](auto& regs) {
        // Change gr mode to one that uses alpha blending for Display_Fullscreen_Image event handling in
        // gameplay_render_frame function
        static rf::gr::Mode mode{
            rf::gr::TEXTURE_SOURCE_WRAP,
            rf::gr::COLOR_SOURCE_TEXTURE,
            rf::gr::ALPHA_SOURCE_VERTEX_TIMES_TEXTURE,
            rf::gr::ALPHA_BLEND_ALPHA,
            rf::gr::ZBUFFER_TYPE_NONE,
            rf::gr::FOG_ALLOWED,
        };
        regs.edx = mode;
    },
};

static FunHook<void(rf::GameState, bool)> rf_do_state_hook{
    0x004B1E70,
    [](rf::GameState state, bool paused) {
        if (state == rf::GS_MULTI_LEVEL_DOWNLOAD) {
            multi_level_download_do_frame();
        }
        else if (state == rf::GS_END_GAME) {
            multi_level_download_abort();
            rf_do_state_hook.call_target(state, paused);
        }
        else {
            rf_do_state_hook.call_target(state, paused);
        }
    },
};

void game_apply_patch()
{
    // Override screenshot filename and directory
    game_print_screen_hook.install();

    // Fix buffer overflow in screenshot to JPG conversion code
    jpeg_write_bitmap_overflow_fix1.install();
    jpeg_write_bitmap_overflow_fix2.install();

    // Bigger cursor bitmap support
    game_render_cursor_gr_bitmap_hook.install();

    // Support textures with alpha channel in Display_Fullscreen_Image event
    gameplay_render_frame_display_full_screen_image_injection.install();

    // States support
    rf_do_state_hook.install();

    // Commands
    screenshot_cmd.register_cmd();
}
