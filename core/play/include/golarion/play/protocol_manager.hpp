#pragma once

#include "golarion/play/protocol_error.hpp"
#include "golarion/play/resource_manager.hpp"
#include "golarion/play/role_manager.hpp"
#include "golarion/play/session_code.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace golarion::play
{
    namespace detail { class UserTokens; struct ProtocolMessage; }

    using ConnectionId = std::uint64_t;

    struct CommandContext
    {
        std::optional<UserId> userId = std::nullopt;
        std::optional<ConnectionId> connectionId = std::nullopt;
    };

    class ProtocolManager final
    {
    public:
        using CommandCallback = std::function<nlohmann::json(const CommandContext &, const nlohmann::json &)>;
        using AddCommandSignature = void(std::string_view, Role, CommandCallback);

        explicit ProtocolManager(ResourceManager &resources);
        ~ProtocolManager();
        ProtocolManager(const ProtocolManager &) = delete;
        ProtocolManager &operator=(const ProtocolManager &) = delete;
        ProtocolManager(ProtocolManager &&) = delete;
        ProtocolManager &operator=(ProtocolManager &&) = delete;

        void add_command(std::string_view name, Role minimumRole, CommandCallback callback);
        nlohmann::json dispatch(std::string_view name, const CommandContext &context, const nlohmann::json &payload) const;

        std::uint16_t startHosting(std::string_view bindAddress, std::uint16_t port);
        void stopHosting();

        std::string getSessionCode() const;
        bool verifySessionCode(std::string_view candidate) const noexcept;

        std::string issueUserToken(const UserId &userId);
        std::optional<UserId> resolveUserToken(std::string_view token) const;
        void saveIdentityState(const std::filesystem::path &path);
        void loadIdentityState(const std::filesystem::path &path);

        ConnectionId registerConnection();
        std::optional<ConnectionId> bindUser(ConnectionId connectionId, const UserId &userId);
        std::optional<UserId> getUserId(ConnectionId connectionId) const;
        CommandContext makeContext(ConnectionId connectionId) const;
        bool closeConnection(ConnectionId connectionId);

    private:
        class Hosting;
        struct MessageResult;
        struct AdmissionResult;

        MessageResult handleMessage(ConnectionId connectionId, const detail::ProtocolMessage &message);
        MessageResult handleCommand(ConnectionId connectionId, const detail::ProtocolMessage &message);
        void registerAdmissionCommands();
        ConnectionId requirePendingAdmission(const CommandContext &context, const nlohmann::json &payload, bool resume) const;
        AdmissionResult admitNewUser(ConnectionId connectionId);
        std::optional<AdmissionResult> resumeUser(ConnectionId connectionId, std::string_view token);
        static nlohmann::json makeAdmissionResult(const AdmissionResult &result);
        static detail::ProtocolMessage makeCommandReply(const std::optional<std::string> &requestId, nlohmann::json result);
        static detail::ProtocolMessage makeRejection(const std::optional<std::string> &requestId, ProtocolErrorCode code);

        struct Command
        {
            Role minimumRole;
            CommandCallback callback;
        };

        ResourceManager &resources_;
        const SessionCode sessionCode_ = SessionCode::generate();
        mutable std::recursive_mutex executionMutex_;
        std::unordered_map<std::string, Command> commands_;
        mutable std::mutex connectionsMutex_;
        std::unordered_map<ConnectionId, std::optional<UserId>> connections_;
        std::unordered_map<UserId, ConnectionId> activeConnections_;
        ConnectionId nextConnectionId_ = 1;
        std::mutex hostingMutex_;
        std::unique_ptr<Hosting> hosting_;
        mutable std::mutex identityMutex_;
        std::unique_ptr<detail::UserTokens> userTokens_;
        std::optional<std::filesystem::path> identityArchivePath_;
    };
}
