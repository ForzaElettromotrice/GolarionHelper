#pragma once

#include "golarion/play/protocol_manager.hpp"
#include "golarion/play/resource_manager.hpp"
#include "golarion/play/role_manager.hpp"

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
        explicit Session(UserId hostId);
        ~Session();

        static Session createHost(UserId hostId);
        static Session createClient();

        std::uint16_t startHosting(std::string_view bindAddress, std::uint16_t port);
        void stopHosting();

        std::future<ClientAdmission> joinSession(std::string ip, std::uint16_t port, SessionCode code);
        std::future<ClientAdmission> resumeSession(std::string ip, std::uint16_t port, SessionCode code, UserToken token);
        void disconnectFromHost();
        ClientSnapshot getClientState() const;

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
        std::optional<RoleManager> roleManager_;
    };
}
