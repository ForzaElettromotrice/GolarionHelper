#include "golarion/resource/resource_manager.hpp"

#include "golarion/resource/expression_parser.hpp"
#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace
{
    class ResolutionGuard final
    {
    public:
        ResolutionGuard(std::vector<std::string> &stack, std::string name) : stack_(stack)
        {
            stack_.push_back(std::move(name));
        }

        ~ResolutionGuard()
        {
            stack_.pop_back();
        }

        ResolutionGuard(const ResolutionGuard &) = delete;
        ResolutionGuard &operator=(const ResolutionGuard &) = delete;

    private:
        std::vector<std::string> &stack_;
    };
}

namespace golarion
{
    void ResourceManager::registerEnhanceableResource(std::string_view name)
    {
        registerEnhanceableResource(name, {});
    }

    void ResourceManager::registerEnhanceableResource(std::string_view name, std::vector<std::string> parentResources)
    {
        std::string normalizedName = normalize(name);
        if (accumulatedResources_.contains(normalizedName) || collectionResources_.contains(normalizedName))
        {
            throw std::invalid_argument("resource is already registered: " + normalizedName);
        }
        std::unordered_set<std::string> uniqueParentResources;

        for (std::string &parentResource : parentResources)
        {
            parentResource = normalize(parentResource);
            if (parentResource == normalizedName)
            {
                throw std::invalid_argument("resource must not inherit from itself: " + normalizedName);
            }
            if (!uniqueParentResources.insert(parentResource).second)
            {
                throw std::invalid_argument("parent resource is duplicated: " + parentResource);
            }
            if (!enhanceableResources_.contains(parentResource))
            {
                throw std::invalid_argument("parent resource is not registered: " + parentResource);
            }
        }

        const bool inserted = enhanceableResources_.emplace(normalizedName, EnhanceableResource{
            .modifierSet = ModifierSet{},
            .parentResources = std::move(parentResources)
        }).second;
        if (!inserted)
        {
            throw std::invalid_argument("resource is already registered: " + normalizedName);
        }
    }

    void ResourceManager::registerAccumulatedResource(std::string_view name)
    {
        const std::string normalizedName = normalize(name);
        if (enhanceableResources_.contains(normalizedName) || collectionResources_.contains(normalizedName))
        {
            throw std::invalid_argument("resource is already registered: " + normalizedName);
        }

        const bool inserted = accumulatedResources_.emplace(normalizedName, ContributionSet{}).second;
        if (!inserted)
        {
            throw std::invalid_argument("resource is already registered: " + normalizedName);
        }
    }

    void ResourceManager::unregisterAccumulatedResource(std::string_view name)
    {
        const std::string normalizedName = normalize(name);
        if (accumulatedResources_.erase(normalizedName) == 0)
        {
            throw std::invalid_argument("resource is not registered: " + normalizedName);
        }
    }

    void ResourceManager::unregisterEnhanceableResource(std::string_view name)
    {
        const std::string normalizedName = normalize(name);
        if (!enhanceableResources_.contains(normalizedName))
        {
            throw std::invalid_argument("resource is not registered: " + normalizedName);
        }

        for (const auto &[resourceName, resource] : enhanceableResources_)
        {
            if (std::ranges::find(resource.parentResources, normalizedName) != resource.parentResources.end())
            {
                throw std::invalid_argument("resource is inherited by another resource: " + resourceName);
            }
        }

        if (!enhanceableResources_.at(normalizedName).modifierSet.empty())
        {
            throw std::invalid_argument("resource still contains modifiers: " + normalizedName);
        }

        enhanceableResources_.erase(normalizedName);
    }

    void ResourceManager::unregisterCollectionResource(std::string_view name)
    {
        const std::string normalizedName = normalize(name);
        if (collectionResources_.erase(normalizedName) == 0)
        {
            throw std::invalid_argument("resource is not registered: " + normalizedName);
        }
    }

    void ResourceManager::registerTarget(std::string_view name, std::function<int()> valueSupplier)
    {
        if (!valueSupplier)
        {
            throw std::invalid_argument("value supplier must not be empty");
        }

        std::string normalizedName = normalize(name);
        const bool inserted = targets_.emplace(normalizedName, std::move(valueSupplier)).second;
        if (!inserted)
        {
            throw std::invalid_argument("target is already registered: " + normalizedName);
        }
    }

    int ResourceManager::targetValue(std::string_view name)
    {
        std::string normalizedName = normalize(name);
        auto target = targets_.find(normalizedName);
        if (target == targets_.end())
        {
            throw std::invalid_argument("target is not registered: " + normalizedName);
        }

        if (std::find(targetResolutionStack_.begin(), targetResolutionStack_.end(), normalizedName) != targetResolutionStack_.end())
        {
            throw std::invalid_argument("circular target reference: " + circularReference(normalizedName));
        }

        ResolutionGuard guard(targetResolutionStack_, normalizedName);
        return target->second();
    }

    int ResourceManager::evaluateExpression(std::string_view expression)
    {
        return ExpressionParser::evaluate(expression, [this](std::string_view targetName)
        {
            return targetValue(targetName);
        });
    }

    int ResourceManager::modifierTotal(std::string_view resourceName)
    {
        return ModifierSet::calculateTotal(inheritedModifierSets(resourceName), *this);
    }

    int ResourceManager::contributionTotal(std::string_view resourceName)
    {
        return contributionSet(resourceName).calculateTotal(*this);
    }

    bool ResourceManager::enhanceableResourceIsOrInheritsFrom(std::string_view resourceName, std::string_view ancestorResourceName) const
    {
        const std::string normalizedResourceName = normalize(resourceName);
        const std::string normalizedAncestorName = normalize(ancestorResourceName);
        if (!enhanceableResources_.contains(normalizedResourceName))
        {
            throw std::invalid_argument("resource is not registered: " + normalizedResourceName);
        }
        if (!enhanceableResources_.contains(normalizedAncestorName))
        {
            throw std::invalid_argument("ancestor resource is not registered: " + normalizedAncestorName);
        }
        if (normalizedResourceName == normalizedAncestorName)
        {
            return true;
        }

        std::vector<std::string> pending{normalizedResourceName};
        std::unordered_set<std::string> visited;
        while (!pending.empty())
        {
            const std::string current = std::move(pending.back());
            pending.pop_back();
            if (!visited.insert(current).second)
            {
                continue;
            }
            for (const std::string &parent : enhanceableResources_.at(current).parentResources)
            {
                if (parent == normalizedAncestorName)
                {
                    return true;
                }
                pending.push_back(parent);
            }
        }
        return false;
    }

    ContributionSetView ResourceManager::contributionSetView(std::string_view resourceName)
    {
        return contributionSet(resourceName).toView(*this);
    }

    ModifierSetView ResourceManager::modifierSetView(std::string_view resourceName)
    {
        return ModifierSet::toView(inheritedModifierSets(resourceName), *this);
    }

    ResourceManagerView ResourceManager::toView()
    {
        std::vector<ResourceManagerView::TargetView> targetViews;
        targetViews.reserve(targets_.size());

        for (const auto &[name, valueSupplier]: targets_)
        {
            targetViews.push_back(ResourceManagerView::TargetView{
                .name = name,
                .value = targetValue(name)
            });
        }

        std::ranges::sort(targetViews, {}, &ResourceManagerView::TargetView::name);
        std::vector<ResourceManagerView::EnhanceableResourceView> enhanceableResourceViews;
        enhanceableResourceViews.reserve(enhanceableResources_.size());
        for (const auto &[name, resource] : enhanceableResources_)
        {
            enhanceableResourceViews.push_back(ResourceManagerView::EnhanceableResourceView{
                .name = name,
                .parentResources = resource.parentResources,
                .modifiers = modifierSetView(name)
            });
        }
        std::ranges::sort(enhanceableResourceViews, {}, &ResourceManagerView::EnhanceableResourceView::name);

        std::vector<std::string> collectionNames;
        collectionNames.reserve(collectionResources_.size());
        for (const auto &[name, resource] : collectionResources_)
        {
            static_cast<void>(resource);
            collectionNames.push_back(name);
        }
        std::ranges::sort(collectionNames);
        return ResourceManagerView{
            .targets = std::move(targetViews),
            .enhanceableResources = std::move(enhanceableResourceViews),
            .collections = std::move(collectionNames)
        };
    }

    void ResourceManager::addModifier(std::string_view resourceName, Modifier modifier)
    {
        modifierSet(resourceName).addModifier(std::move(modifier));
    }

    void ResourceManager::removeModifier(std::string_view resourceName, std::string_view modifierId)
    {
        modifierSet(resourceName).removeModifier(modifierId);
    }

    void ResourceManager::addContribution(std::string_view resourceName, Contribution contribution)
    {
        contributionSet(resourceName).addContribution(std::move(contribution));
    }

    void ResourceManager::removeContribution(std::string_view resourceName, std::string_view contributionId)
    {
        contributionSet(resourceName).removeContribution(contributionId);
    }

    void ResourceManager::removeFromCollection(std::string_view resourceName, std::string_view itemId)
    {
        collectionResource(resourceName).removeItem(itemId);
    }

    ModifierSet &ResourceManager::modifierSet(std::string_view name)
    {
        return enhanceableResource(name).modifierSet;
    }

    ResourceManager::EnhanceableResource &ResourceManager::enhanceableResource(std::string_view name)
    {
        std::string normalizedName = normalize(name);
        auto resource = enhanceableResources_.find(normalizedName);
        if (resource == enhanceableResources_.end())
        {
            throw std::invalid_argument("resource is not registered: " + normalizedName);
        }

        return resource->second;
    }

    ContributionSet &ResourceManager::contributionSet(std::string_view name)
    {
        const std::string normalizedName = normalize(name);
        auto resource = accumulatedResources_.find(normalizedName);
        if (resource == accumulatedResources_.end())
        {
            throw std::invalid_argument("resource is not registered: " + normalizedName);
        }
        return resource->second;
    }

    ResourceManager::CollectionResource &ResourceManager::collectionResource(std::string_view name)
    {
        const std::string normalizedName = normalize(name);
        auto resource = collectionResources_.find(normalizedName);
        if (resource == collectionResources_.end())
        {
            throw std::invalid_argument("resource is not registered: " + normalizedName);
        }
        return resource->second;
    }

    std::vector<const ModifierSet *> ResourceManager::inheritedModifierSets(std::string_view name) const
    {
        const std::string normalizedName = normalize(name);
        if (!enhanceableResources_.contains(normalizedName))
        {
            throw std::invalid_argument("resource is not registered: " + normalizedName);
        }

        std::unordered_map<std::string, bool> visitedResources;
        std::vector<const ModifierSet *> modifierSets;
        collectModifierSets(normalizedName, visitedResources, modifierSets);
        return modifierSets;
    }

    void ResourceManager::collectModifierSets(const std::string &name, std::unordered_map<std::string, bool> &visitedResources, std::vector<const ModifierSet *> &modifierSets) const
    {
        auto visited = visitedResources.find(name);
        if (visited != visitedResources.end())
        {
            if (!visited->second)
            {
                throw std::invalid_argument("circular enhanceable resource inheritance: " + name);
            }
            return;
        }

        visitedResources.emplace(name, false);
        const EnhanceableResource &resource = enhanceableResources_.at(name);
        modifierSets.push_back(&resource.modifierSet);

        for (const std::string &parentResource : resource.parentResources)
        {
            collectModifierSets(parentResource, visitedResources, modifierSets);
        }

        visitedResources.at(name) = true;
    }

    std::string ResourceManager::circularReference(std::string_view repeatedName) const
    {
        auto cycleStart = std::find(targetResolutionStack_.begin(), targetResolutionStack_.end(), repeatedName);
        std::ostringstream stream;

        for (auto current = cycleStart; current != targetResolutionStack_.end(); ++current)
        {
            if (current != cycleStart)
            {
                stream << " -> ";
            }
            stream << *current;
        }
        stream << " -> " << repeatedName;

        return stream.str();
    }
}
