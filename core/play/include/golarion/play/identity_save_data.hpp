#pragma once

#include "golarion/play/permission_manager.hpp"
#include "golarion/play/user_token.hpp"

#include <vector>

namespace golarion::play
{
    struct UserTokenSaveData
    {
        UserId userId;
        UserToken::Digest tokenHash;
    };

    struct IdentitySaveData
    {
        UserId hostId;
        std::vector<UserPermissionsSaveData> users;
        std::vector<UserTokenSaveData> tokens;
    };
}
