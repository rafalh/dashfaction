#include <string_view>
#include "../rf.h"
#include "server_internal.h"
#include "../commands.h"
#include "../main.h"
#include <map>
#include <set>

struct Vote
{
private:
    int num_votes_yes = 0;
    int num_votes_no = 0;
    int start_time = 0;
    bool reminder_sent = false;
    std::map<rf::Player*, bool> players_who_voted;
    std::set<rf::Player*> remaining_players;

public:
    virtual ~Vote() {}

    bool Start(std::string_view arg, rf::Player* source)
    {
        if (!ProcessVoteArg(arg, source)) {
            return false;
        }

        SendVoteStartingMsg(source);

        start_time = std::time(nullptr);

        // prepare allowed player list
        rf::Player* player = rf::player_list;
        while (player) {
            if (player != source && !GetPlayerAdditionalData(player).is_browser) {
                remaining_players.insert(player);
            }
            player = player->next;
            if (player == rf::player_list) {
                break;
            }
        }

        ++num_votes_yes;
        players_who_voted.insert({source, true});

        return CheckForEarlyVoteFinish();
    }

    virtual bool OnPlayerLeave(rf::Player* player)
    {
        remaining_players.erase(player);
        auto it = players_who_voted.find(player);
        if (it != players_who_voted.end()) {
            if (it->second)
                num_votes_yes--;
            else
                num_votes_no--;
        }
        return CheckForEarlyVoteFinish();
    }

    bool AddPlayerVote(bool is_yes_vote, rf::Player* source)
    {
        if (players_who_voted.count(source) == 1)
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
            auto msg = StringFormat("\xA6 Vote status:  Yes: %d  No: %d  Waiting: %d", num_votes_yes, num_votes_no, remaining_players.size());
            SendChatLinePacket(msg.c_str(), nullptr);
            return CheckForEarlyVoteFinish();
        }
        return true;
    }

    bool DoFrame()
    {
        const auto& vote_config = GetConfig();
        int passed_time_sec = std::time(nullptr) - start_time;
        if (passed_time_sec >= vote_config.time_limit_seconds) {
            SendChatLinePacket("\xA6 Vote timed out!", nullptr);
            return false;
        }
        else if (passed_time_sec >= vote_config.time_limit_seconds / 2 && !reminder_sent) {
            // Send reminder to player who did not vote yet
            for (auto player : remaining_players) {
                SendChatLinePacket("\xA6 Send message \"/vote yes\" or \"/vote no\" to vote.", player);
            }
            reminder_sent = true;
        }
        return true;
    }

protected:
    virtual std::string GetTitle() = 0;
    virtual const VoteConfig& GetConfig() const = 0;

    virtual bool ProcessVoteArg([[maybe_unused]] std::string_view arg, [[maybe_unused]] rf::Player* source)
    {
        return true;
    }

    virtual void OnAccepted()
    {
        SendChatLinePacket("\xA6 Vote passed!", nullptr);
    }

    virtual void OnRejected()
    {
        SendChatLinePacket("\xA6 Vote failed!", nullptr);
    }

    void SendVoteStartingMsg(rf::Player* source)
    {
        auto title = GetTitle();
        auto msg = StringFormat(
            "\n=============== VOTE STARTING ===============\n"
            "%s vote started by %s.\n"
            "Send message \"/vote yes\" or \"/vote no\" to participate.",
            title.c_str(), source->name.CStr());
        SendChatLinePacket(msg.c_str(), nullptr);
    }

    void FinishVote(bool is_accepted)
    {
        if (is_accepted) {
            OnAccepted();
        }
        else {
            OnRejected();
        }
    }

    bool CheckForEarlyVoteFinish()
    {
        if (num_votes_yes > num_votes_no + static_cast<int>(remaining_players.size())) {
            FinishVote(true);
            return false;
        }
        else if (num_votes_no > num_votes_yes + static_cast<int>(remaining_players.size())) {
            FinishVote(false);
            return false;
        }
        return true;
    }
};

struct VoteKick : public Vote
{
    rf::Player* m_target_player;

    bool ProcessVoteArg(std::string_view arg, [[ maybe_unused ]] rf::Player* source) override
    {
        std::string player_name{arg};
        m_target_player = FindBestMatchingPlayer(player_name.c_str());
        if (!m_target_player)
            return false;
        return true;
    }

    std::string GetTitle() override
    {
        return StringFormat("KICK PLAYER '%s'", m_target_player->name.CStr());
    }

    void OnAccepted() override
    {
        SendChatLinePacket("\xA6 Vote passed: kicking player", nullptr);
        rf::KickPlayer(m_target_player);
    }

    bool OnPlayerLeave(rf::Player* player) override
    {
        if (m_target_player == player) {
            return false;
        }
        return Vote::OnPlayerLeave(player);
    }

    const VoteConfig& GetConfig() const override
    {
        return g_additional_server_config.vote_kick;
    }
};

struct VoteExtend : public Vote
{
    rf::Player* m_target_player;

    std::string GetTitle() override
    {
        return "EXTEND ROUND BY 5 MINUTES";
    }

    void OnAccepted() override
    {
        SendChatLinePacket("\xA6 Vote passed: extending round", nullptr);
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

    bool ProcessVoteArg([[maybe_unused]] std::string_view arg, rf::Player* source) override
    {
        m_level_name = std::string{arg} + ".rfl";
        if (!rf::FileGetChecksum(m_level_name.c_str())) {
            SendChatLinePacket("Cannot find specified level!", source);
            return false;
        }
        return true;
    }

    std::string GetTitle() override
    {
        return StringFormat("LOAD LEVEL '%s'", m_level_name.c_str());
    }

    void OnAccepted() override
    {
        SendChatLinePacket("\xA6 Vote passed: changing level", nullptr);
        auto& MultiChangeLevel = AddrAsRef<bool(const char* filename)>(0x0047BF50);
        MultiChangeLevel(m_level_name.c_str());
    }

    const VoteConfig& GetConfig() const override
    {
        return g_additional_server_config.vote_level;
    }
};

struct VoteRestart : public Vote
{
    std::string GetTitle() override
    {
        return "RESTART LEVEL";
    }

    void OnAccepted() override
    {
        SendChatLinePacket("\xA6 Vote passed: restarting level", nullptr);
        RestartCurrentLevel();
    }

    const VoteConfig& GetConfig() const override
    {
        return g_additional_server_config.vote_restart;
    }
};

struct VoteNext : public Vote
{
    std::string GetTitle() override
    {
        return "LOAD NEXT LEVEL";
    }

    void OnAccepted() override
    {
        SendChatLinePacket("\xA6 Vote passed: loading next level", nullptr);
        LoadNextLevel();
    }

    const VoteConfig& GetConfig() const override
    {
        return g_additional_server_config.vote_next;
    }
};

struct VotePrevious : public Vote
{
    std::string GetTitle() override
    {
        return "LOAD PREV LEVEL";
    }

    void OnAccepted() override
    {
        SendChatLinePacket("\xA6 Vote passed: loading previous level", nullptr);
        LoadPrevLevel();
    }

    const VoteConfig& GetConfig() const override
    {
        return g_additional_server_config.vote_previous;
    }
};

class VoteMgr
{
private:
    std::optional<std::unique_ptr<Vote>> active_vote;

public:
    const std::optional<std::unique_ptr<Vote>>& GetActiveVote() const
    {
        return active_vote;
    }

    template<typename T>
    bool StartVote(std::string_view arg, rf::Player* source)
    {
        if (active_vote) {
            SendChatLinePacket("Another vote is currently in progress!", source);
            return false;
        }

        auto vote = std::make_unique<T>();
        if (!vote->Start(arg, source)) {
            return false;
        }

        active_vote = {std::move(vote)};
        return true;
    }

    void OnPlayerLeave(rf::Player* player)
    {
        if (!active_vote) {
            return;
        }

        if (!active_vote.value()->OnPlayerLeave(player)) {
            active_vote.reset();
        }
    }

    void AddPlayerVote(bool is_yes_vote, rf::Player* source)
    {
        if (!active_vote) {
            SendChatLinePacket("No vote in progress!", source);
            return;
        }

        if (!active_vote.value()->AddPlayerVote(is_yes_vote, source)) {
            active_vote.reset();
        }
    }
    void DoFrame()
    {
        if (!active_vote)
            return;

        if (!active_vote.value()->DoFrame()) {
            active_vote.reset();
        }
    }
};

VoteMgr g_vote_mgr;

void HandleVoteCommand(std::string_view vote_name, std::string_view vote_arg, rf::Player* sender)
{
    if (GetPlayerAdditionalData(sender).is_browser) {
        SendChatLinePacket("Browsers are not allowed to vote!", sender);
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
        g_vote_mgr.AddPlayerVote(true, sender);
    else if (vote_name == "no" || vote_name == "n")
        g_vote_mgr.AddPlayerVote(false, sender);
    else
        SendChatLinePacket("Unrecognized vote type!", sender);
}

void ServerVoteDoFrame()
{
    g_vote_mgr.DoFrame();
}
