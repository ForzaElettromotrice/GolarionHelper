#pragma once

#include "golarion/play/server_protocol.hpp"
#include "protocol.hpp"

namespace golarion::play
{
    struct ServerProtocol::MessageResult
    {
        detail::ProtocolMessage reply;
        bool closeAfterReply = false;
    };

    struct ServerProtocol::AdmissionResult
    {
        UserId userId;
        PermissionSet permissions;
        std::optional<std::string> userToken;
    };
}
