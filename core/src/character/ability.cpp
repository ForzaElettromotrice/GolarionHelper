#include "golarion/character/ability.hpp"

#include "golarion/data/ability_save_data.hpp"
#include "golarion/view/ability_view.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    int checkedInt(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("ability value is out of range");
        }
        return static_cast<int>(value);
    }

    int floorDivideByTwo(long long value)
    {
        long long quotient = value / 2;
        if (value < 0 && value % 2 != 0)
        {
            --quotient;
        }
        return checkedInt(quotient);
    }
}

namespace golarion
{
    std::string_view displayName(AbilityType type)
    {
        switch (type)
        {
            case AbilityType::Strength:
                return "Forza";
            case AbilityType::Dexterity:
                return "Destrezza";
            case AbilityType::Constitution:
                return "Costituzione";
            case AbilityType::Intelligence:
                return "Intelligenza";
            case AbilityType::Wisdom:
                return "Saggezza";
            case AbilityType::Charisma:
                return "Carisma";
        }

        throw std::invalid_argument("unknown ability type");
    }

    std::string_view displayName(AbilityReplacementStage stage)
    {
        switch (stage)
        {
            case AbilityReplacementStage::Base:
                return "Valore base";
            case AbilityReplacementStage::Final:
                return "Valore finale";
        }

        throw std::invalid_argument("unknown ability replacement stage");
    }

    std::string_view resourceName(AbilityType type)
    {
        switch (type)
        {
            case AbilityType::Strength:
                return "str";
            case AbilityType::Dexterity:
                return "dex";
            case AbilityType::Constitution:
                return "con";
            case AbilityType::Intelligence:
                return "int";
            case AbilityType::Wisdom:
                return "wis";
            case AbilityType::Charisma:
                return "cha";
        }

        throw std::invalid_argument("unknown ability type");
    }

    std::string abilityCheckResourceName(AbilityType type)
    {
        return "abilityCheck." + std::string(resourceName(type));
    }

    std::string abilityReplacementsResourceName(AbilityType type)
    {
        return "ability." + std::string(resourceName(type)) + ".replacements";
    }

    AbilityReplacement::AbilityReplacement(AbilityReplacementDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          expression_(normalize(definition.expression)),
          stage_(definition.stage),
          requirements_(std::move(definition.requirements))
    {
    }

    AbilityChecks::AbilityChecks(ResourceManager &resourceManager)
    {
        resourceManager.registerEnhanceableResource(AbilityCheckRootResource);
        for (AbilityType type : {AbilityType::Strength, AbilityType::Dexterity, AbilityType::Constitution, AbilityType::Intelligence, AbilityType::Wisdom, AbilityType::Charisma})
        {
            resourceManager.registerEnhanceableResource(abilityCheckResourceName(type), {std::string(AbilityCheckRootResource)});
        }
    }

    AbilityScore::AbilityScore(AbilityType type, int baseValue) : type_(type), baseValue_(10)
    {
        setBaseValue(baseValue);
    }

    void AbilityScore::setBaseValue(int baseValue)
    {
        if (baseValue <= 0)
        {
            throw std::invalid_argument("base value must be greater than 0");
        }
        baseValue_ = baseValue;
    }

    void AbilityScore::registerResources(ResourceManager &resourceManager) const
    {
        const std::string resource(resourceName(type_));
        ResourceManager *manager = &resourceManager;

        resourceManager.registerEnhanceableResource(resource);
        resourceManager.registerTarget(resource, [this, manager]
        {
            return totalValue(*manager);
        });
        resourceManager.registerTarget(resource + "Mod", [this, manager]
        {
            return modifier(*manager);
        });
        resourceManager.registerCollectionResource<AbilityReplacement>(abilityReplacementsResourceName(type_), [this](AbilityReplacement replacement)
        {
            addReplacement(std::move(replacement));
        }, [this](std::string_view replacementId)
        {
            removeReplacement(replacementId);
        });
    }

    AbilityView AbilityScore::toView(ResourceManager &resourceManager) const
    {
        const std::optional<int> baseReplacement = replacementValue(AbilityReplacementStage::Base, resourceManager);
        const std::optional<int> finalReplacement = replacementValue(AbilityReplacementStage::Final, resourceManager);
        ModifierSetView modifiers = resourceManager.modifierSetView(resourceName(type_));
        const int effectiveBaseValue = baseReplacement.value_or(baseValue_);
        const int modifiedValue = checkedInt(static_cast<long long>(effectiveBaseValue) + modifiers.total);
        const int total = finalReplacement.value_or(modifiedValue);
        const int abilityModifier = floorDivideByTwo(static_cast<long long>(total) - 10);
        std::vector<AbilityReplacementView> replacementViews;
        replacementViews.reserve(replacements_.size());
        for (const auto &[id, replacement] : replacements_)
        {
            std::vector<RequirementView> requirementViews;
            requirementViews.reserve(replacement.requirements_.size());
            bool active = true;
            for (const Requirement &requirement : replacement.requirements_)
            {
                RequirementView requirementView = requirement.toView(resourceManager);
                active = active && requirementView.satisfied;
                requirementViews.push_back(std::move(requirementView));
            }
            const int resolvedValue = resourceManager.evaluateExpression(replacement.expression_);
            const std::optional<int> selectedValue = replacement.stage_ == AbilityReplacementStage::Base ? baseReplacement : finalReplacement;
            replacementViews.push_back(AbilityReplacementView{
                .id = id,
                .source = replacement.source_,
                .expression = replacement.expression_,
                .stage = replacement.stage_,
                .resolvedValue = resolvedValue,
                .active = active,
                .applied = active && selectedValue.has_value() && *selectedValue == resolvedValue,
                .requirements = std::move(requirementViews)
            });
        }
        ModifierSetView checkModifiers = resourceManager.modifierSetView(abilityCheckResourceName(type_));

        return AbilityView{
            .type = type_,
            .baseValue = baseValue_,
            .effectiveBaseValue = effectiveBaseValue,
            .modifiedValue = modifiedValue,
            .totalValue = total,
            .modifier = abilityModifier,
            .modifiers = std::move(modifiers),
            .replacements = std::move(replacementViews),
            .checkTotal = checkedInt(static_cast<long long>(abilityModifier) + checkModifiers.total),
            .checkModifiers = std::move(checkModifiers)
        };
    }

    AbilitySaveData AbilityScore::toSaveData() const
    {
        return AbilitySaveData{
            .type = type_,
            .baseValue = baseValue_
        };
    }

    int AbilityScore::baseValue() const
    {
        return baseValue_;
    }

    int AbilityScore::totalValue(ResourceManager &resourceManager) const
    {
        const int effectiveBaseValue = replacementValue(AbilityReplacementStage::Base, resourceManager).value_or(baseValue_);
        const int modifiedValue = checkedInt(static_cast<long long>(effectiveBaseValue) + resourceManager.modifierTotal(resourceName(type_)));
        return replacementValue(AbilityReplacementStage::Final, resourceManager).value_or(modifiedValue);
    }

    int AbilityScore::modifier(ResourceManager &resourceManager) const
    {
        return floorDivideByTwo(static_cast<long long>(totalValue(resourceManager)) - 10);
    }

    std::optional<int> AbilityScore::replacementValue(AbilityReplacementStage stage, ResourceManager &resourceManager) const
    {
        std::optional<int> value;
        for (const auto &[id, replacement] : replacements_)
        {
            if (replacement.stage_ != stage || !std::ranges::all_of(replacement.requirements_, [&resourceManager](const Requirement &requirement)
            {
                return requirement.isSatisfied(resourceManager);
            }))
            {
                continue;
            }

            const int resolvedValue = resourceManager.evaluateExpression(replacement.expression_);
            if (stage == AbilityReplacementStage::Base && resolvedValue <= 0)
            {
                throw std::invalid_argument("base ability replacement must resolve to a positive value: " + id);
            }
            if (stage == AbilityReplacementStage::Final && resolvedValue < 0)
            {
                throw std::invalid_argument("final ability replacement must not resolve to a negative value: " + id);
            }
            if (!value.has_value()
                || (stage == AbilityReplacementStage::Base && resolvedValue > *value)
                || (stage == AbilityReplacementStage::Final && resolvedValue < *value))
            {
                value = resolvedValue;
            }
        }
        return value;
    }

    void AbilityScore::addReplacement(AbilityReplacement replacement) const
    {
        const std::string id = replacement.id_;
        if (!replacements_.emplace(id, std::move(replacement)).second)
        {
            throw std::invalid_argument("ability replacement is already registered: " + id);
        }
    }

    void AbilityScore::removeReplacement(std::string_view replacementId) const
    {
        const std::string id = normalize(replacementId);
        if (replacements_.erase(id) == 0)
        {
            throw std::invalid_argument("ability replacement is not registered: " + id);
        }
    }
}
