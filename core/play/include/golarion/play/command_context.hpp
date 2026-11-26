#pragma once

#include "golarion/play/user_id.hpp"

#include <cstdint>
#include <optional>

namespace golarion::play
{
    using ConnectionId = std::uint64_t;

    struct CommandContext
    {
        std::optional<UserId> userId = std::nullopt;
        std::optional<ConnectionId> connectionId = std::nullopt;
    };
}
