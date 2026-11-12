#pragma once

#include <array>
#include <optional>
#include <string>
#include <string_view>

namespace golarion::play::detail
{
    template<std::size_t Size> std::string encodeHex(const std::array<unsigned char, Size> &bytes)
    {
        constexpr std::string_view alphabet = "0123456789abcdef";
        std::string result(Size * 2, '0');
        for (std::size_t index = 0; index < Size; ++index)
        {
            result[index * 2] = alphabet[bytes[index] >> 4];
            result[index * 2 + 1] = alphabet[bytes[index] & 15];
        }
        return result;
    }

    template<std::size_t Size> std::optional<std::array<unsigned char, Size>> decodeHex(std::string_view value) noexcept
    {
        if (value.size() != Size * 2) return std::nullopt;
        std::array<unsigned char, Size> result{};
        for (std::size_t index = 0; index < value.size(); ++index)
        {
            const char character = value[index];
            unsigned char digit;
            if (character >= '0' && character <= '9') digit = static_cast<unsigned char>(character - '0');
            else if (character >= 'a' && character <= 'f') digit = static_cast<unsigned char>(character - 'a' + 10);
            else return std::nullopt;
            result[index / 2] = static_cast<unsigned char>((result[index / 2] << 4) | digit);
        }
        return result;
    }
}
