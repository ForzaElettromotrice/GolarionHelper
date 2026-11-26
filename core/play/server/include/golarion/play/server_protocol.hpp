#pragma once

#include "golarion/play/participants.hpp"
#include "golarion/play/command_requirements.hpp"
#include "golarion/play/protocol_error.hpp"
#include "golarion/play/resource_manager.hpp"
#include "golarion/play/permission_manager.hpp"
#include "golarion/play/session_code.hpp"
#include "golarion/play/user_token.hpp"
#include "golarion/play/command_context.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <functional>
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
    struct IdentitySaveData;

    class ServerProtocol final
    {
    public:
        using CommandCallback = std::function<nlohmann::json(const CommandContext &, const nlohmann::json &)>;
        using AddCommandSignature = void(std::string_view, CommandRequirements, CommandCallback);
        using SendEventSignature = bool(const UserId &, std::string_view, const nlohmann::json &);

        explicit ServerProtocol(ResourceManager &resources, std::optional<SessionCode> hostCode = std::nullopt);
        ~ServerProtocol();
        ServerProtocol(const ServerProtocol &) = delete;
        ServerProtocol &operator=(const ServerProtocol &) = delete;
        ServerProtocol(ServerProtocol &&) = delete;
        ServerProtocol &operator=(ServerProtocol &&) = delete;

        void add_command(std::string_view name, CommandRequirements requirements, CommandCallback callback);
        nlohmann::json dispatch(std::string_view name, const CommandContext &context, const nlohmann::json &payload) const;
        UserToken issueHostConnectionToken();
        bool sendEvent(const UserId &userId, std::string_view name, const nlohmann::json &payload);

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
        friend class SessionServer;
        struct HostingSnapshot
        {
            std::optional<std::uint16_t> listeningPort;
            ParticipantsSnapshot participants;
        };

        const SessionCode &hostCode() const;
        HostingSnapshot getHostingSnapshot() const;
        IdentitySaveData toIdentityData() const;
        IdentitySaveData identityDataLocked() const;
        void setHostDisplayName(std::string displayName);
        bool commitDisplayNameLocked(const UserId &userId, std::string displayName);
        std::optional<ConnectionId> bindUser(ConnectionId connectionId, const UserId &userId, std::optional<std::string> displayName, bool alreadyPresent = false);
        ConnectionId registerConnection(bool loopback);
        class Hosting;
        struct MessageResult;
        struct AdmissionResult;

        MessageResult handleMessage(ConnectionId connectionId, const detail::ProtocolMessage &message);
        MessageResult handleCommand(ConnectionId connectionId, const detail::ProtocolMessage &message);
        void registerAdmissionCommands();
        void registerParticipantCommands();
        ParticipantsSnapshot getParticipants() const;
        void sendParticipantsSnapshot(const UserId &userId) noexcept;
        void publishParticipantsChanged() noexcept;
        void failParticipantsDelivery(std::optional<std::string_view> userId = std::nullopt) noexcept;
        std::optional<std::vector<std::uint8_t>> takeEvent(ConnectionId connectionId);
        bool eventQueueOverflowed(ConnectionId connectionId) const;
        ConnectionId requirePendingAdmission(const CommandContext &context, const nlohmann::json &payload, bool resume) const;
        AdmissionResult admitNewUser(ConnectionId connectionId, std::string displayName);
        std::optional<AdmissionResult> resumeUser(ConnectionId connectionId, std::string_view token, std::optional<std::string> displayName);
        static nlohmann::json makeAdmissionResult(const AdmissionResult &result);
        static detail::ProtocolMessage makeCommandReply(const std::optional<std::string> &requestId, nlohmann::json result);
        static detail::ProtocolMessage makeRejection(const std::optional<std::string> &requestId, ProtocolErrorCode code);
        void shutdown() noexcept;
        PermissionSet updateUserPermissions(const std::function<PermissionSet(const PermissionManager::PermissionsCommit &)> &mutation);

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
        const SessionCode sessionCode_;
        mutable std::recursive_mutex executionMutex_;
        std::unordered_map<std::string, Command> commands_;
        mutable std::mutex connectionsMutex_;
        struct Peer { std::optional<UserId> userId; bool loopback; };
        std::unordered_map<ConnectionId, Peer> connections_;
        std::unordered_map<UserId, ConnectionId> activeConnections_;
        ConnectionId nextConnectionId_ = 1;
        std::uint64_t participantsRevision_ = 0; // Protected by executionMutex_; no separate participant registry.
        mutable std::mutex eventsMutex_;
        std::unordered_map<ConnectionId, EventQueue> events_;
        bool acceptingEvents_ = false;
        mutable std::mutex hostingMutex_;
        std::unique_ptr<Hosting> hosting_;
        std::optional<UserToken> hostConnectionToken_; // Protected by executionMutex_; never archived.
        mutable std::mutex identityMutex_;
        std::unique_ptr<detail::UserTokens> userTokens_;
        std::optional<std::filesystem::path> identityArchivePath_;
    };
}
