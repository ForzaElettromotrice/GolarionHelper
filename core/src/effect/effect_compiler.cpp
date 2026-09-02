#include "golarion/effect/effect_compiler.hpp"

#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/reminder.hpp"
#include "golarion/character/size.hpp"
#include "golarion/resource/contribution.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"

#include <type_traits>
#include <utility>
#include <variant>

namespace golarion
{
    EffectApply compileEffect(EffectDefinition definition, EffectContext context)
    {
        context.instanceId = normalize(context.instanceId);
        context.source = normalize(context.source);
        std::visit([](const auto &effect)
        {
            static_cast<void>(normalize(effect.id));
        }, definition);

        return std::visit([context = std::move(context)](auto effect) -> EffectApply
        {
            using Effect = std::decay_t<decltype(effect)>;

            if constexpr (std::is_same_v<Effect, ModifierEffectDefinition>)
            {
                return [effect = std::move(effect), context](ResourceManager &resourceManager)
                {
                    Modifier modifier(effect.type, context.source, effect.description, effect.bonusType, effect.expression, effect.condition);
                    const std::string modifierId = modifier.id();
                    resourceManager.addModifier(effect.resource, std::move(modifier));
                    return EffectCleanup([&resourceManager, resource = effect.resource, modifierId]
                    {
                        resourceManager.removeModifier(resource, modifierId);
                    });
                };
            }
            else if constexpr (std::is_same_v<Effect, ContributionEffectDefinition>)
            {
                return [effect = std::move(effect), context](ResourceManager &resourceManager)
                {
                    resourceManager.addContribution(effect.resource, Contribution(context.instanceId, effect.expression));
                    return EffectCleanup([&resourceManager, resource = effect.resource, contributionId = context.instanceId]
                    {
                        resourceManager.removeContribution(resource, contributionId);
                    });
                };
            }
            else if constexpr (std::is_same_v<Effect, SizeBaseEffectDefinition>)
            {
                return [effect = std::move(effect), context](ResourceManager &resourceManager)
                {
                    resourceManager.addToCollection(SizeBaseResource, SizeBase(SizeBaseDefinition{
                        .id = context.instanceId,
                        .source = context.source,
                        .category = effect.category
                    }));
                    return EffectCleanup([&resourceManager, instanceId = context.instanceId]
                    {
                        resourceManager.removeFromCollection(SizeBaseResource, instanceId);
                    });
                };
            }
            else if constexpr (std::is_same_v<Effect, MovementGrantEffectDefinition>)
            {
                return [effect = std::move(effect), context](ResourceManager &resourceManager)
                {
                    resourceManager.addToCollection(MovementGrantsResource, MovementGrant(MovementGrantDefinition{
                        .id = context.instanceId,
                        .source = context.source,
                        .type = effect.type,
                        .baseSpeedExpression = effect.baseSpeedExpression,
                        .maneuverability = effect.maneuverability,
                        .affectedByArmor = effect.affectedByArmor,
                        .affectedByLoad = effect.affectedByLoad,
                        .supportsRunning = effect.supportsRunning
                    }));
                    return EffectCleanup([&resourceManager, instanceId = context.instanceId]
                    {
                        resourceManager.removeFromCollection(MovementGrantsResource, instanceId);
                    });
                };
            }
            else if constexpr (std::is_same_v<Effect, CarryingBodyTypeEffectDefinition>)
            {
                return [effect = std::move(effect), context](ResourceManager &resourceManager)
                {
                    resourceManager.addToCollection(CarryingCapacityBodyTypeResource, CarryingBodyTypeBase(CarryingBodyTypeBaseDefinition{
                        .id = context.instanceId,
                        .source = context.source,
                        .type = effect.type
                    }));
                    return EffectCleanup([&resourceManager, instanceId = context.instanceId]
                    {
                        resourceManager.removeFromCollection(CarryingCapacityBodyTypeResource, instanceId);
                    });
                };
            }
            else
            {
                static_assert(std::is_same_v<Effect, ReminderEffectDefinition>);
                return [effect = std::move(effect), context](ResourceManager &resourceManager)
                {
                    resourceManager.addToCollection(ReminderEntriesResource, Reminder(ReminderDefinition{
                        .id = context.instanceId,
                        .message = effect.message
                    }));
                    return EffectCleanup([&resourceManager, instanceId = context.instanceId]
                    {
                        resourceManager.removeFromCollection(ReminderEntriesResource, instanceId);
                    });
                };
            }
        }, std::move(definition));
    }
}
