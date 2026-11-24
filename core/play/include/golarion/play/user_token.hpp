#pragma once

#include <array>
#include <string>
#include <string_view>

namespace golarion::play
{
    class UserToken final
    {
    public:
        explicit UserToken(std::string_view value);
        static UserToken generate();
        std::string value() const;

    private:
        std::array<unsigned char, 8> bytes_{};
    };
}
