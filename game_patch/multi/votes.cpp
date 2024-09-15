#include <string_view>
#include <map>
#include <set>
#include <ctime>
#include <format>
#include "../rf/player/player.h"
#include "../rf/multi.h"
#include "../rf/gameseq.h"
#include "../rf/misc.h"
#include "../os/console.h"
#include "../misc/player.h"
#include "../main/main.h"
#include <common/utils/list-utils.h>
#include "server_internal.h"
#include "multi.h"

struct Vote
{
private:
    int num_votes_yes = 0;
    int num_votes_no = 0;
    std::time_t start_time = 0;
    bool reminder_sent = false;
    std::map<rf::Player*, bool> players_who_voted;
    std::set<rf::Player*> remaining_players;
    rf::Player* owner;

public:
    virtual ~Vote() = default;

    bool start(std::string_view arg, rf::Player* source)
    {
        if (!process_vote_arg(arg, source)) {
            return false;
        }

        owner = source;

        send_vote_starting_msg(source);

        start_time = std::time(nullptr);

        // prepare allowed player list
        auto player_list = SinglyLinkedList{rf::player_list};
        for (auto& player : player_list) {
            if (&player != source && !get_player_additional_data(&player).is_browser) {
                remaining_players.insert(&player);
            }
        }

        ++num_votes_yes;
        players_who_voted.insert({source, true});

        return check_for_early_vote_finish();
    }

    virtual bool on_player_leave(rf::Player* player)
    {
        remaining_players.erase(player);
        auto it = players_who_voted.find(player);
        if (it != players_who_voted.end()) {
            if (it->second)
                num_votes_yes--;
            else
                num_votes_no--;
        }
        return check_for_early_vote_finish();
    }

    [[nodiscard]] virtual bool is_allowed_in_limbo_state() const
    {
        return true;
    }

    bool add_player_vote(bool is_yes_vote, rf::Player* source)
    {
        if (players_who_voted.count(source) == 1)
            send_chat_line_packet("You already voted!", source);
        else if (remaining_players.count(source) == 0)
            send_chat_line_packet("You cannot vote!", source);
        else {
            if (is_yes_vote)
                num_votes_yes++;
            else
                num_votes_no++;
            remaining_players.erase(source);
            players_who_voted.insert({source, is_yes_vote});
            auto msg = std::format("\xA6 Vote status:  Yes: {}  No: {}  Waiting: {}", num_votes_yes, num_votes_no, remaining_players.size());
            send_chat_line_packet(msg.c_str(), nullptr);
            return check_for_early_vote_finish();
        }
        return true;
    }

    bool do_frame()
    {
        const auto& vote_config = get_config();
        std::time_t passed_time_sec = std::time(nullptr) - start_time;
        if (passed_time_sec >= vote_config.time_limit_seconds) {
            send_chat_line_packet("\xA6 Vote timed out!", nullptr);
            return false;
        }
        if (passed_time_sec >= vote_config.time_limit_seconds / 2 && !reminder_sent) {
            // Send reminder to player who did not vote yet
            for (rf::Player* player : remaining_players) {
                send_chat_line_packet("\xA6 Send message \"/vote yes\" or \"/vote no\" to vote.", player);
            }
            reminder_sent = true;
        }
        return true;
    }

    bool try_cancel_vote(rf::Player* source)
    {
        if (owner != source) {
            send_chat_line_packet("You cannot cancel a vote you didn't start!", source);
            return false;
        }

        send_chat_line_packet("\xA6 Vote canceled!", nullptr);
        return true;
    }



protected:
    [[nodiscard]] virtual std::string get_title() const = 0;
    [[nodiscard]] virtual const VoteConfig& get_config() const = 0;

    virtual bool process_vote_arg([[maybe_unused]] std::string_view arg, [[maybe_unused]] rf::Player* source)
    {
        return true;
    }

    virtual void on_accepted()
    {
        send_chat_line_packet("\xA6 Vote passed!", nullptr);
    }

    virtual void on_rejected()
    {
        send_chat_line_packet("\xA6 Vote failed!", nullptr);
    }

    void send_vote_starting_msg(rf::Player* source)
    {
        auto title = get_title();
        auto msg = std::format(
            "\n=============== VOTE STARTING ===============\n"
            "{} vote started by {}.\n"
            "Send message \"/vote yes\" or \"/vote no\" to participate.",
            title.c_str(), source->name.c_str());
        send_chat_line_packet(msg.c_str(), nullptr);
    }

    void finish_vote(bool is_accepted)
    {
        if (is_accepted) {
            on_accepted();
        }
        else {
            on_rejected();
        }
    }

    bool check_for_early_vote_finish()
    {
        if (num_votes_yes > num_votes_no + static_cast<int>(remaining_players.size())) {
            finish_vote(true);
            return false;
        }
        if (num_votes_no > num_votes_yes + static_cast<int>(remaining_players.size())) {
            finish_vote(false);
            return false;
        }
        return true;
    }
};

struct VoteKick : public Vote
{
    rf::Player* m_target_player;

    bool process_vote_arg(std::string_view arg, [[ maybe_unused ]] rf::Player* source) override
    {
        std::string player_name{arg};
        m_target_player = find_best_matching_player(player_name.c_str());
        return m_target_player != nullptr;
    }

    [[nodiscard]] std::string get_title() const override
    {
        return std::format("KICK PLAYER '{}'", m_target_player->name);
    }

    void on_accepted() override
    {
        send_chat_line_packet("\xA6 Vote passed: kicking player", nullptr);
        rf::multi_kick_player(m_target_player);
    }

    bool on_player_leave(rf::Player* player) override
    {
        if (m_target_player == player) {
            return false;
        }
        return Vote::on_player_leave(player);
    }

    [[nodiscard]] const VoteConfig& get_config() const override
    {
        return g_additional_server_config.vote_kick;
    }
};

struct VoteExtend : public Vote
{
    rf::Player* m_target_player;

    [[nodiscard]] std::string get_title() const override
    {
        return "EXTEND ROUND BY 5 MINUTES";
    }

    void on_accepted() override
    {
        send_chat_line_packet("\xA6 Vote passed: extending round", nullptr);
        extend_round_time(5);
    }

    [[nodiscard]] bool is_allowed_in_limbo_state() const override
    {
        return false;
    }

    [[nodiscard]] const VoteConfig& get_config() const override
    {
        return g_additional_server_config.vote_extend;
    }
};

struct VoteLevel : public Vote
{
    std::string m_level_name;

    bool process_vote_arg([[maybe_unused]] std::string_view arg, rf::Player* source) override
    {
        m_level_name = std::string{arg} + ".rfl";
        if (!rf::get_file_checksum(m_level_name.c_str())) {
            send_chat_line_packet("Cannot find specified level!", source);
            return false;
        }
        return true;
    }

    [[nodiscard]] std::string get_title() const override
    {
        return std::format("LOAD LEVEL '{}'", m_level_name);
    }

    void on_accepted() override
    {
        send_chat_line_packet("\xA6 Vote passed: changing level", nullptr);
        rf::multi_change_level(m_level_name.c_str());
    }

    [[nodiscard]] bool is_allowed_in_limbo_state() const override
    {
        return false;
    }

    [[nodiscard]] const VoteConfig& get_config() const override
    {
        return g_additional_server_config.vote_level;
    }
};

struct VoteRestart : public Vote
{
    [[nodiscard]] std::string get_title() const override
    {
        return "RESTART LEVEL";
    }

    void on_accepted() override
    {
        send_chat_line_packet("\xA6 Vote passed: restarting level", nullptr);
        restart_current_level();
    }

    [[nodiscard]] bool is_allowed_in_limbo_state() const override
    {
        return false;
    }

    [[nodiscard]] const VoteConfig& get_config() const override
    {
        return g_additional_server_config.vote_restart;
    }
};

struct VoteNext : public Vote
{
    [[nodiscard]] std::string get_title() const override
    {
        return "LOAD NEXT LEVEL";
    }

    void on_accepted() override
    {
        send_chat_line_packet("\xA6 Vote passed: loading next level", nullptr);
        load_next_level();
    }

    [[nodiscard]] bool is_allowed_in_limbo_state() const override
    {
        return false;
    }

    [[nodiscard]] const VoteConfig& get_config() const override
    {
        return g_additional_server_config.vote_next;
    }
};

struct VotePrevious : public Vote
{
    [[nodiscard]] std::string get_title() const override
    {
        return "LOAD PREV LEVEL";
    }

    void on_accepted() override
    {
        send_chat_line_packet("\xA6 Vote passed: loading previous level", nullptr);
        load_prev_level();
    }

    [[nodiscard]] bool is_allowed_in_limbo_state() const override
    {
        return false;
    }

    [[nodiscard]] const VoteConfig& get_config() const override
    {
        return g_additional_server_config.vote_previous;
    }
};

class VoteMgr
{
private:
    std::optional<std::unique_ptr<Vote>> active_vote;

public:
    template<typename T>
    bool StartVote(std::string_view arg, rf::Player* source)
    {
        if (active_vote) {
            send_chat_line_packet("Another vote is currently in progress!", source);
            return false;
        }

        auto vote = std::make_unique<T>();

        if (!vote->get_config().enabled) {
            send_chat_line_packet("This vote type is disabled!", source);
            return false;
        }

        if (!vote->is_allowed_in_limbo_state() && rf::gameseq_get_state() != rf::GS_GAMEPLAY) {
            send_chat_line_packet("Vote cannot be started now!", source);
            return false;
        }

        if (!vote->start(arg, source)) {
            return false;
        }

        active_vote = {std::move(vote)};
        return true;
    }

    void on_player_leave(rf::Player* player)
    {
        if (active_vote && !active_vote.value()->on_player_leave(player)) {
            active_vote.reset();
        }
    }

    void OnLimboStateEnter()
    {
        if (active_vote && !active_vote.value()->is_allowed_in_limbo_state()) {
            send_chat_line_packet("\xA6 Vote canceled!", nullptr);
            active_vote.reset();
        }
    }

    void add_player_vote(bool is_yes_vote, rf::Player* source)
    {
        if (!active_vote) {
            send_chat_line_packet("No vote in progress!", source);
            return;
        }

        if (!active_vote.value()->add_player_vote(is_yes_vote, source)) {
            active_vote.reset();
        }
    }

    void try_cancel_vote(rf::Player* source)
    {
        if (!active_vote) {
            send_chat_line_packet("No vote in progress!", source);
            return;
        }

        if (active_vote.value()->try_cancel_vote(source)) {
            active_vote.reset();
        }
    }

    void do_frame()
    {
        if (!active_vote)
            return;

        if (!active_vote.value()->do_frame()) {
            active_vote.reset();
        }
    }
};

VoteMgr g_vote_mgr;

void handle_vote_command(std::string_view vote_name, std::string_view vote_arg, rf::Player* sender)
{
    if (get_player_additional_data(sender).is_browser) {
        send_chat_line_packet("Browsers are not allowed to vote!", sender);
        return;
    }
    if (vote_name == "kick")
        g_vote_mgr.StartVote<VoteKick>(vote_arg, sender);
    else if (vote_name == "level" || vote_name == "map")
        g_vote_mgr.StartVote<VoteLevel>(vote_arg, sender);
    else if (vote_name == "extend" || vote_name == "ext")
        g_vote_mgr.StartVote<VoteExtend>(vote_arg, sender);
    else if (vote_name == "restart" || vote_name == "rest")
        g_vote_mgr.StartVote<VoteRestart>(vote_arg, sender);
    else if (vote_name == "next")
        g_vote_mgr.StartVote<VoteNext>(vote_arg, sender);
    else if (vote_name == "previous" || vote_name == "prev")
        g_vote_mgr.StartVote<VotePrevious>(vote_arg, sender);
    else if (vote_name == "yes" || vote_name == "y")
        g_vote_mgr.add_player_vote(true, sender);
    else if (vote_name == "no" || vote_name == "n")
        g_vote_mgr.add_player_vote(false, sender);
    else if (vote_name == "cancel")
        g_vote_mgr.try_cancel_vote(sender);
    else
        send_chat_line_packet("Unrecognized vote type!", sender);
}

void server_vote_do_frame()
{
    g_vote_mgr.do_frame();
}

void server_vote_on_limbo_state_enter()
{
    g_vote_mgr.OnLimboStateEnter();
}
