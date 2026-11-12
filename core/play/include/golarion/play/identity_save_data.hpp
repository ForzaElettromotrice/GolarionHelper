#pragma once

#include "golarion/play/role_manager.hpp"
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
        std::vector<UserRoleSaveData> users;
        std::vector<UserTokenSaveData> tokens;
    };
}
