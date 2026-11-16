#pragma once

#include "golarion/play/role_manager.hpp"
#include "golarion/play/user_token.hpp"

#include <optional>

namespace golarion::play
{
    enum class ClientConnectionState
    {
        Disconnected,
        Connecting,
        Admitting,
        Connected
    };

    struct ClientAdmission
    {
        UserId userId;
        Role role;
        std::optional<UserToken> userToken = std::nullopt;
    };

    struct ClientSnapshot
    {
        ClientConnectionState state = ClientConnectionState::Disconnected;
        std::optional<UserId> userId = std::nullopt;
        std::optional<Role> role = std::nullopt;
    };
}
