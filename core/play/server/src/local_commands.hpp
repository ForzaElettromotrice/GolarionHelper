#pragma once

#include "golarion/play/server_protocol.hpp"

namespace golarion::play
{
    class ServerProtocol::LocalCommands final
    {
    public:
        explicit LocalCommands(ServerProtocol &owner);
        ~LocalCommands();
        LocalCommands(const LocalCommands &) = delete;
        LocalCommands &operator=(const LocalCommands &) = delete;

        std::future<nlohmann::json> submit(std::string name, nlohmann::json payload);
        void stop();

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
