#pragma once

#include "golarion/play/permission_set.hpp"

namespace golarion::play
{
    enum class CommandAccess
    {
        Authenticated,
        Public
    };

    struct CommandRequirements
    {
        constexpr CommandRequirements() noexcept = default;
        constexpr CommandRequirements(Permission permission)
        {
            if (permission == Permission::NoAuth) access = CommandAccess::Public;
            else if (permission != Permission::Any) permissions = PermissionSet(permission);
        }
        constexpr CommandRequirements(PermissionSet required) noexcept : permissions(required) {}
        constexpr CommandRequirements(CommandAccess commandAccess, PermissionSet required = {}) noexcept : access(commandAccess), permissions(required) {}

        CommandAccess access = CommandAccess::Authenticated;
        PermissionSet permissions;
    };
}
