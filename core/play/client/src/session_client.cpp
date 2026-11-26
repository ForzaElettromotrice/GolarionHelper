#include "golarion/play/session_client.hpp"

#include <utility>

namespace golarion::play
{
    SessionClient::SessionClient() : protocolManager_(resourceManager_) {}

    std::future<ClientAdmission> SessionClient::joinSession(std::string ip, std::uint16_t port, SessionCode code, std::string displayName)
    {
        return protocolManager_.joinSession(std::move(ip), port, std::move(code), std::move(displayName));
    }

    std::future<ClientAdmission> SessionClient::resumeSession(std::string ip, std::uint16_t port, SessionCode code, UserToken token, std::optional<std::string> displayName)
    {
        return protocolManager_.resumeSession(std::move(ip), port, std::move(code), std::move(token), std::move(displayName));
    }

    void SessionClient::disconnectFromHost() { protocolManager_.disconnectFromHost(); }
    ClientSnapshot SessionClient::getClientState() const { return protocolManager_.getClientState(); }

    ClientSnapshot SessionClient::toView() const
    {
        return getClientState();
    }

    std::future<nlohmann::json> SessionClient::sendCommand(std::string name, nlohmann::json payload) { return protocolManager_.sendCommand(std::move(name), std::move(payload)); }
    void SessionClient::on_event(std::string_view name, ClientProtocol::EventCallback callback) { protocolManager_.on_event(name, std::move(callback)); }
}
