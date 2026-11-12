#include "admission.hpp"
#include "identity_storage.hpp"
#include "user_tokens.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
    using namespace golarion::play;
    using Json = nlohmann::json;

    bool validRequestId(const std::optional<std::string> &requestId)
    {
        return requestId && !requestId->empty() && requestId->size() <= 128 && requestId->find_first_not_of(" \t\r\n\f\v") != std::string::npos
            && std::none_of(requestId->begin(), requestId->end(), [](unsigned char value) { return value < 32 || value == 127; });
    }

    bool validAdmissionPayload(const Json &payload, bool resume)
    {
        return payload.is_object() && payload.size() == (resume ? 2u : 1u)
            && payload.contains("sessionCode") && payload.at("sessionCode").is_string()
            && (!resume || (payload.contains("userToken") && payload.at("userToken").is_string()));
    }

    std::string_view roleName(Role role)
    {
        switch (role)
        {
            case Role::Any: throw std::invalid_argument("Any is not an admitted participant role");
            case Role::Spectator: return "spectator";
            case Role::Player: return "player";
            case Role::Master: return "master";
            case Role::Host: return "host";
        }
        throw std::invalid_argument("unknown admission role");
    }
}

namespace golarion::play
{
    detail::ProtocolMessage ProtocolManager::makeRejection(const std::optional<std::string> &requestId, ProtocolErrorCode error)
    {
        std::string_view code;
        std::string_view message;
        switch (error)
        {
            case ProtocolErrorCode::InvalidRequest: code = "INVALID_REQUEST"; message = "Messaggio non valido."; break;
            case ProtocolErrorCode::InvalidSessionCode: code = "INVALID_SESSION_CODE"; message = "Codice della sessione non valido."; break;
            case ProtocolErrorCode::InvalidUserToken: code = "INVALID_USER_TOKEN"; message = "Token utente non valido."; break;
            case ProtocolErrorCode::NotAdmitted: code = "NOT_ADMITTED"; message = "Devi prima entrare nella sessione."; break;
            case ProtocolErrorCode::AlreadyAdmitted: code = "ALREADY_ADMITTED"; message = "Questa connessione è già associata a un utente."; break;
            case ProtocolErrorCode::AdmissionFailed: code = "ADMISSION_FAILED"; message = "Impossibile completare l'ingresso nella sessione."; break;
            case ProtocolErrorCode::UnknownCommand: code = "UNKNOWN_COMMAND"; message = "Comando non riconosciuto."; break;
            case ProtocolErrorCode::Forbidden: code = "FORBIDDEN"; message = "Non hai i permessi per eseguire questo comando."; break;
            case ProtocolErrorCode::CommandFailed: code = "COMMAND_FAILED"; message = "Impossibile completare il comando."; break;
        }
        return {"request.rejected", requestId, Json{{"code", code}, {"message", message}}};
    }

    ProtocolManager::MessageResult ProtocolManager::handleMessage(ConnectionId connectionId, const detail::ProtocolMessage &message)
    {
        const std::lock_guard executionLock(executionMutex_);
        if (!validRequestId(message.requestId)) return {makeRejection(std::nullopt, ProtocolErrorCode::InvalidRequest), true};
        return handleCommand(connectionId, message);
    }

    ProtocolManager::MessageResult ProtocolManager::handleCommand(ConnectionId connectionId, const detail::ProtocolMessage &message)
    {
        const std::lock_guard executionLock(executionMutex_);
        bool closeOnFailure = true;
        try
        {
            const auto context = makeContext(connectionId);
            closeOnFailure = !context.userId.has_value();
            return {makeCommandReply(message.requestId, dispatch(message.type, context, message.payload)), false};
        }
        catch (const ProtocolError &error)
        {
            return {makeRejection(message.requestId, error.code()), closeOnFailure || error.code() == ProtocolErrorCode::NotAdmitted};
        }
        catch (const std::invalid_argument &)
        {
            return {makeRejection(message.requestId, ProtocolErrorCode::InvalidRequest), closeOnFailure};
        }
        catch (const Json::exception &)
        {
            return {makeRejection(message.requestId, ProtocolErrorCode::InvalidRequest), closeOnFailure};
        }
        catch (...)
        {
            return {makeRejection(message.requestId, ProtocolErrorCode::CommandFailed), closeOnFailure};
        }
    }

    void ProtocolManager::registerAdmissionCommands()
    {
        add_command("session.join", Role::Any, [this](const CommandContext &context, const Json &payload)
        {
            const auto connectionId = requirePendingAdmission(context, payload, false);
            try { return makeAdmissionResult(admitNewUser(connectionId)); }
            catch (const ProtocolError &) { throw; }
            catch (...) { throw ProtocolError(ProtocolErrorCode::AdmissionFailed, "cannot create and persist the admitted identity"); }
        });
        add_command("session.resume", Role::Any, [this](const CommandContext &context, const Json &payload)
        {
            const auto connectionId = requirePendingAdmission(context, payload, true);
            try
            {
                const auto result = resumeUser(connectionId, payload.at("userToken").get_ref<const std::string &>());
                if (!result) throw ProtocolError(ProtocolErrorCode::InvalidUserToken, "user token is malformed or unknown");
                return makeAdmissionResult(*result);
            }
            catch (const ProtocolError &) { throw; }
            catch (...) { throw ProtocolError(ProtocolErrorCode::AdmissionFailed, "cannot resume the admitted identity"); }
        });
    }

    ConnectionId ProtocolManager::requirePendingAdmission(const CommandContext &context, const Json &payload, bool resume) const
    {
        if (!context.connectionId) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "admission requires a network connection");
        if (context.userId) throw ProtocolError(ProtocolErrorCode::AlreadyAdmitted, "connection already has an admitted identity");
        if (!validAdmissionPayload(payload, resume)) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "invalid admission payload");
        if (!verifySessionCode(payload.at("sessionCode").get_ref<const std::string &>()))
        {
            throw ProtocolError(ProtocolErrorCode::InvalidSessionCode, "session code does not match");
        }
        return *context.connectionId;
    }

    detail::ProtocolMessage ProtocolManager::makeCommandReply(const std::optional<std::string> &requestId, Json result)
    {
        return {"request.accepted", requestId, Json{{"result", std::move(result)}}};
    }

    ProtocolManager::AdmissionResult ProtocolManager::admitNewUser(ConnectionId connectionId)
    {
        const std::lock_guard identityLock(identityMutex_);
        if (!identityArchivePath_) throw std::logic_error("save or load an identity archive before admitting users");
        const std::lock_guard connectionsLock(connectionsMutex_);
        const auto connection = connections_.find(connectionId);
        if (connection == connections_.end() || connection->second) throw std::invalid_argument("admission requires a pending connection");

        const auto users = resources_.invoke<RoleManager::ToSaveDataSignature>("roles.toSaveData");
        UserId userId;
        for (int attempt = 0; attempt < 32; ++attempt)
        {
            // Generate the public ID independently; never expose the credential as a UserId.
            auto candidate = "user-" + UserToken::generate().value();
            if (activeConnections_.contains(candidate) || std::any_of(users.begin(), users.end(), [&candidate](const auto &user) { return user.userId == candidate; })) continue;
            userId = std::move(candidate);
            break;
        }
        if (userId.empty()) throw std::runtime_error("cannot generate an unused user ID");
        auto candidate = std::make_unique<detail::UserTokens>(*userTokens_);
        AdmissionResult result{userId, Role::Player, candidate->issue(userId)};
        const auto tokens = candidate->toSaveData();
        UserId boundUser = userId;
        activeConnections_.emplace(userId, connectionId);
        connection->second = std::move(boundUser);
        try
        {
            resources_.invoke<RoleManager::RegisterUserWithCommitSignature>("roles.registerUserWithCommit", userId, [this, &tokens](const std::vector<UserRoleSaveData> &savedUsers)
            {
                detail::writeIdentityState(*identityArchivePath_, IdentitySaveData{savedUsers, tokens});
            });
        }
        catch (...)
        {
            connection->second.reset();
            activeConnections_.erase(userId);
            throw;
        }
        userTokens_.swap(candidate);
        return result;
    }

    std::optional<ProtocolManager::AdmissionResult> ProtocolManager::resumeUser(ConnectionId connectionId, std::string_view token)
    {
        const auto userId = resolveUserToken(token);
        if (!userId) return std::nullopt;
        AdmissionResult result{*userId, resources_.invoke<RoleManager::GetRoleSignature>("roles.getRole", *userId), std::nullopt};
        bindUser(connectionId, *userId);
        return result;
    }

    Json ProtocolManager::makeAdmissionResult(const AdmissionResult &result)
    {
        Json payload{{"userId", result.userId}, {"role", roleName(result.role)}};
        if (result.userToken) payload["userToken"] = *result.userToken;
        return payload;
    }
}
