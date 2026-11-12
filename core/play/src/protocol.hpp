#pragma once

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace golarion::play::detail
{
    inline constexpr std::size_t MaximumFrameBytes = 65536;
    inline constexpr std::size_t MaximumJsonNesting = 64;
    inline constexpr std::uint32_t JsonProtocolVersion = 2;

    struct ProtocolMessage
    {
        std::string type;
        std::optional<std::string> requestId = std::nullopt;
        nlohmann::json payload = nlohmann::json::object();
    };

    std::vector<std::uint8_t> encodeMessage(const ProtocolMessage &message);
    std::optional<ProtocolMessage> decodeMessage(std::vector<std::uint8_t> &buffer);

}
