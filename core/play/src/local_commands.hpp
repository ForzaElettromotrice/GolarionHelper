#pragma once

#include "golarion/play/protocol_manager.hpp"

namespace golarion::play
{
    class ProtocolManager::LocalCommands final
    {
    public:
        explicit LocalCommands(ProtocolManager &owner);
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
