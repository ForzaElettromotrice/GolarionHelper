#include "golarion/effect/effect_compiler.hpp"

#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/reminder.hpp"
#include "golarion/character/languages.hpp"
#include "golarion/character/granted_spells.hpp"
#include "golarion/character/feats.hpp"
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
    void validateSpellGrantEffectDefinition(const SpellGrantEffectDefinition &effect)
    {
        normalize(effect.id);
        normalize(effect.spellId);
        switch (effect.usage)
        {
        case GrantedSpellUsage::Limited:
            if (effect.capacity <= 0 || effect.poolId || effect.cost != 1) throw std::invalid_argument("invalid limited spell grant");
            break;
        case GrantedSpellUsage::SharedPool:
            if (!effect.poolId || effect.capacity != 0 || effect.cost <= 0) throw std::invalid_argument("invalid shared spell grant");
            normalize(*effect.poolId);
            break;
        case GrantedSpellUsage::AtWill:
        case GrantedSpellUsage::Constant:
            if (effect.capacity != 0 || effect.poolId || effect.cost != 1) throw std::invalid_argument("unlimited spell cannot have usage costs");
            break;
        default: throw std::invalid_argument("unknown granted spell usage");
        }
    }

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
            else if constexpr (std::is_same_v<Effect, SpellGrantEffectDefinition>)
            {
                validateSpellGrantEffectDefinition(effect);
                std::shared_ptr<UsagePool> pool;
                if (effect.poolId)
                {
                    if (!context.usagePool) throw std::invalid_argument("spell grant requires a source usage pool resolver");
                    pool = context.usagePool(normalize(*effect.poolId));
                    if (!pool || effect.cost > pool->capacity()) throw std::invalid_argument("invalid spell grant usage pool");
                }
                return [effect = std::move(effect), context, pool = std::move(pool)](ResourceManager &resourceManager)
                {
                    EffectCleanup cleanup = [&resourceManager, id = context.instanceId]
                    {
                        resourceManager.removeFromCollection(GrantedSpells::Resource, id);
                    };
                    resourceManager.addToCollection(GrantedSpells::Resource, GrantedSpellDefinition{
                        context.instanceId, context.source, effect.spellId, effect.usage, effect.capacity, pool, effect.cost});
                    return cleanup;
                };
            }
            else if constexpr (std::is_same_v<Effect, FeatSlotGrantEffectDefinition>)
            {
                return [effect = std::move(effect), context](ResourceManager &resourceManager)
                {
                    resourceManager.addToCollection(FeatSlotsResource, FeatSlot{.id = context.instanceId, .source = context.source, .filter = effect.filter});
                    return EffectCleanup([&resourceManager, instanceId = context.instanceId]
                    {
                        resourceManager.removeFromCollection(FeatSlotsResource, instanceId);
                    });
                };
            }
            else if constexpr (std::is_same_v<Effect, LanguageGrantEffectDefinition>)
            {
                return [effect = std::move(effect), context](ResourceManager &resourceManager)
                {
                    resourceManager.addToCollection(SpokenLanguagesResource, LanguageGrant{context.instanceId, context.source, effect.languageId});
                    return EffectCleanup([&resourceManager, instanceId = context.instanceId]
                    {
                        resourceManager.removeFromCollection(SpokenLanguagesResource, instanceId);
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
