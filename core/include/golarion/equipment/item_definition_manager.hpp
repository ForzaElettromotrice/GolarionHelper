#pragma once

#include "golarion/equipment/item.hpp"

#include <map>
#include <mutex>
#include <string>
#include <string_view>

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

        const ItemDefinition &get(std::string_view itemDefinitionId);

    private:
        ItemDefinitionManager() = default;

        void loadCatalog();

        std::map<std::string, ItemDefinition> definitions_;
        bool loaded_ = false;
        std::mutex mutex_;
    };
}
