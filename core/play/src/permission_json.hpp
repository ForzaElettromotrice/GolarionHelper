#pragma once

#include "golarion/play/permission_set.hpp"

#include <nlohmann/json.hpp>

#include <cstdint>
#include <stdexcept>

namespace golarion::play::detail
{
    inline PermissionSet parsePermissions(const nlohmann::json &value)
    {
        if (!value.is_number_integer() || (!value.is_number_unsigned() && value.get<std::int64_t>() < 0))
        {
            throw std::invalid_argument("permissions must be a nonnegative integer mask");
        }
        return PermissionSet::fromMask(value.get<std::uint64_t>());
    }
}
