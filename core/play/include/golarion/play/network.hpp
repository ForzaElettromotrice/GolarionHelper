#pragma once

#include "golarion/play/game.hpp"

#include <chrono>
#include <cstdint>
#include <memory>
#include <optional>
#include <string_view>

namespace golarion::play
{
    class GameHost final
    {
    public:
        GameHost(Game game, std::string_view bindAddress, std::uint16_t port);
        ~GameHost();
        GameHost(const GameHost &) = delete;
        GameHost &operator=(const GameHost &) = delete;

        std::uint16_t port() const;
        void poll(std::chrono::milliseconds timeout);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };

    class GameClient final
    {
    public:
        GameClient(std::string_view host, std::uint16_t port, std::string_view participantName);
        ~GameClient();
        GameClient(const GameClient &) = delete;
        GameClient &operator=(const GameClient &) = delete;

        std::optional<GameSnapshot> poll(std::chrono::milliseconds timeout);

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
