#pragma once

#include "golarion/play/user_id.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace golarion::play
{
    // A public projection of a participant, independent of its transport connection.
    struct ParticipantInfo
    {
        UserId userId;
        bool isHost = false;
        std::string displayName;

        bool operator==(const ParticipantInfo &) const = default;
    };

    struct ParticipantsSnapshot
    {
        std::uint64_t revision = 0;
        std::vector<ParticipantInfo> participants;

        bool operator==(const ParticipantsSnapshot &) const = default;
    };
}
