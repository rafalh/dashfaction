#include <patch_common/CodeInjection.h>
#include <patch_common/CallHook.h>
#include <patch_common/AsmWriter.h>
#include <patch_common/MemUtils.h>
#include "../rf/gr/gr.h"

static auto& message_log_bm_w = addr_as_ref<int>(0x006428C4);
static auto& message_log_bm_h = addr_as_ref<int>(0x006428C8);
static auto& message_log_bg_x = addr_as_ref<int>(0x006428CC);
static auto& message_log_bg_y = addr_as_ref<int>(0x006428D0);
static auto& message_log_entries_clip_h = addr_as_ref<int>(0x006425D4);
static auto& message_log_entries_clip_y = addr_as_ref<int>(0x006425D8);
static auto& message_log_entries_clip_w = addr_as_ref<int>(0x006425DC);
static auto& message_log_entries_clip_x = addr_as_ref<int>(0x006425E0);
static float message_log_scale_x;
static float message_log_scale_y;
static int message_log_no_messages_text_y;

CodeInjection message_log_init_injection{
    0x00454CD7,
    []() {
        message_log_scale_x = rf::gr::screen_width() / 640.0f;
        message_log_scale_y = rf::gr::screen_height() / 480.0f;

        auto clip_w = static_cast<float>(rf::gr::clip_width());
        auto clip_h = static_cast<float>(rf::gr::clip_height());
        message_log_bg_x = static_cast<int>((clip_w - message_log_bm_w * message_log_scale_x) / 2.0f);
        message_log_bg_y = static_cast<int>((clip_h - message_log_bm_h * message_log_scale_y) / 2.0f);
        message_log_entries_clip_x = message_log_bg_x + static_cast<int>(30.0f * message_log_scale_x);
        message_log_entries_clip_y = message_log_bg_y + static_cast<int>(41.0f * message_log_scale_y);
        message_log_entries_clip_w = static_cast<int>(313.0f * message_log_scale_x);
        message_log_entries_clip_h = static_cast<int>(296.0f * message_log_scale_y);
        message_log_no_messages_text_y = static_cast<int>(180.0f * message_log_scale_y);
    },
};

CallHook<void(int, int, int, int, int, int, int, int, int, bool, bool, rf::gr::Mode)>
    message_log_render_gr_bitmap_stretched_hook{
        0x004551F0,
        [](int bm_handle, int x, int y, int w, int h, int sx, int sy, int sw, int sh, bool flip_x, bool flip_y,
           rf::gr::Mode mode) {
            w = static_cast<int>(sw * message_log_scale_x);
            h = static_cast<int>(sh * message_log_scale_y);
            message_log_render_gr_bitmap_stretched_hook.call_target(
                bm_handle, x, y, w, h, sx, sy, sw, sh, flip_x, flip_y, mode
            );
        },
    };

void message_log_apply_patch()
{
    // Fix message log rendering in resolutions with ratio different than 4:3
    message_log_init_injection.install();
    message_log_render_gr_bitmap_stretched_hook.install();
    AsmWriter{0x00455299, 0x0045529F}.add(asm_regs::ecx, &message_log_no_messages_text_y);
}
