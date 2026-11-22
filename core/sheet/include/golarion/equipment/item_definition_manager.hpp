#pragma once

#include "golarion/equipment/item.hpp"

#include <map>
#include <mutex>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace golarion
{
    class ItemDefinitionManager final
    {
    public:
        static ItemDefinitionManager &instance();

        ItemDefinitionManager(const ItemDefinitionManager &) = delete;
        ItemDefinitionManager &operator=(const ItemDefinitionManager &) = delete;
        ItemDefinitionManager(ItemDefinitionManager &&) = delete;
        ItemDefinitionManager &operator=(ItemDefinitionManager &&) = delete;

        bool contains(std::string_view itemDefinitionId);
        const ItemDefinition &get(std::string_view itemDefinitionId);
        std::vector<std::pair<std::string, std::string>> catalogEntries();

    private:
        ItemDefinitionManager() = default;

        void loadCatalog();

        std::map<std::string, ItemDefinition> definitions_;
        bool loaded_ = false;
        std::mutex mutex_;
    };
}
