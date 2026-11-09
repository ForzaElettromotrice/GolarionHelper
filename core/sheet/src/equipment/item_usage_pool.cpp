#include "golarion/equipment/item_usage_pool.hpp"
#include "golarion/util/string_utils.hpp"

#include <stdexcept>
#include <utility>

namespace golarion
{
    ItemUsagePools createItemUsagePools(const std::vector<ItemUsagePoolDefinition> &definitions, const std::optional<std::map<std::string, int>> &remaining)
    {
        std::map<std::string, int> normalized;
        if (remaining)
            for (const auto &[id, uses] : *remaining)
                if (!normalized.emplace(normalize(id), uses).second) throw std::invalid_argument("duplicate item usage pool state");
        ItemUsagePools pools;
        for (const auto &definition : definitions)
        {
            const auto id = normalize(definition.id);
            if (remaining && !normalized.contains(id)) throw std::invalid_argument("missing item usage pool state");
            auto pool = std::make_shared<UsagePool>(definition.capacity, remaining ? std::optional<int>(normalized.at(id)) : std::nullopt);
            if (!pools.emplace(id, std::move(pool)).second) throw std::invalid_argument("duplicate item usage pool definition");
        }
        if (remaining && normalized.size() != pools.size()) throw std::invalid_argument("unknown item usage pool state");
        return pools;
    }
}
