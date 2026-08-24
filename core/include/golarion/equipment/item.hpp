#pragma once

#include <cstdint>
#include <functional>
#include <string>

namespace golarion
{
    class Inventory;
    class ResourceManager;

    struct ItemDefinition
    {
        std::string id;
        std::string name;
        std::int64_t weightGrams;
    };

    struct ItemInstanceDefinition
    {
        std::string id;
        std::string itemDefinitionId;
        int quantity = 1;
    };

    class ItemInstance final
    {
    public:
        explicit ItemInstance(ItemInstanceDefinition definition);

        void registerWeight(ResourceManager &resourceManager) const;
        void refreshWeight(ResourceManager &resourceManager) const;
        void unregisterWeight(ResourceManager &resourceManager) const;

    private:
        friend class Inventory;

        std::string id_;
        std::reference_wrapper<const ItemDefinition> itemDefinition_;
        int quantity_;
    };
}
