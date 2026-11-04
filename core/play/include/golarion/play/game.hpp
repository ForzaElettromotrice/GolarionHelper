#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace golarion::play
{
    struct Participant
    {
        std::string id;
        std::string name;
    };

    struct GameSnapshot
    {
        std::string id;
        std::string name;
        std::vector<Participant> connectedParticipants;
    };

    class Game final
    {
    public:
        Game(std::string_view id, std::string_view name);
        Participant connectParticipant(std::string_view name);
        void disconnectParticipant(std::string_view participantId);
        GameSnapshot snapshot() const;

    private:
        std::string id_;
        std::string name_;
        std::uint64_t nextParticipantId_ = 1;
        std::map<std::string, Participant> connectedParticipants_;
    };
}
