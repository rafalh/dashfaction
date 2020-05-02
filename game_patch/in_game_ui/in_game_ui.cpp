#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/AsmOpcodes.h>
#include <patch_common/AsmWriter.h>
#include "../rf/graphics.h"

constexpr int CHAT_MSG_MAX_LEN = 224;

FunHook<void(uint16_t)> ChatSayAddChar_hook{
    0x00444740,
    [](uint16_t key) {
        if (key)
            ChatSayAddChar_hook.CallTarget(key);
    },
};

FunHook<void(const char*, bool)> ChatSayAccept_hook{
    0x00444440,
    [](const char* msg, bool is_team_msg) {
        std::string msg_str{msg};
        if (msg_str.size() > CHAT_MSG_MAX_LEN)
            msg_str.resize(CHAT_MSG_MAX_LEN);
        ChatSayAccept_hook.CallTarget(msg_str.c_str(), is_team_msg);
    },
};

CallHook<void(int, int, int, int, int, int, int, int, int, char, char, int)> gr_bitmap_stretched_message_log_hook{
    0x004551F0,
    [](int bm_handle, int dst_x, int dst_y, int dst_w, int dst_h, int src_x, int src_y, int src_w, int src_h,
        char unk_u, char unk_v, int render_state) {

        float scale_x = rf::GrGetMaxWidth() / 640.0f;
        float scale_y = rf::GrGetMaxHeight() / 480.0f;
        dst_w = static_cast<int>(src_w * scale_x);
        dst_h = static_cast<int>(src_h * scale_y);
        dst_x = (rf::GrGetMaxWidth() - dst_w) / 2;
        dst_y = (rf::GrGetMaxHeight() - dst_h) / 2;
        gr_bitmap_stretched_message_log_hook.CallTarget(bm_handle, dst_x, dst_y, dst_w, dst_h,
            src_x, src_y, src_w, src_h, unk_u, unk_v, render_state);

        auto& message_log_entries_clip_h = AddrAsRef<int>(0x006425D4);
        auto& message_log_entries_clip_y = AddrAsRef<int>(0x006425D8);
        auto& message_log_entries_clip_w = AddrAsRef<int>(0x006425DC);
        auto& message_log_entries_clip_x = AddrAsRef<int>(0x006425E0);

        message_log_entries_clip_x = static_cast<int>(dst_x + scale_x * 30);
        message_log_entries_clip_y = static_cast<int>(dst_y + scale_y * 41);
        message_log_entries_clip_w = static_cast<int>(scale_x * 313);
        message_log_entries_clip_h = static_cast<int>(scale_y * 296);
    },
};

void ApplyGameUiPatches()
{
    // Chat color alpha
    using namespace asm_regs;
    AsmWriter(0x00477490, 0x004774A4).mov(eax, 0x30); // chatbox border
    AsmWriter(0x00477528, 0x00477535).mov(ebx, 0x40); // chatbox background
    AsmWriter(0x00478E00, 0x00478E14).mov(eax, 0x30); // chat input border
    AsmWriter(0x00478E91, 0x00478E9E).mov(ebx, 0x40); // chat input background

    // Fix game beeping every frame if chat input buffer is full
    ChatSayAddChar_hook.Install();

    // Change chat input limit to 224 (RF can support 255 safely but PF kicks if message is longer than 224)
    WriteMem<i32>(0x0044474A + 1, CHAT_MSG_MAX_LEN);

    // Add chat message limit for say/teamsay commands
    ChatSayAccept_hook.Install();

    // Fix message log rendering in resolutions with ratio different than 4:3
    gr_bitmap_stretched_message_log_hook.Install();
}
