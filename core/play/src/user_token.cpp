#include "golarion/play/user_token.hpp"
#include "token_encoding.hpp"

#include <openssl/evp.h>
#include <openssl/rand.h>

#include <stdexcept>

namespace golarion::play
{
    UserToken::UserToken(std::string_view value)
    {
        const auto bytes = detail::decodeHex<8>(value);
        if (!bytes) throw std::invalid_argument("user token must contain exactly sixteen lowercase hexadecimal characters");
        bytes_ = *bytes;
    }

    UserToken UserToken::generate()
    {
        std::array<unsigned char, 8> bytes{};
        if (RAND_priv_bytes(bytes.data(), static_cast<int>(bytes.size())) != 1) throw std::runtime_error("cannot generate user token");
        return UserToken(detail::encodeHex(bytes));
    }

    std::string UserToken::value() const
    {
        return detail::encodeHex(bytes_);
    }

    UserToken::Digest UserToken::digest() const
    {
        Digest result{};
        unsigned int size = 0;
        if (EVP_Digest(bytes_.data(), bytes_.size(), result.data(), &size, EVP_sha256(), nullptr) != 1 || size != result.size())
        {
            throw std::runtime_error("cannot hash user token");
        }
        return result;
    }
}
