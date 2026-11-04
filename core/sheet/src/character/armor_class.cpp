#include "golarion/character/armor_class.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/armor_class_view.hpp"

#include <algorithm>
#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    int checkedArmorClassValue(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("armor class value is out of range");
        }
        return static_cast<int>(value);
    }

    std::vector<golarion::ArmorClassValueView> armorClassValues(golarion::ResourceManager &resourceManager, golarion::AbilityType abilityType, const std::optional<int> maximumDexterityBonus, bool abilityBonusSuppressed)
    {
        const int abilityModifier = resourceManager.targetValue(std::string(golarion::resourceName(abilityType)) + "Mod");
        std::vector<golarion::ArmorClassValueView> values;
        values.reserve(3);

        for (const golarion::ArmorClassType type : {golarion::ArmorClassType::Normal, golarion::ArmorClassType::Touch, golarion::ArmorClassType::FlatFooted})
        {
            golarion::ModifierSetView modifiers = resourceManager.modifierSetView(golarion::resourceName(type));
            int appliedAbilityModifier = type == golarion::ArmorClassType::FlatFooted
                ? std::min(abilityModifier, 0)
                : maximumDexterityBonus.has_value() ? std::min(abilityModifier, *maximumDexterityBonus) : abilityModifier;
            if (abilityBonusSuppressed)
            {
                appliedAbilityModifier = std::min(appliedAbilityModifier, 0);
            }
            values.push_back(golarion::ArmorClassValueView{
                .type = type,
                .appliedAbilityModifier = appliedAbilityModifier,
                .totalValue = checkedArmorClassValue(10LL + appliedAbilityModifier + modifiers.total),
                .modifiers = std::move(modifiers)
            });
        }

        return values;
    }
}

namespace golarion
{
    std::string_view displayName(ArmorClassType type)
    {
        switch (type)
        {
            case ArmorClassType::Normal:
                return "Classe Armatura";
            case ArmorClassType::Touch:
                return "Contatto";
            case ArmorClassType::FlatFooted:
                return "Impreparato";
        }
        throw std::invalid_argument("unknown armor class type");
    }

    std::string_view resourceName(ArmorClassType type)
    {
        switch (type)
        {
            case ArmorClassType::Normal:
                return "armorClass.normal";
            case ArmorClassType::Touch:
                return "armorClass.touch";
            case ArmorClassType::FlatFooted:
                return "armorClass.flatFooted";
        }
        throw std::invalid_argument("unknown armor class type");
    }

    std::string_view displayName(MaximumDexterityLimitType type)
    {
        switch (type)
        {
            case MaximumDexterityLimitType::Armor:
                return "Armatura";
            case MaximumDexterityLimitType::Shield:
                return "Scudo";
            case MaximumDexterityLimitType::Load:
                return "Carico";
            case MaximumDexterityLimitType::Other:
                return "Altro";
        }
        throw std::invalid_argument("unknown maximum Dexterity limit type");
    }

    std::string_view maximumDexterityResourceName(MaximumDexterityLimitType type)
    {
        switch (type)
        {
            case MaximumDexterityLimitType::Armor:
                return "armorClass.maxDex.armor";
            case MaximumDexterityLimitType::Shield:
                return "armorClass.maxDex.shield";
            case MaximumDexterityLimitType::Load:
                return "armorClass.maxDex.load";
            case MaximumDexterityLimitType::Other:
                return "armorClass.maxDex.other";
        }
        throw std::invalid_argument("unknown maximum Dexterity limit type");
    }

    std::string maximumDexterityResourceName(std::string_view limitId)
    {
        return "armorClass.maxDex.limit." + normalize(limitId);
    }

    MaximumDexterityLimit::MaximumDexterityLimit(MaximumDexterityLimitDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          type_(definition.type),
          expression_(normalize(definition.expression))
    {
    }

    ArmorClassAbilityReplacement::ArmorClassAbilityReplacement(ArmorClassAbilityReplacementDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          abilityType_(definition.abilityType)
    {
    }

    ArmorClassAbilitySuppression::ArmorClassAbilitySuppression(ArmorClassAbilitySuppressionDefinition definition)
        : id_(normalize(definition.id)), source_(normalize(definition.source))
    {
    }

    ArmorClass::ArmorClass(ResourceManager &resourceManager)
        : resourceManager_(resourceManager)
    {
        resourceManager_.registerEnhanceableResource(ArmorClassAllResource);
        resourceManager_.registerEnhanceableResource(ArmorClassReflexiveResource, {std::string(ArmorClassAllResource)});
        resourceManager_.registerEnhanceableResource(ArmorClassSolidResource, {std::string(ArmorClassAllResource)});
        resourceManager_.registerEnhanceableResource(resourceName(ArmorClassType::Normal), {std::string(ArmorClassReflexiveResource), std::string(ArmorClassSolidResource)});
        resourceManager_.registerEnhanceableResource(resourceName(ArmorClassType::Touch), {std::string(ArmorClassReflexiveResource)});
        resourceManager_.registerEnhanceableResource(resourceName(ArmorClassType::FlatFooted), {std::string(ArmorClassSolidResource)});
        resourceManager_.registerCollectionResource<ArmorClassAbilityReplacement>(ArmorClassAbilityReplacementsResource, [this](ArmorClassAbilityReplacement replacement)
        {
            addAbilityReplacement(std::move(replacement));
        }, [this](std::string_view replacementId)
        {
            removeAbilityReplacement(replacementId);
        });
        resourceManager_.registerCollectionResource<ArmorClassAbilitySuppression>(ArmorClassAbilitySuppressionsResource, [this](ArmorClassAbilitySuppression suppression)
        {
            addAbilitySuppression(std::move(suppression));
        }, [this](std::string_view suppressionId)
        {
            removeAbilitySuppression(suppressionId);
        });
        resourceManager_.registerEnhanceableResource(MaximumDexterityAllResource);
        for (const MaximumDexterityLimitType type : {MaximumDexterityLimitType::Armor, MaximumDexterityLimitType::Shield, MaximumDexterityLimitType::Load, MaximumDexterityLimitType::Other})
        {
            resourceManager_.registerEnhanceableResource(maximumDexterityResourceName(type), {std::string(MaximumDexterityAllResource)});
        }
        resourceManager_.registerCollectionResource<MaximumDexterityLimit>(MaximumDexterityLimitsResource, [this](MaximumDexterityLimit limit)
        {
            addMaximumDexterityLimit(std::move(limit));
        }, [this](std::string_view limitId)
        {
            removeMaximumDexterityLimit(limitId);
        });
    }

    std::optional<int> ArmorClass::maximumDexterityBonus()
    {
        std::optional<int> maximum;
        for (const auto &[id, limit] : maximumDexterityLimits_)
        {
            const int baseValue = resourceManager_.evaluateExpression(limit.expression_);
            if (baseValue < 0)
            {
                throw std::invalid_argument("maximum Dexterity limit expression must not resolve to a negative value: " + limit.expression_);
            }
            const int effectiveValue = std::max(checkedArmorClassValue(static_cast<long long>(baseValue) + resourceManager_.modifierTotal(maximumDexterityResourceName(id))), 0);
            maximum = maximum.has_value() ? std::min(*maximum, effectiveValue) : effectiveValue;
        }
        return maximum;
    }

    ArmorClassView ArmorClass::toView()
    {
        std::optional<int> maximum;
        std::vector<MaximumDexterityLimitView> maximumDexterityLimits;
        maximumDexterityLimits.reserve(maximumDexterityLimits_.size());
        for (const auto &[id, limit] : maximumDexterityLimits_)
        {
            const int baseValue = resourceManager_.evaluateExpression(limit.expression_);
            if (baseValue < 0)
            {
                throw std::invalid_argument("maximum Dexterity limit expression must not resolve to a negative value: " + limit.expression_);
            }
            ModifierSetView modifiers = resourceManager_.modifierSetView(maximumDexterityResourceName(id));
            const int effectiveValue = std::max(checkedArmorClassValue(static_cast<long long>(baseValue) + modifiers.total), 0);
            maximum = maximum.has_value() ? std::min(*maximum, effectiveValue) : effectiveValue;
            maximumDexterityLimits.push_back(MaximumDexterityLimitView{
                .id = id,
                .source = limit.source_,
                .type = limit.type_,
                .baseValue = baseValue,
                .effectiveValue = effectiveValue,
                .modifiers = std::move(modifiers)
            });
        }

        std::vector<ArmorClassAbilitySuppressionView> abilitySuppressionViews;
        abilitySuppressionViews.reserve(abilitySuppressions_.size());
        for (const auto &[id, suppression] : abilitySuppressions_)
        {
            abilitySuppressionViews.push_back(ArmorClassAbilitySuppressionView{
                .id = id,
                .source = suppression.source_
            });
        }
        const bool abilityBonusSuppressed = !abilitySuppressions_.empty();

        std::vector<ArmorClassAbilityOptionView> abilityOptions;
        abilityOptions.reserve(abilityReplacements_.size() + 1);
        const int dexterityModifier = resourceManager_.targetValue("dexMod");
        abilityOptions.push_back(ArmorClassAbilityOptionView{
            .replacementId = std::nullopt,
            .source = "Base",
            .abilityType = AbilityType::Dexterity,
            .abilityModifier = dexterityModifier,
            .values = armorClassValues(resourceManager_, AbilityType::Dexterity, maximum, abilityBonusSuppressed)
        });
        for (const auto &[id, replacement] : abilityReplacements_)
        {
            const int abilityModifier = resourceManager_.targetValue(std::string(resourceName(replacement.abilityType_)) + "Mod");
            abilityOptions.push_back(ArmorClassAbilityOptionView{
                .replacementId = id,
                .source = replacement.source_,
                .abilityType = replacement.abilityType_,
                .abilityModifier = abilityModifier,
                .values = armorClassValues(resourceManager_, replacement.abilityType_, maximum, abilityBonusSuppressed)
            });
        }

        return ArmorClassView{
            .maximumDexterityBonus = maximum,
            .maximumDexterityLimits = std::move(maximumDexterityLimits),
            .abilityBonusSuppressed = abilityBonusSuppressed,
            .abilitySuppressions = std::move(abilitySuppressionViews),
            .abilityOptions = std::move(abilityOptions)
        };
    }

    void ArmorClass::addAbilityReplacement(ArmorClassAbilityReplacement replacement)
    {
        const std::string id = replacement.id_;
        if (!abilityReplacements_.emplace(id, std::move(replacement)).second)
        {
            throw std::invalid_argument("armor class ability replacement is already registered: " + id);
        }
    }

    void ArmorClass::removeAbilityReplacement(std::string_view replacementId)
    {
        const std::string id = normalize(replacementId);
        if (abilityReplacements_.erase(id) == 0)
        {
            throw std::invalid_argument("armor class ability replacement is not registered: " + id);
        }
    }

    void ArmorClass::addAbilitySuppression(ArmorClassAbilitySuppression suppression)
    {
        const std::string id = suppression.id_;
        if (!abilitySuppressions_.emplace(id, std::move(suppression)).second)
        {
            throw std::invalid_argument("armor class ability suppression is already registered: " + id);
        }
    }

    void ArmorClass::removeAbilitySuppression(std::string_view suppressionId)
    {
        const std::string id = normalize(suppressionId);
        if (abilitySuppressions_.erase(id) == 0)
        {
            throw std::invalid_argument("armor class ability suppression is not registered: " + id);
        }
    }

    void ArmorClass::addMaximumDexterityLimit(MaximumDexterityLimit limit)
    {
        const std::string id = limit.id_;
        if (maximumDexterityLimits_.contains(id))
        {
            throw std::invalid_argument("maximum Dexterity limit is already registered: " + id);
        }

        const MaximumDexterityLimitType type = limit.type_;
        maximumDexterityLimits_.emplace(id, std::move(limit));
        try
        {
            resourceManager_.registerEnhanceableResource(maximumDexterityResourceName(id), {std::string(maximumDexterityResourceName(type))});
        }
        catch (...)
        {
            maximumDexterityLimits_.erase(id);
            throw;
        }
    }

    void ArmorClass::removeMaximumDexterityLimit(std::string_view limitId)
    {
        const std::string id = normalize(limitId);
        if (!maximumDexterityLimits_.contains(id))
        {
            throw std::invalid_argument("maximum Dexterity limit is not registered: " + id);
        }

        resourceManager_.unregisterEnhanceableResource(maximumDexterityResourceName(id));
        maximumDexterityLimits_.erase(id);
    }

}
