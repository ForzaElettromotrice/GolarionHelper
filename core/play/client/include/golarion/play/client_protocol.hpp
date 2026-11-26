#pragma once

#include "golarion/play/client_state.hpp"
#include "golarion/play/command_context.hpp"
#include "golarion/play/protocol_error.hpp"
#include "golarion/play/resource_manager.hpp"
#include "golarion/play/session_code.hpp"

#include <nlohmann/json.hpp>
#include <future>
#include <mutex>
#include <unordered_map>

namespace golarion::play
{
    class ClientProtocol final
    {
    public:
        using EventCallback = std::function<void(const CommandContext &, const nlohmann::json &)>;
        using OnEventSignature = void(std::string_view, EventCallback);

        explicit ClientProtocol(ResourceManager &resources);
        ~ClientProtocol();
        ClientProtocol(const ClientProtocol &) = delete;
        ClientProtocol &operator=(const ClientProtocol &) = delete;

        std::future<ClientAdmission> joinSession(std::string ip, std::uint16_t port, SessionCode code, std::string displayName = "Giocatore");
        std::future<ClientAdmission> resumeSession(std::string ip, std::uint16_t port, SessionCode code, UserToken token, std::optional<std::string> displayName = std::nullopt);
        void disconnectFromHost();
        ClientSnapshot getClientState() const;

        std::future<nlohmann::json> sendCommand(std::string name, nlohmann::json payload);
        void on_event(std::string_view name, EventCallback callback);
        void dispatchEvent(std::string_view name, const CommandContext &context, const nlohmann::json &payload) const;

    private:
        class ClientConnection;
        void registerClientEvents();
        std::future<ClientAdmission> startClient(std::string ip, std::uint16_t port, SessionCode code, std::optional<UserToken> token, std::optional<std::string> displayName);

        ResourceManager &resources_;
        mutable std::recursive_mutex executionMutex_;
        std::unordered_map<std::string, EventCallback> events_;
        std::mutex connectionMutex_;
        std::shared_ptr<ClientConnection> connection_;
        std::weak_ptr<ClientConnection> activeConnection_; // Protected by executionMutex_; never joins a worker.
        std::uint64_t nextRequestId_ = 1;
    };
}
