#pragma once

#include "golarion/play/permission_manager.hpp"

#include <functional>
#include <string_view>

namespace golarion::play::detail
{
    using UserPermissionsMutation = std::function<PermissionSet(const PermissionManager::PermissionsCommit &)>;
    using UpdateUserPermissionsSignature = PermissionSet(UserPermissionsMutation);
    inline constexpr std::string_view UpdateUserPermissionsFeature = "identity.updateUserPermissions";
}
