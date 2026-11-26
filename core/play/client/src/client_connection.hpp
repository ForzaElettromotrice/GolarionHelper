#pragma once

#include "golarion/play/client_protocol.hpp"

#include <future>
#include <memory>
#include <optional>
#include <string>

namespace golarion::play
{
    class ClientProtocol::ClientConnection final
    {
    public:
        ClientConnection(ClientProtocol &owner, ConnectionId connectionId, std::string ip, std::uint16_t port, SessionCode code, std::optional<UserToken> token, std::optional<std::string> displayName, std::string requestId);
        ~ClientConnection();
        ClientConnection(const ClientConnection &) = delete;
        ClientConnection &operator=(const ClientConnection &) = delete;

        std::future<ClientAdmission> takeResult();
        void start();
        ClientSnapshot snapshot() const;
        bool acceptsContext(const CommandContext &context) const;
        void updatePermissions(PermissionSet permissions);
        void updateParticipants(ParticipantsSnapshot participants);
        std::future<nlohmann::json> sendCommand(std::string name, nlohmann::json payload, std::string requestId);
        void stop();

    private:
        struct Impl;
        std::unique_ptr<Impl> impl_;
    };
}
