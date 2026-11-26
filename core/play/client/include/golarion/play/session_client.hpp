#pragma once

#include "golarion/play/client_protocol.hpp"
#include "golarion/play/resource_manager.hpp"

#include <future>
#include <string>
#include <string_view>

namespace golarion::play
{
    class SessionClient final
    {
    public:
        SessionClient();

        std::future<ClientAdmission> joinSession(std::string ip, std::uint16_t port, SessionCode code, std::string displayName = "Giocatore");
        std::future<ClientAdmission> resumeSession(std::string ip, std::uint16_t port, SessionCode code, UserToken token, std::optional<std::string> displayName = std::nullopt);
        void disconnectFromHost();
        ClientSnapshot getClientState() const;
        ClientSnapshot toView() const;
        std::future<nlohmann::json> sendCommand(std::string name, nlohmann::json payload);
        void on_event(std::string_view name, ClientProtocol::EventCallback callback);

    private:
        ResourceManager resourceManager_;
        ClientProtocol protocolManager_;
    };
}
