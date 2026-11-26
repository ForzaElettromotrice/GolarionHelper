#include "golarion/play/permission_manager.hpp"
#include "golarion/play/server_protocol.hpp"
#include "identity_transaction.hpp"
#include "permission_json.hpp"

#include <algorithm>
#include <initializer_list>
#include <string_view>
#include <utility>

namespace
{
    using namespace golarion::play;
    using Json = nlohmann::json;

    void requireFields(const Json &payload, std::initializer_list<std::string_view> fields)
    {
        if (!payload.is_object() || payload.size() != fields.size()) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "invalid permission command fields");
        for (const auto field : fields) if (!payload.contains(std::string(field))) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "missing permission command field");
    }

    UserId targetId(const Json &payload)
    {
        const auto &value = payload.at("targetUserId");
        if (!value.is_string()) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "targetUserId must be a string");
        auto result = value.get<UserId>();
        if (result.empty() || std::any_of(result.begin(), result.end(), [](unsigned char byte) { return byte <= 32 || byte == 127; })) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "invalid targetUserId");
        return result;
    }

    PermissionPreset presetValue(const Json &value)
    {
        if (!value.is_string()) throw ProtocolError(ProtocolErrorCode::InvalidRequest, "preset must be a string");
        const auto &name = value.get_ref<const std::string &>();
        if (name == "master") return PermissionPreset::Master;
        if (name == "player") return PermissionPreset::Player;
        if (name == "spectator") return PermissionPreset::Spectator;
        throw ProtocolError(ProtocolErrorCode::InvalidRequest, "unknown permission preset");
    }

    template<typename Action> Json resultFor(const UserId &target, Action action)
    {
        try { return Json{{"userId", target}, {"permissions", action().toMask()}}; }
        catch (const PermissionDenied &) { throw ProtocolError(ProtocolErrorCode::Forbidden, "permission operation is not authorized"); }
        catch (const ProtocolError &) { throw; }
        catch (const std::invalid_argument &) { throw ProtocolError(ProtocolErrorCode::InvalidRequest, "invalid permission operation"); }
    }

    PermissionSet changePermissions(ResourceManager &resources, const UserId &target, detail::UserPermissionsMutation mutation)
    {
        const auto previous = resources.invoke<PermissionManager::GetPermissionsSignature>("permissions.getPermissions", target);
        const auto current = resources.invoke<detail::UpdateUserPermissionsSignature>(detail::UpdateUserPermissionsFeature, std::move(mutation));
        if (current != previous)
        {
            // Persistence and the manager's swap have completed; delivery cannot roll them back.
            try { resources.invoke<ServerProtocol::SendEventSignature>("protocol.sendEvent", target, "permissions.changed", Json{{"userId", target}, {"permissions", current.toMask()}}); }
            catch (...) { } // An unavailable transport must not turn a committed mutation into failure.
        }
        return current;
    }
}

namespace golarion::play
{
    void PermissionManager::registerCommands()
    {
        auto &resources = resources_;
        resources.invoke<ServerProtocol::AddCommandSignature>("protocol.add_command", "permissions.get", Permission::Any, [&resources](const CommandContext &context, const Json &payload)
        {
            if (!payload.empty()) requireFields(payload, {"targetUserId"});
            const auto target = payload.empty() ? *context.userId : targetId(payload);
            if (target != *context.userId && !resources.invoke<HasPermissionSignature>("permissions.hasPermission", *context.userId, Permission::ChangePermissions)) throw ProtocolError(ProtocolErrorCode::Forbidden, "cannot inspect another participant's permissions");
            return resultFor(target, [&] { return resources.invoke<GetPermissionsSignature>("permissions.getPermissions", target); });
        });
        resources.invoke<ServerProtocol::AddCommandSignature>("protocol.add_command", "permissions.applyPreset", Permission::ChangePermissions, [&resources](const CommandContext &context, const Json &payload)
        {
            requireFields(payload, {"targetUserId", "preset"});
            const auto target = targetId(payload);
            const auto preset = presetValue(payload.at("preset"));
            return resultFor(target, [&]
            {
                return changePermissions(resources, target, [&](const PermissionsCommit &commit)
                {
                    return resources.invoke<ApplyPresetWithCommitSignature>("permissions.applyPresetWithCommit", *context.userId, target, preset, commit);
                });
            });
        });
        resources.invoke<ServerProtocol::AddCommandSignature>("protocol.add_command", "permissions.update", Permission::ChangePermissions, [&resources](const CommandContext &context, const Json &payload)
        {
            requireFields(payload, {"targetUserId", "grant", "revoke"});
            const auto target = targetId(payload);
            return resultFor(target, [&]
            {
                const auto grant = detail::parsePermissions(payload.at("grant"));
                const auto revoke = detail::parsePermissions(payload.at("revoke"));
                return changePermissions(resources, target, [&](const PermissionsCommit &commit)
                {
                    return resources.invoke<UpdatePermissionsWithCommitSignature>("permissions.updatePermissionsWithCommit", *context.userId, target, grant, revoke, commit);
                });
            });
        });
    }
}
