#include "golarion/character/combat_maneuvers.hpp"

#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/combat_maneuvers_view.hpp"

#include <algorithm>
#include <array>
#include <limits>
#include <stdexcept>
#include <utility>

namespace
{
    constexpr std::array CombatManeuverTypes{
        golarion::CombatManeuverType::BullRush,
        golarion::CombatManeuverType::DirtyTrick,
        golarion::CombatManeuverType::Disarm,
        golarion::CombatManeuverType::Drag,
        golarion::CombatManeuverType::Grapple,
        golarion::CombatManeuverType::Overrun,
        golarion::CombatManeuverType::Reposition,
        golarion::CombatManeuverType::Steal,
        golarion::CombatManeuverType::Sunder,
        golarion::CombatManeuverType::Trip
    };

    std::string_view resourceSegment(golarion::CombatManeuverType type)
    {
        switch (type)
        {
            case golarion::CombatManeuverType::BullRush:
                return "bullRush";
            case golarion::CombatManeuverType::DirtyTrick:
                return "dirtyTrick";
            case golarion::CombatManeuverType::Disarm:
                return "disarm";
            case golarion::CombatManeuverType::Drag:
                return "drag";
            case golarion::CombatManeuverType::Grapple:
                return "grapple";
            case golarion::CombatManeuverType::Overrun:
                return "overrun";
            case golarion::CombatManeuverType::Reposition:
                return "reposition";
            case golarion::CombatManeuverType::Steal:
                return "steal";
            case golarion::CombatManeuverType::Sunder:
                return "sunder";
            case golarion::CombatManeuverType::Trip:
                return "trip";
        }

        throw std::invalid_argument("unknown combat maneuver type");
    }

    int checkedCombatManeuverValue(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("combat maneuver value is out of range");
        }
        return static_cast<int>(value);
    }

    int abilityModifier(golarion::ResourceManager &resourceManager, golarion::AbilityType type)
    {
        return resourceManager.targetValue(std::string(golarion::resourceName(type)) + "Mod");
    }
}

namespace golarion
{
    std::string_view displayName(CombatManeuverType type)
    {
        switch (type)
        {
            case CombatManeuverType::BullRush:
                return "Spingere";
            case CombatManeuverType::DirtyTrick:
                return "Colpo sporco";
            case CombatManeuverType::Disarm:
                return "Disarmare";
            case CombatManeuverType::Drag:
                return "Trascinare";
            case CombatManeuverType::Grapple:
                return "Lottare";
            case CombatManeuverType::Overrun:
                return "Oltrepassare";
            case CombatManeuverType::Reposition:
                return "Riposizionare";
            case CombatManeuverType::Steal:
                return "Rubare";
            case CombatManeuverType::Sunder:
                return "Spezzare";
            case CombatManeuverType::Trip:
                return "Sbilanciare";
        }

        throw std::invalid_argument("unknown combat maneuver type");
    }

    std::string combatManeuverBonusResourceName(CombatManeuverType type)
    {
        return "combatManeuver.bonus." + std::string(resourceSegment(type));
    }

    std::string combatManeuverDefenseResourceName(CombatManeuverType type)
    {
        return "combatManeuver.defense." + std::string(resourceSegment(type));
    }

    CombatManeuverAbilityReplacement::CombatManeuverAbilityReplacement(CombatManeuverAbilityReplacementDefinition definition)
        : id_(normalize(definition.id)),
          source_(normalize(definition.source)),
          targetResourceName_(normalize(definition.targetResourceName)),
          abilityType_(definition.abilityType)
    {
    }

    CombatManeuverDefenseDexteritySuppression::CombatManeuverDefenseDexteritySuppression(CombatManeuverDefenseDexteritySuppressionDefinition definition)
        : id_(normalize(definition.id)), source_(normalize(definition.source))
    {
    }

    CombatManeuvers::CombatManeuvers(ResourceManager &resourceManager) : resourceManager_(resourceManager)
    {
        resourceManager_.registerEnhanceableResource(CombatManeuverBonusAllResource, {"attack.all"});
        resourceManager_.registerEnhanceableResource(CombatManeuverDefenseAllResource);
        for (const CombatManeuverType type : CombatManeuverTypes)
        {
            resourceManager_.registerEnhanceableResource(combatManeuverBonusResourceName(type), {std::string(CombatManeuverBonusAllResource)});
            resourceManager_.registerEnhanceableResource(combatManeuverDefenseResourceName(type), {std::string(CombatManeuverDefenseAllResource)});
        }
        resourceManager_.registerCollectionResource<CombatManeuverAbilityReplacement>(CombatManeuverAbilityReplacementsResource, [this](CombatManeuverAbilityReplacement replacement)
        {
            addAbilityReplacement(std::move(replacement));
        }, [this](std::string_view replacementId)
        {
            removeAbilityReplacement(replacementId);
        });
        resourceManager_.registerCollectionResource<CombatManeuverDefenseDexteritySuppression>(CombatManeuverDefenseDexteritySuppressionsResource, [this](CombatManeuverDefenseDexteritySuppression suppression)
        {
            addDexteritySuppression(std::move(suppression));
        }, [this](std::string_view suppressionId)
        {
            removeDexteritySuppression(suppressionId);
        });
    }

    CombatManeuversView CombatManeuvers::toView()
    {
        const int baseAttackBonus = resourceManager_.targetValue("bab");
        const int strengthModifier = abilityModifier(resourceManager_, AbilityType::Strength);
        const int dexterityModifier = abilityModifier(resourceManager_, AbilityType::Dexterity);
        const bool dexterityBonusSuppressed = !dexteritySuppressions_.empty();
        const int appliedDexterityModifier = dexterityBonusSuppressed ? std::min(dexterityModifier, 0) : dexterityModifier;
        std::vector<CombatManeuverDefenseDexteritySuppressionView> dexteritySuppressionViews;
        dexteritySuppressionViews.reserve(dexteritySuppressions_.size());
        for (const auto &[id, suppression] : dexteritySuppressions_)
        {
            dexteritySuppressionViews.push_back(CombatManeuverDefenseDexteritySuppressionView{
                .id = id,
                .source = suppression.source_
            });
        }
        std::vector<CombatManeuverView> maneuverViews;
        maneuverViews.reserve(CombatManeuverTypes.size());

        for (const CombatManeuverType type : CombatManeuverTypes)
        {
            const std::string bonusResource = combatManeuverBonusResourceName(type);
            ModifierSetView bonusModifiers = resourceManager_.modifierSetView(bonusResource);
            std::vector<CombatManeuverAbilityOptionView> abilityOptions;
            abilityOptions.reserve(abilityReplacements_.size() + 1);
            abilityOptions.push_back(CombatManeuverAbilityOptionView{
                .replacementId = std::nullopt,
                .source = "Base",
                .abilityType = AbilityType::Strength,
                .abilityModifier = strengthModifier,
                .totalValue = checkedCombatManeuverValue(static_cast<long long>(baseAttackBonus) + strengthModifier + bonusModifiers.total)
            });
            for (const auto &[id, replacement] : abilityReplacements_)
            {
                if (!resourceManager_.enhanceableResourceIsOrInheritsFrom(bonusResource, replacement.targetResourceName_))
                {
                    continue;
                }
                const int replacementModifier = abilityModifier(resourceManager_, replacement.abilityType_);
                abilityOptions.push_back(CombatManeuverAbilityOptionView{
                    .replacementId = id,
                    .source = replacement.source_,
                    .abilityType = replacement.abilityType_,
                    .abilityModifier = replacementModifier,
                    .totalValue = checkedCombatManeuverValue(static_cast<long long>(baseAttackBonus) + replacementModifier + bonusModifiers.total)
                });
            }

            ModifierSetView defenseModifiers = resourceManager_.modifierSetView(combatManeuverDefenseResourceName(type));
            maneuverViews.push_back(CombatManeuverView{
                .type = type,
                .bonus = CombatManeuverBonusView{
                    .abilityOptions = std::move(abilityOptions),
                    .modifiers = std::move(bonusModifiers)
                },
                .defense = CombatManeuverDefenseView{
                    .strengthModifier = strengthModifier,
                    .dexterityModifier = dexterityModifier,
                    .appliedDexterityModifier = appliedDexterityModifier,
                    .dexterityBonusSuppressed = dexterityBonusSuppressed,
                    .dexteritySuppressions = dexteritySuppressionViews,
                    .totalValue = checkedCombatManeuverValue(10LL + baseAttackBonus + strengthModifier + appliedDexterityModifier + defenseModifiers.total),
                    .modifiers = std::move(defenseModifiers)
                }
            });
        }

        return CombatManeuversView{
            .baseAttackBonus = baseAttackBonus,
            .maneuvers = std::move(maneuverViews)
        };
    }

    void CombatManeuvers::addAbilityReplacement(CombatManeuverAbilityReplacement replacement)
    {
        if (!resourceManager_.enhanceableResourceIsOrInheritsFrom(replacement.targetResourceName_, CombatManeuverBonusAllResource))
        {
            throw std::invalid_argument("combat maneuver ability replacement must target a combat maneuver bonus resource: " + replacement.targetResourceName_);
        }

        const std::string id = replacement.id_;
        if (!abilityReplacements_.emplace(id, std::move(replacement)).second)
        {
            throw std::invalid_argument("combat maneuver ability replacement is already registered: " + id);
        }
    }

    void CombatManeuvers::removeAbilityReplacement(std::string_view replacementId)
    {
        const std::string id = normalize(replacementId);
        if (abilityReplacements_.erase(id) == 0)
        {
            throw std::invalid_argument("combat maneuver ability replacement is not registered: " + id);
        }
    }

    void CombatManeuvers::addDexteritySuppression(CombatManeuverDefenseDexteritySuppression suppression)
    {
        const std::string id = suppression.id_;
        if (!dexteritySuppressions_.emplace(id, std::move(suppression)).second)
        {
            throw std::invalid_argument("combat maneuver defense Dexterity suppression is already registered: " + id);
        }
    }

    void CombatManeuvers::removeDexteritySuppression(std::string_view suppressionId)
    {
        const std::string id = normalize(suppressionId);
        if (dexteritySuppressions_.erase(id) == 0)
        {
            throw std::invalid_argument("combat maneuver defense Dexterity suppression is not registered: " + id);
        }
    }
}
