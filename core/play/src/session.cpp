#include "golarion/play/session.hpp"

#include <utility>
#include <stdexcept>

namespace golarion::play
{
    Session::Session(UserId hostId, std::optional<SessionCode> hostCode) : protocolManager_(resourceManager_, SessionMode::Host, hostCode), permissionManager_(std::in_place, resourceManager_, std::move(hostId))
    {
        permissionManager_->registerCommands();
    }

    Session::Session() : protocolManager_(resourceManager_, SessionMode::Client) {}

    Session Session::createHost(UserId hostId) { return Session(std::move(hostId)); }
    Session Session::createClient() { return Session(); }

    Session::~Session()
    {
        protocolManager_.shutdown(); // Join all workers before permission providers are destroyed.
    }

    std::uint16_t Session::startHosting(std::string_view bindAddress, std::uint16_t port)
    {
        return protocolManager_.startHosting(bindAddress, port);
    }

    void Session::stopHosting()
    {
        protocolManager_.stopHosting();
    }

    void Session::registerSessionInfo(std::string name)
    {
        add_command("session.info", CommandRequirements{},
            [name = std::move(name)](const CommandContext &, const nlohmann::json &payload)
            {
                if (!payload.empty()) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "session.info requires an empty payload");
                return nlohmann::json{{"name", name}};
            });
    }

    void Session::setHostDisplayName(std::string displayName)
    {
        protocolManager_.setHostDisplayName(std::move(displayName));
    }

    std::future<ClientAdmission> Session::joinSession(std::string ip, std::uint16_t port, SessionCode code, std::string displayName)
    {
        return protocolManager_.joinSession(std::move(ip), port, std::move(code), std::move(displayName));
    }

    std::future<ClientAdmission> Session::resumeSession(std::string ip, std::uint16_t port, SessionCode code, UserToken token, std::optional<std::string> displayName)
    {
        return protocolManager_.resumeSession(std::move(ip), port, std::move(code), std::move(token), std::move(displayName));
    }

    void Session::disconnectFromHost() { protocolManager_.disconnectFromHost(); }
    ClientSnapshot Session::getClientState() const { return protocolManager_.getClientState(); }

    SessionView Session::toView() const
    {
        if (!permissionManager_)
        {
            return {
                .mode = SessionMode::Client,
                .host = std::nullopt,
                .client = protocolManager_.getClientState()
            };
        }

        auto hosting = protocolManager_.getHostingSnapshot();
        return {
            .mode = SessionMode::Host,
            .host = HostSessionView{
                .code = protocolManager_.hostCode(),
                .listeningPort = hosting.listeningPort,
                .participants = std::move(hosting.participants)
            },
            .client = std::nullopt
        };
    }

    SessionData Session::toData() const
    {
        return {
            .code = protocolManager_.hostCode(),
            .identities = protocolManager_.toIdentityData()
        };
    }

    std::future<nlohmann::json> Session::sendCommand(std::string name, nlohmann::json payload) { return protocolManager_.sendCommand(std::move(name), std::move(payload)); }
    void Session::add_command(std::string_view name, CommandRequirements requirements, ProtocolManager::CommandCallback callback) { protocolManager_.add_command(name, requirements, std::move(callback)); }
    void Session::add_command(std::string_view name, ProtocolManager::CommandCallback callback) { protocolManager_.add_command(name, std::move(callback)); }
    bool Session::sendEvent(const UserId &userId, std::string_view name, const nlohmann::json &payload) { return protocolManager_.sendEvent(userId, name, payload); }

    std::string Session::getSessionCode() const
    {
        return protocolManager_.getSessionCode();
    }

    void Session::registerUser(UserId userId)
    {
        if (!permissionManager_) throw std::logic_error("user registration requires a host Session");
        permissionManager_->registerUser(std::move(userId));
    }

    std::string Session::issueUserToken(const UserId &userId)
    {
        return protocolManager_.issueUserToken(userId);
    }

    void Session::saveIdentityState(const std::filesystem::path &path)
    {
        protocolManager_.saveIdentityState(path);
    }

    void Session::loadIdentityState(const std::filesystem::path &path)
    {
        protocolManager_.loadIdentityState(path);
    }

    ConnectionId Session::registerConnection()
    {
        return protocolManager_.registerConnection();
    }

    std::optional<ConnectionId> Session::bindUser(ConnectionId connectionId, const UserId &userId)
    {
        return protocolManager_.bindUser(connectionId, userId);
    }

    CommandContext Session::makeContext(ConnectionId connectionId) const
    {
        return protocolManager_.makeContext(connectionId);
    }

    bool Session::closeConnection(ConnectionId connectionId)
    {
        return protocolManager_.closeConnection(connectionId);
    }
}
