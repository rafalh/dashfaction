#include <string_view>
#include "../rf.h"
#include "server_internal.h"
#include "../commands.h"
#include <map>
#include <set>

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

void HandleVoteCommand(std::string_view vote_name, std::string_view vote_arg, rf::Player* sender)
{
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
