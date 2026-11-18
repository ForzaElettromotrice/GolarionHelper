#pragma once

#include "golarion/play/permission_set.hpp"
#include "golarion/play/user_id.hpp"
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
        PermissionSet permissions;
        std::optional<UserToken> userToken = std::nullopt;
    };

    struct ClientSnapshot
    {
        ClientConnectionState state = ClientConnectionState::Disconnected;
        std::optional<UserId> userId = std::nullopt;
        std::optional<PermissionSet> permissions = std::nullopt;
    };
}
