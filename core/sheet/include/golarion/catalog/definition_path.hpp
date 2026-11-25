#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>

namespace golarion::catalog
{
    inline std::filesystem::path definitionPath(std::string_view library, const std::string &id)
    {
        constexpr std::string_view letters = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ";
        if (id.empty() || letters.find(id.front()) == letters.npos ||
            id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != id.npos)
            throw std::invalid_argument("Invalid catalog filename ID: " + id);
        char initial = id.front();
        if (initial >= 'A' && initial <= 'Z') initial += 'a' - 'A';
        const std::string bucket(1, initial);
        return std::filesystem::path(library) / bucket / (id + ".json");
    }
}
