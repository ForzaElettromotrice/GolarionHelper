#pragma once

#include "golarion/resource/contribution.hpp"
#include "golarion/resource/contribution_set.hpp"
#include "golarion/view/resource_manager_view.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/modifier_set.hpp"
#include "golarion/view/modifier_set_view.hpp"
#include "golarion/view/contribution_set_view.hpp"
#include "golarion/util/string_utils.hpp"

#include <any>
#include <functional>
#include <stdexcept>
#include <string>
#include <string_view>
#include <typeindex>
#include <type_traits>
#include <unordered_map>
#include <utility>
#include <vector>

namespace golarion
{
    class ResourceManager final
    {
    public:
        ResourceManager() = default;
        ResourceManager(const ResourceManager &) = delete;
        ResourceManager &operator=(const ResourceManager &) = delete;
        ResourceManager(ResourceManager &&) = delete;
        ResourceManager &operator=(ResourceManager &&) = delete;

        void registerEnhanceableResource(std::string_view name);
        void registerEnhanceableResource(std::string_view name, std::vector<std::string> parentResources);
        void unregisterEnhanceableResource(std::string_view name);
        void registerAccumulatedResource(std::string_view name);
        void unregisterAccumulatedResource(std::string_view name);
        void registerTarget(std::string_view name, std::function<int()> valueSupplier);
        template<typename Item> void registerCollectionResource(std::string_view name, std::function<void(Item)> addItem, std::function<void(std::string_view)> removeItem);
        void unregisterCollectionResource(std::string_view name);

        int targetValue(std::string_view name);
        int evaluateExpression(std::string_view expression);
        int modifierTotal(std::string_view resourceName);
        int modifierTotal(const std::vector<std::string> &resourceNames);
        int contributionTotal(std::string_view resourceName);
        bool enhanceableResourceIsOrInheritsFrom(std::string_view resourceName, std::string_view ancestorResourceName) const;
        ContributionSetView contributionSetView(std::string_view resourceName);
        ModifierSetView modifierSetView(std::string_view resourceName);
        ModifierSetView modifierSetView(std::string_view resourceName, const std::vector<std::string> &activeConditions);
        ModifierSetView modifierSetView(const std::vector<std::string> &resourceNames);
        ModifierSetView modifierSetView(const std::vector<std::string> &resourceNames, const std::vector<std::string> &activeConditions);
        ResourceManagerView toView();

        void addModifier(std::string_view resourceName, Modifier modifier);
        void removeModifier(std::string_view resourceName, std::string_view modifierId);
        void addContribution(std::string_view resourceName, Contribution contribution);
        void removeContribution(std::string_view resourceName, std::string_view contributionId);
        template<typename Item> void addToCollection(std::string_view resourceName, Item item);
        void removeFromCollection(std::string_view resourceName, std::string_view itemId);

    private:
        struct EnhanceableResource
        {
            ModifierSet modifierSet;
            std::vector<std::string> parentResources;
        };

        struct CollectionResource
        {
            std::type_index itemType;
            std::function<void(std::any)> addItem;
            std::function<void(std::string_view)> removeItem;
        };

        EnhanceableResource &enhanceableResource(std::string_view name);
        ContributionSet &contributionSet(std::string_view name);
        ModifierSet &modifierSet(std::string_view name);
        CollectionResource &collectionResource(std::string_view name);
        std::vector<const ModifierSet *> inheritedModifierSets(std::string_view name) const;
        std::vector<const ModifierSet *> inheritedModifierSets(const std::vector<std::string> &names) const;
        void collectModifierSets(const std::string &name, std::unordered_map<std::string, bool> &visitedResources, std::vector<const ModifierSet *> &modifierSets) const;
        std::string circularReference(std::string_view repeatedName) const;

        std::unordered_map<std::string, EnhanceableResource> enhanceableResources_;
        std::unordered_map<std::string, ContributionSet> accumulatedResources_;
        std::unordered_map<std::string, CollectionResource> collectionResources_;
        std::unordered_map<std::string, std::function<int()>> targets_;
        std::vector<std::string> targetResolutionStack_;
    };

    template<typename Item>
    void ResourceManager::registerCollectionResource(std::string_view name, std::function<void(Item)> addItem, std::function<void(std::string_view)> removeItem)
    {
        static_assert(std::is_copy_constructible_v<Item>, "collection resource items must be copy constructible");
        if (!addItem || !removeItem)
        {
            throw std::invalid_argument("collection resource callbacks must not be empty");
        }

        const std::string normalizedName = normalize(name);
        if (enhanceableResources_.contains(normalizedName) || accumulatedResources_.contains(normalizedName) || collectionResources_.contains(normalizedName))
        {
            throw std::invalid_argument("resource is already registered: " + normalizedName);
        }

        collectionResources_.emplace(normalizedName, CollectionResource{
            .itemType = typeid(Item),
            .addItem = [callback = std::move(addItem)](std::any item) mutable
            {
                callback(std::any_cast<Item>(std::move(item)));
            },
            .removeItem = std::move(removeItem)
        });
    }

    template<typename Item>
    void ResourceManager::addToCollection(std::string_view resourceName, Item item)
    {
        CollectionResource &resource = collectionResource(resourceName);
        if (resource.itemType != std::type_index(typeid(Item)))
        {
            throw std::invalid_argument("collection resource item type does not match: " + normalize(resourceName));
        }
        resource.addItem(std::any(std::move(item)));
    }

}
