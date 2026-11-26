#pragma once

#include "golarion/play/participants.hpp"
#include "golarion/play/session_code.hpp"

#include <cstdint>
#include <optional>

namespace golarion::play
{
    struct HostSessionView
    {
        SessionCode code;
        std::optional<std::uint16_t> listeningPort;
        ParticipantsSnapshot participants;
    };
}
