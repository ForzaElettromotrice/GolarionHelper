#pragma once

#include "golarion/play/protocol_manager.hpp"
#include "golarion/play/resource_manager.hpp"
#include "golarion/play/role_manager.hpp"

#include <cstdint>
#include <filesystem>
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

        std::uint16_t startHosting(std::string_view bindAddress, std::uint16_t port);
        void stopHosting();

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
        ResourceManager resourceManager_;
        ProtocolManager protocolManager_;
        RoleManager roleManager_;
    };
}
