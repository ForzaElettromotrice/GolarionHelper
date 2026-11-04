#include "golarion/play/game.hpp"

#include <algorithm>
#include <cctype>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
    std::string validatedText(std::string_view text)
    {
        const auto whitespace = [](unsigned char value) { return std::isspace(value) != 0; };
        const auto begin = std::find_if_not(text.begin(), text.end(), whitespace);
        const auto end = std::find_if_not(text.rbegin(), text.rend(), whitespace).base();
        if (begin >= end || end - begin > 128 || std::any_of(begin, end, [](unsigned char value) { return value < 32 || value == 127; }))
        {
            throw std::invalid_argument("game names and IDs must contain 1 to 128 bytes without control characters");
        }
        return std::string(begin, end);
    }
}

namespace golarion::play
{
    Game::Game(std::string_view id, std::string_view name) : id_(validatedText(id)), name_(validatedText(name))
    {
    }

    Participant Game::connectParticipant(std::string_view name)
    {
        const std::string normalizedName = validatedText(name);
        if (nextParticipantId_ == std::numeric_limits<std::uint64_t>::max())
        {
            throw std::overflow_error("participant ID space is exhausted");
        }
        Participant participant{"participant." + std::to_string(nextParticipantId_), normalizedName};
        connectedParticipants_.emplace(participant.id, participant);
        ++nextParticipantId_;
        return participant;
    }

    void Game::disconnectParticipant(std::string_view participantId)
    {
        if (connectedParticipants_.erase(std::string(participantId)) == 0)
        {
            throw std::invalid_argument("participant is not connected: " + std::string(participantId));
        }
    }

    GameSnapshot Game::snapshot() const
    {
        std::vector<Participant> participants;
        participants.reserve(connectedParticipants_.size());
        for (const auto &[id, participant] : connectedParticipants_)
        {
            static_cast<void>(id);
            participants.push_back(participant);
        }
        return GameSnapshot{id_, name_, std::move(participants)};
    }
}
