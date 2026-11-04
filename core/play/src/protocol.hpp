#pragma once

#include "golarion/play/game.hpp"

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <variant>
#include <vector>

namespace golarion::play::detail
{
    inline constexpr std::size_t MaximumFrameBytes = 65536;

    struct JoinRequest { std::string name; };
    struct ConnectionRejected { std::string reason; };
    using Message = std::variant<JoinRequest, GameSnapshot, ConnectionRejected>;

    std::vector<std::uint8_t> encodeJoin(std::string_view name);
    std::vector<std::uint8_t> encodeSnapshot(const GameSnapshot &snapshot);
    std::vector<std::uint8_t> encodeRejection(std::string_view reason);
    std::optional<Message> extractMessage(std::vector<std::uint8_t> &buffer);
}
