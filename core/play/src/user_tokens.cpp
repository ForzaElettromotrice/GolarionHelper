#include "user_tokens.hpp"

#include <openssl/crypto.h>

#include <algorithm>
#include <stdexcept>

namespace golarion::play::detail
{
    bool UserTokens::containsHash(const UserToken::Digest &hash) const
    {
        return std::any_of(tokens_.begin(), tokens_.end(), [&hash](const auto &entry)
        {
            return CRYPTO_memcmp(hash.data(), entry.second.data(), hash.size()) == 0;
        });
    }

    std::string UserTokens::issue(const UserId &userId, const Generator &generator)
    {
        if (tokens_.contains(userId)) throw std::invalid_argument("user already has a token");
        for (int attempt = 0; attempt < 32; ++attempt)
        {
            const auto token = generator();
            const auto hash = token.digest();
            if (containsHash(hash)) continue;
            auto value = token.value();
            tokens_.emplace(userId, hash);
            return value;
        }
        throw std::runtime_error("cannot generate an unused user token");
    }

    std::optional<UserId> UserTokens::resolve(std::string_view value) const
    {
        UserToken::Digest hash;
        try { hash = UserToken(value).digest(); }
        catch (const std::invalid_argument &) { return std::nullopt; }
        for (const auto &[userId, stored] : tokens_)
        {
            if (CRYPTO_memcmp(hash.data(), stored.data(), hash.size()) == 0) return userId;
        }
        return std::nullopt;
    }

    std::vector<UserTokenSaveData> UserTokens::toSaveData() const
    {
        std::vector<UserTokenSaveData> result;
        result.reserve(tokens_.size());
        for (const auto &[userId, hash] : tokens_) result.push_back({userId, hash});
        std::sort(result.begin(), result.end(), [](const auto &left, const auto &right) { return left.userId < right.userId; });
        return result;
    }

    UserTokens UserTokens::fromSaveData(const std::vector<UserTokenSaveData> &data)
    {
        UserTokens result;
        for (const auto &entry : data)
        {
            if (result.containsHash(entry.tokenHash)) throw std::invalid_argument("duplicate token hash in identity data");
            if (!result.tokens_.emplace(entry.userId, entry.tokenHash).second) throw std::invalid_argument("duplicate token user ID in identity data");
        }
        return result;
    }
}
