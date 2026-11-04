#include "golarion/character/saving_throw.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/saving_throw_view.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    int checkedSavingThrowValue(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("saving throw value is out of range");
        }
        return static_cast<int>(value);
    }
}

namespace golarion
{
    std::string_view displayName(SavingThrowType type)
    {
        switch (type)
        {
            case SavingThrowType::Fortitude:
                return "Tempra";
            case SavingThrowType::Reflex:
                return "Riflessi";
            case SavingThrowType::Will:
                return "Volontà";
        }
        throw std::invalid_argument("unknown saving throw type");
    }

    std::string_view resourceName(SavingThrowType type)
    {
        switch (type)
        {
            case SavingThrowType::Fortitude:
                return "savingThrow.fortitude";
            case SavingThrowType::Reflex:
                return "savingThrow.reflex";
            case SavingThrowType::Will:
                return "savingThrow.will";
        }
        throw std::invalid_argument("unknown saving throw type");
    }

    std::string baseResourceName(SavingThrowType type)
    {
        return std::string(resourceName(type)) + ".base";
    }

    AbilityType defaultAbility(SavingThrowType type)
    {
        switch (type)
        {
            case SavingThrowType::Fortitude:
                return AbilityType::Constitution;
            case SavingThrowType::Reflex:
                return AbilityType::Dexterity;
            case SavingThrowType::Will:
                return AbilityType::Wisdom;
        }
        throw std::invalid_argument("unknown saving throw type");
    }

    SavingThrow::SavingThrow(SavingThrowType type) : type_(type)
    {
    }

    SavingThrowAbilityReplacement::SavingThrowAbilityReplacement(SavingThrowAbilityReplacementDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          savingThrowType_(definition.savingThrowType),
          abilityType_(definition.abilityType)
    {
    }

    void SavingThrow::registerResources(ResourceManager &resourceManager, std::vector<std::string> parentResources) const
    {
        resourceManager.registerEnhanceableResource(resourceName(type_), std::move(parentResources));
        resourceManager.registerAccumulatedResource(baseResourceName(type_));
    }

    SavingThrowView SavingThrow::toView(ResourceManager &resourceManager, const std::map<std::string, SavingThrowAbilityReplacement> &abilityReplacements) const
    {
        ModifierSetView modifiers = resourceManager.modifierSetView(resourceName(type_));
        ContributionSetView baseContributions = resourceManager.contributionSetView(baseResourceName(type_));
        const int baseValue = baseContributions.total;
        std::vector<SavingThrowAbilityOptionView> abilityOptions;
        const AbilityType baseAbilityType = defaultAbility(type_);
        const int baseAbilityModifier = resourceManager.targetValue(std::string(resourceName(baseAbilityType)) + "Mod");
        abilityOptions.push_back(SavingThrowAbilityOptionView{
            .replacementId = std::nullopt,
            .source = "Base",
            .abilityType = baseAbilityType,
            .abilityModifier = baseAbilityModifier,
            .totalValue = checkedSavingThrowValue(static_cast<long long>(baseValue) + baseAbilityModifier + modifiers.total)
        });
        for (const auto &[id, replacement] : abilityReplacements)
        {
            if (replacement.savingThrowType_ != type_)
            {
                continue;
            }
            const int abilityModifier = resourceManager.targetValue(std::string(resourceName(replacement.abilityType_)) + "Mod");
            abilityOptions.push_back(SavingThrowAbilityOptionView{
                .replacementId = id,
                .source = replacement.source_,
                .abilityType = replacement.abilityType_,
                .abilityModifier = abilityModifier,
                .totalValue = checkedSavingThrowValue(static_cast<long long>(baseValue) + abilityModifier + modifiers.total)
            });
        }

        return SavingThrowView{
            .type = type_,
            .baseValue = baseValue,
            .baseContributions = std::move(baseContributions),
            .abilityOptions = std::move(abilityOptions),
            .modifiers = std::move(modifiers)
        };
    }

    int SavingThrow::totalValue(ResourceManager &resourceManager) const
    {
        const AbilityType abilityType = defaultAbility(type_);
        const int abilityModifier = resourceManager.targetValue(std::string(resourceName(abilityType)) + "Mod");
        return checkedSavingThrowValue(static_cast<long long>(resourceManager.contributionTotal(baseResourceName(type_))) + abilityModifier + resourceManager.modifierTotal(resourceName(type_)));
    }
}
