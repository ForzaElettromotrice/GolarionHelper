#pragma once
#include "golarion/resource/usage_pool.hpp"

#include <map>
#include <memory>
#include <optional>
#include <string>
#include <vector>

namespace golarion
{
    struct ItemUsagePoolDefinition
    {
        std::string id;
        int capacity;
    };

    using ItemUsagePool = UsagePool;

    using ItemUsagePools = std::map<std::string, std::shared_ptr<ItemUsagePool>>;
    ItemUsagePools createItemUsagePools(const std::vector<ItemUsagePoolDefinition> &definitions, const std::optional<std::map<std::string, int>> &remaining = std::nullopt);
}
