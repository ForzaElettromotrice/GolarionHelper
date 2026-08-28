#include "golarion/character/initiative.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/initiative_view.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    int checkedInitiativeValue(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("initiative value is out of range");
        }
        return static_cast<int>(value);
    }
}

namespace golarion
{
    InitiativeAbilityReplacement::InitiativeAbilityReplacement(InitiativeAbilityReplacementDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          abilityType_(definition.abilityType)
    {
    }

    Initiative::Initiative(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
        resourceManager_.registerEnhanceableResource(InitiativeResource, {std::string(AbilityCheckRootResource)});
        resourceManager_.registerCollectionResource<InitiativeAbilityReplacement>(InitiativeAbilityReplacementsResource, [this](InitiativeAbilityReplacement replacement)
        {
            addAbilityReplacement(std::move(replacement));
        }, [this](std::string_view replacementId)
        {
            removeAbilityReplacement(replacementId);
        });
    }

    InitiativeView Initiative::toView()
    {
        ModifierSetView modifiers = resourceManager_.modifierSetView(InitiativeResource);
        std::vector<InitiativeAbilityOptionView> abilityOptions;
        abilityOptions.reserve(abilityReplacements_.size() + 1);
        const int dexterityModifier = resourceManager_.targetValue("dexMod");
        ModifierSetView dexterityModifiers = resourceManager_.modifierSetView(std::vector<std::string>{std::string(InitiativeResource), abilityCheckResourceName(AbilityType::Dexterity)});
        abilityOptions.push_back(InitiativeAbilityOptionView{
            .replacementId = std::nullopt,
            .source = "Base",
            .abilityType = AbilityType::Dexterity,
            .abilityModifier = dexterityModifier,
            .totalValue = checkedInitiativeValue(static_cast<long long>(dexterityModifier) + dexterityModifiers.total),
            .modifiers = std::move(dexterityModifiers)
        });
        for (const auto &[id, replacement] : abilityReplacements_)
        {
            const int abilityModifier = resourceManager_.targetValue(std::string(resourceName(replacement.abilityType_)) + "Mod");
            ModifierSetView optionModifiers = resourceManager_.modifierSetView(std::vector<std::string>{std::string(InitiativeResource), abilityCheckResourceName(replacement.abilityType_)});
            abilityOptions.push_back(InitiativeAbilityOptionView{
                .replacementId = id,
                .source = replacement.source_,
                .abilityType = replacement.abilityType_,
                .abilityModifier = abilityModifier,
                .totalValue = checkedInitiativeValue(static_cast<long long>(abilityModifier) + optionModifiers.total),
                .modifiers = std::move(optionModifiers)
            });
        }

        return InitiativeView{
            .abilityOptions = std::move(abilityOptions),
            .modifiers = std::move(modifiers)
        };
    }

    void Initiative::addAbilityReplacement(InitiativeAbilityReplacement replacement)
    {
        const std::string id = replacement.id_;
        if (!abilityReplacements_.emplace(id, std::move(replacement)).second)
        {
            throw std::invalid_argument("initiative ability replacement is already registered: " + id);
        }
    }

    void Initiative::removeAbilityReplacement(std::string_view replacementId)
    {
        const std::string id = normalize(replacementId);
        if (abilityReplacements_.erase(id) == 0)
        {
            throw std::invalid_argument("initiative ability replacement is not registered: " + id);
        }
    }
}
