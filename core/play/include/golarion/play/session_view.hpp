#pragma once

#include "golarion/play/client_state.hpp"
#include "golarion/play/session_code.hpp"
#include "golarion/play/session_mode.hpp"

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

    struct SessionView
    {
        SessionMode mode;
        std::optional<HostSessionView> host;
        std::optional<ClientSnapshot> client;
    };
}
