#pragma once

#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace golarion::play
{
    inline std::string normalizeDisplayName(std::string_view name)
    {
        const auto isSpace = [](std::uint32_t code)
        {
            return code == 0x20 || code == 0xa0 || code == 0x1680 || (code >= 0x2000 && code <= 0x200a)
                || code == 0x202f || code == 0x205f || code == 0x3000;
        };
        constexpr std::size_t maximumCharacters = 48;
        std::size_t first = name.size(), last = 0, characters = 0, lastCharacter = 0;
        for (std::size_t index = 0; index < name.size();)
        {
            const auto start = index;
            const auto lead = static_cast<unsigned char>(name[index++]);
            std::uint32_t code = lead;
            if (lead >= 0x80)
            {
                if (lead < 0xc2 || lead > 0xf4) throw std::invalid_argument("display name must be valid UTF-8");
                const unsigned extra = lead < 0xe0 ? 1 : lead < 0xf0 ? 2 : 3;
                code = lead & (0x7f >> (extra + 1));
                for (unsigned i = 0; i < extra; ++i)
                {
                    if (index == name.size()) throw std::invalid_argument("display name must be valid UTF-8");
                    const auto byte = static_cast<unsigned char>(name[index++]);
                    if ((byte & 0xc0) != 0x80) throw std::invalid_argument("display name must be valid UTF-8");
                    code = (code << 6) | (byte & 0x3f);
                }
                if ((extra == 2 && code < 0x800) || (extra == 3 && code < 0x10000)
                    || (code >= 0xd800 && code <= 0xdfff) || code > 0x10ffff)
                    throw std::invalid_argument("display name must be valid UTF-8");
            }
            if (code < 0x20 || (code >= 0x7f && code <= 0x9f) || code == 0x2028 || code == 0x2029)
                throw std::invalid_argument("display name must be on one line without control characters");
            if (isSpace(code))
            {
                if (first != name.size()) ++characters;
                continue;
            }
            if (first == name.size()) first = start;
            last = index;
            lastCharacter = ++characters;
            if (lastCharacter > maximumCharacters) throw std::invalid_argument("display name exceeds 48 characters");
        }
        if (lastCharacter == 0) throw std::invalid_argument("display name must not be blank");
        return std::string(name.substr(first, last - first));
    }
}
