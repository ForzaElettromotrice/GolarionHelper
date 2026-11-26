#pragma once

#include "golarion/play/server_protocol.hpp"
#include "golarion/play/resource_manager.hpp"
#include "golarion/play/permission_manager.hpp"
#include "golarion/play/session_data.hpp"
#include "golarion/play/session_view.hpp"
#include "golarion/play/sheet_manager.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace golarion::play
{
    class SessionServer final
    {
    public:
        explicit SessionServer(UserId hostId, std::optional<SessionCode> hostCode = std::nullopt, const std::filesystem::path &campaignDirectory = {});
        ~SessionServer();

        std::uint16_t startHosting(std::string_view bindAddress, std::uint16_t port);
        void stopHosting();
        void registerSessionInfo(std::string name);
        void setHostDisplayName(std::string displayName);

        HostSessionView toView() const;
        SessionData toData() const;
        UserToken issueHostConnectionToken();
        void add_command(std::string_view name, CommandRequirements requirements, ServerProtocol::CommandCallback callback);
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
        ResourceManager resourceManager_;
        ServerProtocol protocolManager_;
        PermissionManager permissionManager_;
        SheetManager sheetManager_;
    };
}
