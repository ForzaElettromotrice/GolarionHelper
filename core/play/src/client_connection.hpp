#pragma once

#include "golarion/play/protocol_manager.hpp"

#include <future>
#include <memory>
#include <optional>
#include <string>

namespace golarion::play
{
    class ProtocolManager::ClientConnection final
    {
    public:
        ClientConnection(std::string ip, std::uint16_t port, SessionCode code, std::optional<UserToken> token, std::string requestId);
        ~ClientConnection();
        ClientConnection(const ClientConnection &) = delete;
        ClientConnection &operator=(const ClientConnection &) = delete;

        std::future<ClientAdmission> takeResult();
        ClientSnapshot snapshot() const;
        void stop();

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
