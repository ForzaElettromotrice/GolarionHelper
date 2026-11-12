#pragma once

#include "golarion/play/protocol_manager.hpp"
#include "protocol.hpp"

namespace golarion::play
{
    struct ProtocolManager::MessageResult
    {
        detail::ProtocolMessage reply;
        bool closeAfterReply = false;
    };

    struct ProtocolManager::AdmissionResult
    {
        UserId userId;
        Role role;
        std::optional<std::string> userToken;
    };
}
