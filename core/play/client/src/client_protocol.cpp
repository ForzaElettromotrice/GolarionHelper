#include "golarion/play/client_protocol.hpp"
#include "golarion/play/display_name.hpp"
#include "client_connection.hpp"
#include "dispatch_support.hpp"
#include "participants_json.hpp"
#include "permission_json.hpp"

namespace golarion::play
{
    ClientProtocol::ClientProtocol(ResourceManager &resources) : resources_(resources)
    {
        registerClientEvents();
        resources_.registerFeature<OnEventSignature>("protocol.on_event", [this](std::string_view name, EventCallback callback) { on_event(name, std::move(callback)); });
    }

    ClientProtocol::~ClientProtocol()
    {
        try { disconnectFromHost(); } catch (...) {}
        try { resources_.unregisterFeature("protocol.on_event"); } catch (...) {}
    }

    std::future<nlohmann::json> ClientProtocol::sendCommand(std::string name, nlohmann::json payload)
    {
        if (detail::DispatchScope<ClientProtocol>::contains(*this)) throw std::logic_error("sendCommand is not allowed from a command callback");
        name = detail::normalizedName(name);
        if (!payload.is_object()) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "command payload must be a JSON object");
        const std::lock_guard lock(connectionMutex_);
        if (!connection_) throw std::logic_error("client is not connected");
        if (nextRequestId_ == 0) throw std::overflow_error("client request ID space is exhausted");
        return connection_->sendCommand(std::move(name), std::move(payload), "client-" + std::to_string(nextRequestId_++));
    }

    void ClientProtocol::on_event(std::string_view name, EventCallback callback)
    {
        const std::lock_guard lock(executionMutex_);
        const auto key = detail::normalizedName(name);
        if (key == "request.accepted" || key == "request.rejected") throw std::invalid_argument("reply names are reserved");
        if (!callback) throw std::invalid_argument("event callback must not be empty");
        if (!events_.emplace(key, std::move(callback)).second) throw std::invalid_argument("event is already registered: " + key);
    }

    void ClientProtocol::dispatchEvent(std::string_view name, const CommandContext &context, const nlohmann::json &payload) const
    {
        const std::lock_guard lock(executionMutex_);
        const auto key = detail::normalizedName(name);
        if (!payload.is_object()) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "event payload must be a JSON object");
        const auto connection = activeConnection_.lock();
        if (!connection || !connection->acceptsContext(context)) throw ProtocolError(ProtocolErrorCode::NotAdmitted, "notification must come from the currently admitted host connection");
        const auto event = events_.find(key);
        if (event == events_.end()) throw ProtocolError(ProtocolErrorCode::UnknownCommand, "event is not registered: " + key);
        const detail::DispatchScope scope(*this);
        event->second(context, payload);
    }

    void ClientProtocol::registerClientEvents()
    {
        on_event("session.participantsChanged", [this](const CommandContext &, const nlohmann::json &payload)
        {
            try { activeConnection_.lock()->updateParticipants(detail::parseParticipants(payload)); }
            catch (const nlohmann::json::exception &) { throw ProtocolError(ProtocolErrorCode::InvalidRequest, "invalid participant notification"); }
            catch (const std::invalid_argument &) { throw ProtocolError(ProtocolErrorCode::InvalidRequest, "invalid participant notification"); }
        });
        on_event("permissions.changed", [this](const CommandContext &, const nlohmann::json &payload)
        {
            try
            {
                const auto connection = activeConnection_.lock(); // dispatch already holds executionMutex_.
                const auto state = connection->snapshot();
                if (payload.size() != 2 || !payload.at("userId").is_string() || payload.at("userId") != *state.userId) throw std::invalid_argument("permission notification targets another user");
                connection->updatePermissions(detail::parsePermissions(payload.at("permissions")));
            }
            catch (const nlohmann::json::exception &) { throw ProtocolError(ProtocolErrorCode::InvalidRequest, "invalid permission notification"); }
            catch (const std::invalid_argument &) { throw ProtocolError(ProtocolErrorCode::InvalidRequest, "invalid permission notification"); }
        });
    }

    std::future<ClientAdmission> ClientProtocol::joinSession(std::string ip, std::uint16_t port, SessionCode code, std::string displayName)
    {
        return startClient(std::move(ip), port, std::move(code), std::nullopt, std::move(displayName));
    }

    std::future<ClientAdmission> ClientProtocol::resumeSession(std::string ip, std::uint16_t port, SessionCode code, UserToken token, std::optional<std::string> displayName)
    {
        return startClient(std::move(ip), port, std::move(code), std::move(token), std::move(displayName));
    }

    std::future<ClientAdmission> ClientProtocol::startClient(std::string ip, std::uint16_t port, SessionCode code, std::optional<UserToken> token, std::optional<std::string> displayName)
    {
        detail::requireOutsideDispatch(*this);
        if (displayName) *displayName = normalizeDisplayName(*displayName);
        const std::lock_guard lock(connectionMutex_);
        if (connection_)
        {
            if (connection_->snapshot().state != ClientConnectionState::Disconnected) throw std::logic_error("client connection is already active");
            connection_->stop();
            connection_.reset();
        }
        if (nextRequestId_ == 0) throw std::overflow_error("client request ID space is exhausted");
        const std::lock_guard executionLock(executionMutex_);
        auto connection = std::make_shared<ClientConnection>(*this, nextRequestId_, std::move(ip), port, std::move(code), std::move(token), std::move(displayName), "client-" + std::to_string(nextRequestId_));
        auto result = connection->takeResult();
        connection_ = std::move(connection);
        activeConnection_ = connection_;
        ++nextRequestId_;
        try { connection_->start(); }
        catch (...)
        {
            activeConnection_.reset();
            connection_.reset();
            throw;
        }
        return result;
    }

    void ClientProtocol::disconnectFromHost()
    {
        detail::requireOutsideDispatch(*this);
        const std::lock_guard lock(connectionMutex_);
        if (connection_) connection_->stop();
        const std::lock_guard executionLock(executionMutex_);
        activeConnection_.reset();
        connection_.reset();
    }

    ClientSnapshot ClientProtocol::getClientState() const
    {
        const std::lock_guard lock(executionMutex_);
        const auto connection = activeConnection_.lock();
        return connection ? connection->snapshot() : ClientSnapshot{};
    }
}
