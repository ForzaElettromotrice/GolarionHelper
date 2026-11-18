#pragma once

#include "golarion/play/client_state.hpp"
#include "golarion/play/command_requirements.hpp"
#include "golarion/play/protocol_error.hpp"
#include "golarion/play/resource_manager.hpp"
#include "golarion/play/permission_manager.hpp"
#include "golarion/play/session_code.hpp"
#include "golarion/play/session_mode.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
#include <future>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

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
        using AddCommandSignature = void(std::string_view, CommandRequirements, CommandCallback);
        using AddClientCommandSignature = void(std::string_view, CommandCallback);
        using SendEventSignature = bool(const UserId &, std::string_view, const nlohmann::json &);

        explicit ProtocolManager(ResourceManager &resources, SessionMode mode = SessionMode::Host);
        ~ProtocolManager();
        ProtocolManager(const ProtocolManager &) = delete;
        ProtocolManager &operator=(const ProtocolManager &) = delete;
        ProtocolManager(ProtocolManager &&) = delete;
        ProtocolManager &operator=(ProtocolManager &&) = delete;

        void add_command(std::string_view name, CommandRequirements requirements, CommandCallback callback);
        void add_command(std::string_view name, CommandCallback callback);
        nlohmann::json dispatch(std::string_view name, const CommandContext &context, const nlohmann::json &payload) const;
        std::future<nlohmann::json> sendCommand(std::string name, nlohmann::json payload);
        bool sendEvent(const UserId &userId, std::string_view name, const nlohmann::json &payload);

        std::uint16_t startHosting(std::string_view bindAddress, std::uint16_t port);
        void stopHosting();

        std::future<ClientAdmission> joinSession(std::string ip, std::uint16_t port, SessionCode code);
        std::future<ClientAdmission> resumeSession(std::string ip, std::uint16_t port, SessionCode code, UserToken token);
        void disconnectFromHost();
        ClientSnapshot getClientState() const;

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
        friend class Session;
        class Hosting;
        class ClientConnection;
        class LocalCommands;
        struct MessageResult;
        struct AdmissionResult;

        MessageResult handleMessage(ConnectionId connectionId, const detail::ProtocolMessage &message);
        MessageResult handleCommand(ConnectionId connectionId, const detail::ProtocolMessage &message);
        void registerAdmissionCommands();
        void registerClientCommands();
        void registerCommand(std::string_view name, CommandRequirements requirements, CommandCallback callback);
        std::optional<std::vector<std::uint8_t>> takeEvent(ConnectionId connectionId);
        bool eventQueueOverflowed(ConnectionId connectionId) const;
        ConnectionId requirePendingAdmission(const CommandContext &context, const nlohmann::json &payload, bool resume) const;
        AdmissionResult admitNewUser(ConnectionId connectionId);
        std::optional<AdmissionResult> resumeUser(ConnectionId connectionId, std::string_view token);
        static nlohmann::json makeAdmissionResult(const AdmissionResult &result);
        static detail::ProtocolMessage makeCommandReply(const std::optional<std::string> &requestId, nlohmann::json result);
        static detail::ProtocolMessage makeRejection(const std::optional<std::string> &requestId, ProtocolErrorCode code);
        void requireHostMode() const;
        void requireClientMode() const;
        void shutdown() noexcept;
        PermissionSet updateUserPermissions(const std::function<PermissionSet(const PermissionManager::PermissionsCommit &)> &mutation);
        std::future<ClientAdmission> startClient(std::string ip, std::uint16_t port, SessionCode code, std::optional<UserToken> token);

        struct Command
        {
            CommandRequirements requirements;
            CommandCallback callback;
        };

        struct EventQueue
        {
            std::deque<std::vector<std::uint8_t>> frames;
            std::size_t bytes = 0;
            bool overflowed = false;
        };

        ResourceManager &resources_;
        const SessionMode mode_;
        const std::optional<SessionCode> sessionCode_;
        mutable std::recursive_mutex executionMutex_;
        std::unordered_map<std::string, Command> commands_;
        mutable std::mutex connectionsMutex_;
        std::unordered_map<ConnectionId, std::optional<UserId>> connections_;
        std::unordered_map<UserId, ConnectionId> activeConnections_;
        ConnectionId nextConnectionId_ = 1;
        mutable std::mutex eventsMutex_;
        std::unordered_map<ConnectionId, EventQueue> events_;
        bool acceptingEvents_ = false;
        std::mutex hostingMutex_;
        std::unique_ptr<Hosting> hosting_;
        std::mutex localCommandsMutex_;
        std::unique_ptr<LocalCommands> localCommands_;
        mutable std::mutex clientMutex_;
        std::shared_ptr<ClientConnection> client_;
        std::weak_ptr<ClientConnection> activeClient_; // Protected by executionMutex_; never joins a worker.
        std::uint64_t nextClientRequestId_ = 1;
        mutable std::mutex identityMutex_;
        std::unique_ptr<detail::UserTokens> userTokens_;
        std::optional<std::filesystem::path> identityArchivePath_;
    };
}
