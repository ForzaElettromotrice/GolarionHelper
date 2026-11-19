#pragma once

#include "golarion/play/protocol_manager.hpp"
#include "golarion/play/resource_manager.hpp"
#include "golarion/play/permission_manager.hpp"

#include <cstdint>
#include <filesystem>
#include <future>
#include <optional>
#include <string>
#include <string_view>

namespace golarion::play
{
    class Session final
    {
    public:
        explicit Session(UserId hostId, std::optional<SessionCode> hostCode = std::nullopt);
        ~Session();

        static Session createHost(UserId hostId);
        static Session createClient();

        std::uint16_t startHosting(std::string_view bindAddress, std::uint16_t port);
        void stopHosting();
        void registerSessionInfo(std::string name);

        std::future<ClientAdmission> joinSession(std::string ip, std::uint16_t port, SessionCode code);
        std::future<ClientAdmission> resumeSession(std::string ip, std::uint16_t port, SessionCode code, UserToken token);
        void disconnectFromHost();
        ClientSnapshot getClientState() const;
        std::future<nlohmann::json> sendCommand(std::string name, nlohmann::json payload);
        void add_command(std::string_view name, CommandRequirements requirements, ProtocolManager::CommandCallback callback);
        void add_command(std::string_view name, ProtocolManager::CommandCallback callback);
        bool sendEvent(const UserId &userId, std::string_view name, const nlohmann::json &payload);

        std::string getSessionCode() const;

        void registerUser(UserId userId);
        std::string issueUserToken(const UserId &userId);
        void saveIdentityState(const std::filesystem::path &path);
        void loadIdentityState(const std::filesystem::path &path);

        ConnectionId registerConnection();
        std::optional<ConnectionId> bindUser(ConnectionId connectionId, const UserId &userId);
        CommandContext makeContext(ConnectionId connectionId) const;
        bool closeConnection(ConnectionId connectionId);

    private:
        Session();

        ResourceManager resourceManager_;
        ProtocolManager protocolManager_;
        std::optional<PermissionManager> permissionManager_;
    };
}
