#pragma once

#include "golarion/character/ability.hpp"
#include "golarion/character/armor_class.hpp"
#include "golarion/character/combat_maneuvers.hpp"
#include "golarion/character/condition.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/strike.hpp"
#include "golarion/resource/contribution.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <optional>
#include <string>
#include <utility>

namespace golarion::conditionEffects
{
    inline ConditionEffectDefinition modifier(std::string id, std::string description, std::string source, std::string resource, ModifierType type, std::optional<BonusType> bonusType, std::string expression, std::optional<std::string> condition = std::nullopt)
    {
        return ConditionEffectDefinition{
            .id = std::move(id),
            .description = description,
            .apply = [description = std::move(description), source = std::move(source), resource = std::move(resource), type, bonusType, expression = std::move(expression), condition = std::move(condition)](ResourceManager &manager, const ConditionEffectContext &)
            {
                Modifier appliedModifier(type, source, description, bonusType, expression, condition);
                const std::string modifierId = appliedModifier.id();
                manager.addModifier(resource, std::move(appliedModifier));
                return ConditionCleanup([&manager, resource, modifierId]
                {
                    manager.removeModifier(resource, modifierId);
                });
            }
        };
    }

    inline ConditionEffectDefinition penalty(std::string id, std::string description, std::string source, std::string resource, std::string expression, std::optional<std::string> condition = std::nullopt)
    {
        return modifier(std::move(id), std::move(description), std::move(source), std::move(resource), ModifierType::Penalty, std::nullopt, std::move(expression), std::move(condition));
    }

    inline ConditionEffectDefinition perEntryPenalty(std::string id, std::string description, std::string fallbackSource, std::string resource, std::string expression)
    {
        return ConditionEffectDefinition{
            .id = std::move(id),
            .description = description,
            .apply = [description = std::move(description), fallbackSource = std::move(fallbackSource), resource = std::move(resource), expression = std::move(expression)](ResourceManager &manager, const ConditionEffectContext &context)
            {
                Modifier appliedModifier(ModifierType::Penalty, context.source.value_or(fallbackSource), description, std::nullopt, expression);
                const std::string modifierId = appliedModifier.id();
                manager.addModifier(resource, std::move(appliedModifier));
                return ConditionCleanup([&manager, resource, modifierId]
                {
                    manager.removeModifier(resource, modifierId);
                });
            }
        };
    }

    inline ConditionEffectDefinition genericBonus(std::string id, std::string description, std::string source, std::string resource, std::string expression, std::optional<std::string> condition = std::nullopt)
    {
        return modifier(std::move(id), std::move(description), std::move(source), std::move(resource), ModifierType::Bonus, BonusType::Generic, std::move(expression), std::move(condition));
    }

    inline ConditionEffectDefinition movementAdjustment(std::string id, std::string description, std::string source, MovementAdjustmentType type, std::optional<std::string> expression)
    {
        return ConditionEffectDefinition{
            .id = std::move(id),
            .description = description,
            .apply = [description = std::move(description), source = std::move(source), type, expression = std::move(expression)](ResourceManager &manager, const ConditionEffectContext &context)
            {
                manager.addToCollection(MovementAdjustmentsResource, MovementAdjustment(MovementAdjustmentDefinition{
                    .id = context.instanceId,
                    .source = source,
                    .description = description,
                    .type = type,
                    .selector = MovementSelector{},
                    .expression = expression,
                    .condition = std::nullopt
                }));
                const std::string adjustmentId = context.instanceId;
                return ConditionCleanup([&manager, adjustmentId]
                {
                    manager.removeFromCollection(MovementAdjustmentsResource, adjustmentId);
                });
            }
        };
    }

    inline ConditionEffectDefinition armorClassAbilitySuppression(std::string id, std::string description, std::string source)
    {
        return ConditionEffectDefinition{
            .id = std::move(id),
            .description = std::move(description),
            .apply = [source = std::move(source)](ResourceManager &manager, const ConditionEffectContext &context)
            {
                manager.addToCollection(ArmorClassAbilitySuppressionsResource, ArmorClassAbilitySuppression(ArmorClassAbilitySuppressionDefinition{
                    .id = context.instanceId,
                    .source = source
                }));
                const std::string suppressionId = context.instanceId;
                return ConditionCleanup([&manager, suppressionId]
                {
                    manager.removeFromCollection(ArmorClassAbilitySuppressionsResource, suppressionId);
                });
            }
        };
    }

    inline ConditionEffectDefinition combatManeuverDefenseDexteritySuppression(std::string id, std::string description, std::string source)
    {
        return ConditionEffectDefinition{
            .id = std::move(id),
            .description = std::move(description),
            .apply = [source = std::move(source)](ResourceManager &manager, const ConditionEffectContext &context)
            {
                manager.addToCollection(CombatManeuverDefenseDexteritySuppressionsResource, CombatManeuverDefenseDexteritySuppression(CombatManeuverDefenseDexteritySuppressionDefinition{
                    .id = context.instanceId,
                    .source = source
                }));
                const std::string suppressionId = context.instanceId;
                return ConditionCleanup([&manager, suppressionId]
                {
                    manager.removeFromCollection(CombatManeuverDefenseDexteritySuppressionsResource, suppressionId);
                });
            }
        };
    }

    inline ConditionEffectDefinition finalAbilityReplacement(std::string id, std::string description, std::string source, AbilityType ability, std::string expression)
    {
        return ConditionEffectDefinition{
            .id = std::move(id),
            .description = std::move(description),
            .apply = [source = std::move(source), ability, expression = std::move(expression)](ResourceManager &manager, const ConditionEffectContext &context)
            {
                const std::string resource = abilityReplacementsResourceName(ability);
                manager.addToCollection(resource, AbilityReplacement(AbilityReplacementDefinition{
                    .id = context.instanceId,
                    .source = source,
                    .expression = expression,
                    .stage = AbilityReplacementStage::Final,
                    .requirements = {}
                }));
                const std::string replacementId = context.instanceId;
                return ConditionCleanup([&manager, resource, replacementId]
                {
                    manager.removeFromCollection(resource, replacementId);
                });
            }
        };
    }

    inline ConditionEffectDefinition attackDefenseReplacement(std::string id, std::string description, std::string source, std::string targetResource, ArmorClassType defenseType, std::optional<std::string> condition)
    {
        return ConditionEffectDefinition{
            .id = std::move(id),
            .description = std::move(description),
            .apply = [source = std::move(source), targetResource = std::move(targetResource), defenseType, condition = std::move(condition)](ResourceManager &manager, const ConditionEffectContext &context)
            {
                manager.addToCollection(AttackDefenseReplacementsResource, AttackDefenseReplacement(AttackDefenseReplacementDefinition{
                    .id = context.instanceId,
                    .source = source,
                    .targetResourceName = targetResource,
                    .defenseType = defenseType,
                    .condition = condition
                }));
                const std::string replacementId = context.instanceId;
                return ConditionCleanup([&manager, replacementId]
                {
                    manager.removeFromCollection(AttackDefenseReplacementsResource, replacementId);
                });
            }
        };
    }

    inline ConditionEffectDefinition contribution(std::string id, std::string description, std::string resource, std::string expression)
    {
        return ConditionEffectDefinition{
            .id = std::move(id),
            .description = std::move(description),
            .apply = [resource = std::move(resource), expression = std::move(expression)](ResourceManager &manager, const ConditionEffectContext &context)
            {
                manager.addContribution(resource, Contribution(context.instanceId, expression));
                const std::string contributionId = context.instanceId;
                return ConditionCleanup([&manager, resource, contributionId]
                {
                    manager.removeContribution(resource, contributionId);
                });
            }
        };
    }
}
