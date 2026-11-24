#include "golarion/play/user_token.hpp"
#include "token_encoding.hpp"

#if defined(_WIN32)
#include <windows.h>
#include <bcrypt.h>
#else
#include <unistd.h>
#endif

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
#if defined(_WIN32)
        if (BCryptGenRandom(nullptr, bytes.data(), static_cast<ULONG>(bytes.size()), BCRYPT_USE_SYSTEM_PREFERRED_RNG) != 0)
#else
        if (::getentropy(bytes.data(), bytes.size()) != 0)
#endif
            throw std::runtime_error("cannot generate user token");
        return UserToken(detail::encodeHex(bytes));
    }

    std::string UserToken::value() const
    {
        return detail::encodeHex(bytes_);
    }

}
