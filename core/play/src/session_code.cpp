#include "golarion/play/session_code.hpp"

#include <random>
#include <stdexcept>

namespace golarion::play
{
    SessionCode::SessionCode(std::string_view value)
    {
        const auto normalized = normalize(value);
        if (!normalized)
        {
            throw std::invalid_argument("session code must contain exactly five ASCII letters or digits");
        }
        value_ = *normalized;
    }

    SessionCode SessionCode::generate()
    {
        constexpr std::string_view alphabet = "ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
        std::random_device random;
        std::uniform_int_distribution<std::size_t> distribution(0, alphabet.size() - 1);
        Value value{};
        for (char &character : value) character = alphabet[distribution(random)];
        return SessionCode(std::string_view(value.data(), value.size()));
    }

    std::string_view SessionCode::value() const noexcept
    {
        return {value_.data(), value_.size()};
    }

    bool SessionCode::matches(std::string_view candidate) const noexcept
    {
        const auto normalized = normalize(candidate);
        return normalized && *normalized == value_;
    }

    std::optional<SessionCode::Value> SessionCode::normalize(std::string_view candidate) noexcept
    {
        Value value{};
        if (candidate.size() != value.size()) return std::nullopt;
        for (std::size_t index = 0; index < value.size(); ++index)
        {
            char character = candidate[index];
            if (character >= 'a' && character <= 'z') character = static_cast<char>(character - 'a' + 'A');
            if (!((character >= 'A' && character <= 'Z') || (character >= '0' && character <= '9'))) return std::nullopt;
            value[index] = character;
        }
        return value;
    }
}
