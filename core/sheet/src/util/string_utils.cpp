#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace golarion
{
    std::string normalize(std::string_view value)
    {
        const auto isWhitespace = [](unsigned char character)
        {
            return std::isspace(character) != 0;
        };

        auto begin = std::find_if_not(value.begin(), value.end(), isWhitespace);
        auto end = std::find_if_not(value.rbegin(), value.rend(), isWhitespace).base();

        if (begin >= end)
        {
            throw std::invalid_argument("value must not be blank");
        }

        return std::string(begin, end);
    }
}
