#pragma once

#include <array>
#include <optional>
#include <string_view>

namespace golarion::play
{
    class SessionCode final
    {
    public:
        explicit SessionCode(std::string_view value);

        static SessionCode generate();
        std::string_view value() const noexcept;
        bool matches(std::string_view candidate) const noexcept;

    private:
        using Value = std::array<char, 5>;

        static std::optional<Value> normalize(std::string_view candidate) noexcept;

        Value value_{};
    };
}
