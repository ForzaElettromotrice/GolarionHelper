#pragma once

#include <stdexcept>

namespace golarion::play
{
    class PermissionDenied final : public std::invalid_argument
    {
    public:
        using std::invalid_argument::invalid_argument;
    };
}
