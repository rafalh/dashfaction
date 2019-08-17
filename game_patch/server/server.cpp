#include <patch_common/CallHook.h>
#include <patch_common/FunHook.h>
#include <patch_common/CodeInjection.h>
#include <patch_common/ShortTypes.h>
#include <common/rfproto.h>
#include <map>
#include <set>
#include "server.h"
#include "../commands.h"

struct VoteConfig
{
    bool enabled = false;
    int min_voters = 1;
    int min_percentage = 50;
    int time_limit_seconds = 60;
};

struct ServerAdditionalConfig
{
    VoteConfig vote_kick;
    VoteConfig vote_level;
    VoteConfig vote_extend;
    int spawn_protection_duration_ms = 1500;
};

bool g_win32_console = false;

const char* g_rcon_cmd_whitelist[] = {
    "kick",
    "level",
    "ban",
    "ban_ip",
    "map_ext",
};

ServerAdditionalConfig g_additional_server_config;

void SendChatLinePacket(const char* msg, rf::Player* target, rf::Player* sender = nullptr, bool is_team_msg = false)
{
    uint8_t buf[512];
    rfMessage& packet = *reinterpret_cast<rfMessage*>(buf);
    packet.type = RF_MESSAGE;
    packet.size = strlen(msg) + 3;
    packet.player_id = sender ? sender->nw_data->player_id : 0xFF;
    packet.is_team_msg = is_team_msg;
    strncpy(packet.message, msg, 256);
    if (target == nullptr)
        rf::NwSendReliablePacketToAll(buf, packet.size + 3, 0);
    else
        rf::NwSendReliablePacket(target, buf, packet.size + 3, 0);
}

struct Vote
{
    virtual ~Vote() {}
    virtual bool OnStart(std::string_view arg, rf::Player* source) = 0;
    virtual void OnAccepted() = 0;
    virtual void OnRejected() {}
    virtual bool OnPlayerLeave([[maybe_unused]] rf::Player* player) { return true; }
    virtual const VoteConfig& GetConfig() const = 0;
};

struct VoteKick : public Vote
{
    rf::Player* m_target_player;

    bool OnStart(std::string_view arg, rf::Player* source) override
    {
        std::string player_name{arg};
        m_target_player = FindBestMatchingPlayer(player_name.c_str());
        if (!m_target_player)
            return false;
        std::string msg = StringFormat("%s started a vote for kick of %s!", source->name.CStr(), m_target_player->name.CStr());
        SendChatLinePacket(msg.c_str(), nullptr);
        return true;
    }

    void OnAccepted() override
    {
        rf::KickPlayer(m_target_player);
    }

    bool OnPlayerLeave(rf::Player* player) override
    {
        return m_target_player != player;
    }

    const VoteConfig& GetConfig() const override
    {
        return g_additional_server_config.vote_kick;
    }
};

struct VoteExtend : public Vote
{
    rf::Player* m_target_player;

    bool OnStart([[maybe_unused]] std::string_view arg, rf::Player* source) override
    {
        std::string msg = StringFormat("%s started a vote for extending round time by 5 minutes!", source->name.CStr());
        SendChatLinePacket(msg.c_str(), nullptr);
        return true;
    }

    void OnAccepted() override
    {
        ExtendRoundTime(5);
    }

    const VoteConfig& GetConfig() const override
    {
        return g_additional_server_config.vote_extend;
    }
};

struct VoteLevel : public Vote
{
    std::string m_level_name;

    bool OnStart([[maybe_unused]] std::string_view arg, rf::Player* source) override
    {
        m_level_name = std::string{arg} + ".rfl";
        if (!rf::FileGetChecksum(m_level_name.c_str())) {
            SendChatLinePacket("Cannot find level!", source);
            return false;
        }
        std::string msg = StringFormat("%s started a vote for changing a level to %s!", source->name.CStr(), m_level_name.c_str());
        SendChatLinePacket(msg.c_str(), nullptr);
        return true;
    }

    void OnAccepted() override
    {
        auto& MultiChangeLevel = AddrAsRef<bool(const char* filename)>(0x0047BF50);
        MultiChangeLevel(m_level_name.c_str());
    }

    const VoteConfig& GetConfig() const override
    {
        return g_additional_server_config.vote_level;
    }
};

class VoteMgr
{
private:
    std::optional<std::unique_ptr<Vote>> active_vote;
    int num_votes_yes = 0;
    int num_votes_no = 0;
    std::map<rf::Player*, bool> players_who_voted;
    std::set<rf::Player*> remaining_players;
    int vote_start_time;

public:
    const std::optional<std::unique_ptr<Vote>>& GetActiveVote() const
    {
        return active_vote;
    }

    bool StartVote(std::unique_ptr<Vote>&& vote, std::string_view arg, rf::Player* source)
    {
        if (active_vote)
            return false;

        if (!vote->OnStart(arg, source))
            return false;

        active_vote = {std::move(vote)};
        vote_start_time = std::time(nullptr);
        SendChatLinePacket("Send chat message \"/vote yes\" or \"/vote no\" to vote.", nullptr);

        ++num_votes_yes;
        players_who_voted.insert({source, true});
        CheckForEarlyVoteFinish();

        return true;
    }

    void ResetActiveVote()
    {
        active_vote.reset();
        num_votes_yes = 0;
        num_votes_no = 0;
        players_who_voted.clear();
        remaining_players.clear();
    }

    void OnPlayerLeave(rf::Player* player)
    {
        if (!active_vote)
            return;

        active_vote.value()->OnPlayerLeave(player);
        remaining_players.erase(player);
        auto it = players_who_voted.find(player);
        if (it != players_who_voted.end()) {
            if (it->second)
                num_votes_yes--;
            else
                num_votes_no--;
        }
        CheckForEarlyVoteFinish();
    }

    void AddPlayerVote(bool is_yes_vote, rf::Player* source)
    {
        if (!active_vote)
            SendChatLinePacket("No vote in progress!", source);
        else if (players_who_voted.count(source) == 1)
            SendChatLinePacket("You already voted!", source);
        else if (remaining_players.count(source) == 0)
            SendChatLinePacket("You cannot vote!", source);
        else {
            if (is_yes_vote)
                num_votes_yes++;
            else
                num_votes_no++;
            remaining_players.erase(source);
            players_who_voted.insert({source, is_yes_vote});
            auto msg = StringFormat("Vote status: Yes - %d, No - %d, Remaining - %d", num_votes_yes, num_votes_no, remaining_players.size());
            CheckForEarlyVoteFinish();
        }
    }

    void FinishVote(bool is_accepted)
    {
        if (is_accepted) {
            SendChatLinePacket("Vote accepted!", nullptr);
            active_vote.value()->OnAccepted();
        }
        else {
            SendChatLinePacket("Vote rejected!", nullptr);
            active_vote.value()->OnRejected();
        }
        ResetActiveVote();
    }

    void CheckForEarlyVoteFinish()
    {
        if (num_votes_yes > num_votes_no + static_cast<int>(remaining_players.size()))
            FinishVote(true);
        else if (num_votes_no > num_votes_yes + static_cast<int>(remaining_players.size()))
            FinishVote(false);
    }

    void OnFrame()
    {
        if (!active_vote)
            return;

        const auto& vote_config = active_vote.value()->GetConfig();
        if (std::time(nullptr) - vote_start_time >= vote_config.time_limit_seconds) {
            SendChatLinePacket("Vote timed out!", nullptr);
            ResetActiveVote();
        }
    }
};

VoteMgr g_vote_mgr;

void ParseVoteConfig(const char* vote_name, VoteConfig& config, rf::StrParser& parser)
{
    std::string vote_option_name = StringFormat("$DF %s:", vote_name);
    if (parser.OptionalString(vote_option_name.c_str())) {
        config.enabled = parser.GetBool();
        rf::DcPrintf("DF %s: %s", vote_name, config.enabled ? "true" : "false");

        if (parser.OptionalString("+Min Voters:")) {
            config.min_voters = parser.GetUInt();
        }

        if (parser.OptionalString("+Min Percentage:")) {
            config.min_percentage = parser.GetUInt();
        }

        if (parser.OptionalString("+Time Limit:")) {
            config.time_limit_seconds = parser.GetUInt();
        }
    }
}

CodeInjection dedicated_server_load_config_patch{
    0x0046E216,
    [](auto& regs) {
        auto& parser = *reinterpret_cast<rf::StrParser*>(regs.esp - 4 + 0x4C0 - 0x470);
        ParseVoteConfig("Vote Kick", g_additional_server_config.vote_kick, parser);
        ParseVoteConfig("Vote Level", g_additional_server_config.vote_level, parser);
        ParseVoteConfig("Vote Extend", g_additional_server_config.vote_extend, parser);
        if (parser.OptionalString("$DF Spawn Protection Duration:")) {
            g_additional_server_config.spawn_protection_duration_ms = parser.GetUInt();
        }
    },
};

std::pair<std::string_view, std::string_view> StripBySpace(std::string_view str)
{
    auto space_pos = str.find(' ');
    if (space_pos == std::string_view::npos)
        return {str, {}};
    else
        return {str.substr(0, space_pos), str.substr(space_pos + 1)};
}

void HandleServerChatCommand(std::string_view server_command, rf::Player* sender)
{
    auto [cmd_name, cmd_arg] = StripBySpace(server_command);

    if (cmd_name == "info")
        SendChatLinePacket("Server powered by Dash Faction", sender);
    else if (cmd_name == "vote") {
        auto [vote_name, vote_arg] = StripBySpace(cmd_arg);
        if (vote_name == "kick")
            g_vote_mgr.StartVote(std::make_unique<VoteKick>(), vote_arg, sender);
        else if (vote_name == "level")
            g_vote_mgr.StartVote(std::make_unique<VoteLevel>(), vote_arg, sender);
        else if (vote_name == "extend")
            g_vote_mgr.StartVote(std::make_unique<VoteExtend>(), vote_arg, sender);
        else if (vote_name == "yes")
            g_vote_mgr.AddPlayerVote(true, sender);
        else if (vote_name == "no")
            g_vote_mgr.AddPlayerVote(false, sender);
        else
            SendChatLinePacket("Unrecognized vote type!", sender);
    }
    else
        SendChatLinePacket("Unrecognized server command!", sender);
}

bool CheckServerChatCommand(const char* msg, rf::Player* sender)
{
    const char server_prefix[] = "server ";
    if (msg[0] == '/')
        HandleServerChatCommand(msg + 1, sender);
    else if (strncmp(server_prefix, msg, sizeof(server_prefix) - 1) == 0)
        HandleServerChatCommand(msg + sizeof(server_prefix) - 1, sender);
    else
        return false;

    return true;
}

CodeInjection spawn_protection_duration_patch{
    0x0048089A,
    [](auto& regs) {
        *reinterpret_cast<int*>(regs.esp) = g_additional_server_config.spawn_protection_duration_ms;
    },
};

#if SERVER_WIN32_CONSOLE

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
        for (int i = 0; i < scr_buf_info.dwCursorPosition.X; ++i) WriteConsoleA(output_handle, " ", 1, nullptr, nullptr);
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
    INFO("Quiting after Console CTRL");
    static auto& close = AddrAsRef<int32_t>(0x01B0D758);
    close = 1;
    return TRUE;
}

void InputThreadProc()
{
    while (true) {
        INPUT_RECORD input_record;
        DWORD num_read = 0;
        ReadConsoleInput(GetStdHandle(STD_INPUT_HANDLE), &input_record, 1, &num_read);
    }
}

CallHook<void()> OsInitWindow_Server_hook{
    0x004B27C5,
    []() {
        AllocConsole();
        SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

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
                    ERR("unknown color %s", color.c_str());
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

        // PrintCmdInputLine();
    },
};

CallHook<void()> DcPutChar_NewLine_hook{
    0x0050A081,
    [] {
        HANDLE output_handle = GetStdHandle(STD_OUTPUT_HANDLE);
        WriteConsoleA(output_handle, "\r\n", 2, nullptr, nullptr);
    },
};

FunHook<void()> DcDrawServerConsole_hook{
    0x0050A770,
    []() {
        static char prev_cmd_line[256];
        if (strncmp(rf::dc_cmd_line, prev_cmd_line, std::size(prev_cmd_line)) != 0) {
            ResetConsoleCursorColumn(true);
            PrintCmdInputLine();
            strncpy(prev_cmd_line, rf::dc_cmd_line, std::size(prev_cmd_line));
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
        while (false) {
            if (!PeekConsoleInput(input_handle, &input_record, 1, &num_read) || num_read == 0)
                break;
            if (!ReadConsoleInput(input_handle, &input_record, 1, &num_read) || num_read == 0)
                break;
            if (input_record.EventType == KEY_EVENT)
                KeyProcessEvent(input_record.Event.KeyEvent.wVirtualScanCode, input_record.Event.KeyEvent.bKeyDown, 0);
        }

        return KeyGetFromQueue_hook.CallTarget();
    },
};

#endif // SERVER_WIN32_CONSOLE

void ServerInit()
{
    // Override rcon command whitelist
    WriteMemPtr(0x0046C794 + 1, g_rcon_cmd_whitelist);
    WriteMemPtr(0x0046C7D1 + 2, g_rcon_cmd_whitelist + std::size(g_rcon_cmd_whitelist));

    // Additional server config
    dedicated_server_load_config_patch.Install();

    // Apply customized spawn protection duration
    spawn_protection_duration_patch.Install();

#if SERVER_WIN32_CONSOLE // win32 console
    g_win32_console = stristr(GetCommandLineA(), "-win32-console") != nullptr;
    if (g_win32_console) {
        OsInitWindow_Server_hook.Install();
        // AsmWritter(0x0050A770).ret(); // null DcDrawServerConsole
        DcPrint_hook.Install();
        DcDrawServerConsole_hook.Install();
        KeyGetFromQueue_hook.Install();
        DcPutChar_NewLine_hook.Install();
    }
#endif
}

void ServerCleanup()
{
#if SERVER_WIN32_CONSOLE // win32 console
    if (g_win32_console)
        FreeConsole();
#endif
}

