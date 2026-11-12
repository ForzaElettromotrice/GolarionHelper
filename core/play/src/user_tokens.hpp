#pragma once

#include "golarion/play/identity_save_data.hpp"

#include <functional>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_map>

namespace golarion::play::detail
{
    class UserTokens final
    {
    public:
        using Generator = std::function<UserToken()>;

        std::string issue(const UserId &userId, const Generator &generator = UserToken::generate);
        std::optional<UserId> resolve(std::string_view value) const;
        std::vector<UserTokenSaveData> toSaveData() const;
        static UserTokens fromSaveData(const std::vector<UserTokenSaveData> &data);

    private:
        bool containsHash(const UserToken::Digest &hash) const;

        std::unordered_map<UserId, UserToken::Digest> tokens_;
    };
}
