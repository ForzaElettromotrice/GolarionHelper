#pragma once

#include <array>
#include <string>
#include <string_view>

namespace golarion::play
{
    class UserToken final
    {
    public:
        using Digest = std::array<unsigned char, 32>;

        explicit UserToken(std::string_view value);
        static UserToken generate();
        std::string value() const;
        Digest digest() const;

    private:
        std::array<unsigned char, 8> bytes_{};
    };
}
