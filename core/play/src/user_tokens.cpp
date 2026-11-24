#include "user_tokens.hpp"

#include <algorithm>
#include <stdexcept>

namespace golarion::play::detail
{
    bool UserTokens::containsToken(std::string_view token) const
    {
        return std::any_of(tokens_.begin(), tokens_.end(), [token](const auto &entry)
        {
            return entry.second == token;
        });
    }

    std::string UserTokens::issue(const UserId &userId, const Generator &generator)
    {
        if (tokens_.contains(userId)) throw std::invalid_argument("user already has a token");
        for (int attempt = 0; attempt < 32; ++attempt)
        {
            auto token = generator().value();
            if (containsToken(token)) continue;
            tokens_.emplace(userId, token);
            return token;
        }
        throw std::runtime_error("cannot generate an unused user token");
    }

    std::optional<UserId> UserTokens::resolve(std::string_view value) const
    {
        for (const auto &[userId, stored] : tokens_)
        {
            if (stored == value) return userId;
        }
        return std::nullopt;
    }

    std::vector<UserTokenSaveData> UserTokens::toSaveData() const
    {
        std::vector<UserTokenSaveData> result;
        result.reserve(tokens_.size());
        for (const auto &[userId, token] : tokens_) result.push_back({userId, token});
        std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) { return left.userId < right.userId; });
        return result;
    }

    UserTokens UserTokens::fromSaveData(const std::vector<UserTokenSaveData> &data)
    {
        UserTokens result;
        for (const auto &entry : data)
        {
            const auto token = UserToken(entry.token).value();
            if (result.containsToken(token)) throw std::invalid_argument("duplicate token in identity data");
            if (!result.tokens_.emplace(entry.userId, token).second) throw std::invalid_argument("duplicate token user ID in identity data");
        }
        return result;
    }
}
