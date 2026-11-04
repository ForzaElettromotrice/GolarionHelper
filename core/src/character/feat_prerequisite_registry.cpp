#include "golarion/character/feat_prerequisite_registry.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <stdexcept>
#include <utility>

namespace golarion
{
    FeatPrerequisiteRegistry::FeatPrerequisiteRegistry(ResourceManager &resourceManager)
        : resourceManager_(resourceManager)
    {
        resourceManager_.registerCollectionResource<FeatPrerequisiteAlternative>(FeatPrerequisiteAlternativesResource, [this](FeatPrerequisiteAlternative alternative)
        {
            add(std::move(alternative));
        }, [this](std::string_view id)
        {
            remove(id);
        });
    }

    FeatPrerequisiteRegistry::~FeatPrerequisiteRegistry()
    {
        resourceManager_.unregisterCollectionResource(FeatPrerequisiteAlternativesResource);
    }

    void FeatPrerequisiteRegistry::add(FeatPrerequisiteAlternative alternative)
    {
        alternative.id = normalize(alternative.id);
        alternative.source = normalize(alternative.source);
        alternative.target = normalize(alternative.target);
        if (!alternative.applies || !alternative.calculate)
        {
            throw std::invalid_argument("feat prerequisite alternative callbacks must not be empty");
        }
        const std::string id = alternative.id;
        if (!alternatives_.emplace(id, std::move(alternative)).second)
        {
            throw std::invalid_argument("feat prerequisite alternative is already registered: " + id);
        }
    }

    void FeatPrerequisiteRegistry::remove(std::string_view id)
    {
        const std::string normalizedId = normalize(id);
        if (alternatives_.erase(normalizedId) == 0)
        {
            throw std::invalid_argument("feat prerequisite alternative is not registered: " + normalizedId);
        }
    }

    std::vector<FeatPrerequisiteValue> FeatPrerequisiteRegistry::values(std::string_view target, const FeatPrerequisiteContext &context) const
    {
        const std::string normalizedTarget = normalize(target);
        const FeatTargetResolver originalValue = [this](std::string_view name)
        {
            return resourceManager_.targetValue(name);
        };
        std::vector<FeatPrerequisiteValue> result{
            {.value = originalValue(normalizedTarget), .alternativeId = std::nullopt, .source = std::nullopt}
        };
        for (const auto &[id, alternative] : alternatives_)
        {
            if (alternative.target == normalizedTarget && alternative.applies(context))
            {
                result.push_back(FeatPrerequisiteValue{
                    .value = alternative.calculate(context, originalValue),
                    .alternativeId = id,
                    .source = alternative.source
                });
            }
        }
        return result;
    }

    bool FeatPrerequisiteRegistry::satisfies(std::string_view target, const FeatPrerequisiteContext &context, const std::function<bool(int)> &requirement) const
    {
        if (!requirement)
        {
            throw std::invalid_argument("feat prerequisite value check must not be empty");
        }
        const auto candidates = values(target, context);
        return std::any_of(candidates.begin(), candidates.end(), [&requirement](const auto &candidate)
        {
            return requirement(candidate.value);
        });
    }
}
