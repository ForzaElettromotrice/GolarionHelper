#include "golarion/play/session_server.hpp"

#include <utility>

namespace golarion::play
{
    SessionServer::SessionServer(UserId hostId, std::optional<SessionCode> hostCode, const std::filesystem::path &campaignDirectory) : protocolManager_(resourceManager_, std::move(hostCode)), permissionManager_(resourceManager_, std::move(hostId)), sheetManager_(campaignDirectory)
    {
        permissionManager_.registerCommands();
        sheetManager_.registerCommands(protocolManager_);
    }

    SessionServer::~SessionServer()
    {
        protocolManager_.shutdown(); // Join all workers before permission providers are destroyed.
    }

    std::uint16_t SessionServer::startHosting(std::string_view bindAddress, std::uint16_t port)
    {
        return protocolManager_.startHosting(bindAddress, port);
    }

    void SessionServer::stopHosting()
    {
        protocolManager_.stopHosting();
    }

    void SessionServer::registerSessionInfo(std::string name)
    {
        add_command("session.info", Permission::Any,
            [name = std::move(name)](const CommandContext &, const nlohmann::json &payload)
            {
                if (!payload.empty()) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "session.info requires an empty payload");
                return nlohmann::json{{"name", name}};
            });
    }

    void SessionServer::setHostDisplayName(std::string displayName)
    {
        protocolManager_.setHostDisplayName(std::move(displayName));
    }

    HostSessionView SessionServer::toView() const
    {
        auto hosting = protocolManager_.getHostingSnapshot();
        return {protocolManager_.hostCode(), hosting.listeningPort, std::move(hosting.participants)};
    }

    SessionData SessionServer::toData() const
    {
        return {
            .code = protocolManager_.hostCode(),
            .identities = protocolManager_.toIdentityData()
        };
    }

    UserToken SessionServer::issueHostConnectionToken() { return protocolManager_.issueHostConnectionToken(); }
    void SessionServer::add_command(std::string_view name, CommandRequirements requirements, ServerProtocol::CommandCallback callback) { protocolManager_.add_command(name, requirements, std::move(callback)); }
    bool SessionServer::sendEvent(const UserId &userId, std::string_view name, const nlohmann::json &payload) { return protocolManager_.sendEvent(userId, name, payload); }

    std::string SessionServer::getSessionCode() const
    {
        return protocolManager_.getSessionCode();
    }

    void SessionServer::registerUser(UserId userId)
    {
        permissionManager_.registerUser(std::move(userId));
    }

    std::string SessionServer::issueUserToken(const UserId &userId)
    {
        return protocolManager_.issueUserToken(userId);
    }

    void SessionServer::saveIdentityState(const std::filesystem::path &path)
    {
        protocolManager_.saveIdentityState(path);
    }

    void SessionServer::loadIdentityState(const std::filesystem::path &path)
    {
        protocolManager_.loadIdentityState(path);
    }

    ConnectionId SessionServer::registerConnection()
    {
        return protocolManager_.registerConnection();
    }

    std::optional<ConnectionId> SessionServer::bindUser(ConnectionId connectionId, const UserId &userId)
    {
        return protocolManager_.bindUser(connectionId, userId);
    }

    CommandContext SessionServer::makeContext(ConnectionId connectionId) const
    {
        return protocolManager_.makeContext(connectionId);
    }

    bool SessionServer::closeConnection(ConnectionId connectionId)
    {
        return protocolManager_.closeConnection(connectionId);
    }
}
