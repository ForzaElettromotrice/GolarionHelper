#include "golarion/effect/effect_compiler.hpp"
#include "collection_effect.hpp"

#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/reminder.hpp"
#include "golarion/character/languages.hpp"
#include "golarion/character/granted_spells.hpp"
#include "golarion/character/spellcastings.hpp"
#include "golarion/character/flexible_spellcasting.hpp"
#include "golarion/character/feats.hpp"
#include "golarion/character/favored_classes.hpp"
#include "golarion/character/size.hpp"
#include "golarion/resource/contribution.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/resource/expression_parser.hpp"
#include "golarion/util/string_utils.hpp"

#include <type_traits>
#include <algorithm>
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

            if constexpr (std::is_same_v<Effect, CastingRuleEffectDefinition>)
            {
                return compileCastingRuleEffect(std::move(effect), context);
            }
            else if constexpr (std::is_same_v<Effect, FeatPrerequisiteAlternativeEffectDefinition>)
            {
                effect.target = normalize(effect.target);
                effect.expression = normalize(effect.expression);
                return [effect = std::move(effect), context](ResourceManager &resources) -> EffectCleanup
                {
                    EffectCleanup cleanup = [&resources, id = context.instanceId] { resources.removeFromCollection(FeatPrerequisiteAlternativesResource, id); };
                    resources.addToCollection(FeatPrerequisiteAlternativesResource, FeatPrerequisiteAlternative{
                        context.instanceId, context.source, effect.target,
                        [filter = effect.filter](const FeatPrerequisiteContext &query)
                        {
                            return std::visit([&](const auto &value)
                            {
                                using Filter = std::decay_t<decltype(value)>;
                                if constexpr (std::is_same_v<Filter, AnyFeat>) return true;
                                else if constexpr (std::is_same_v<Filter, FeatTypeFilter>) return std::find(query.feat.types.begin(), query.feat.types.end(), value.type) != query.feat.types.end();
                                else return std::find(value.ids.begin(), value.ids.end(), query.feat.id) != value.ids.end();
                            }, filter);
                        }, [expression = effect.expression](const FeatPrerequisiteContext &, const FeatTargetResolver &read)
                        { return ExpressionParser::evaluate(expression, read); }});
                    return cleanup;
                };
            }
            else if constexpr (std::is_same_v<Effect, CollectionEffectDefinition>)
            {
                return compileCollectionEffect(std::move(effect), context);
            }
            else if constexpr (std::is_same_v<Effect, ModifierEffectDefinition>)
            {
                return [effect = std::move(effect), context](ResourceManager &resourceManager)
                {
                    Modifier modifier({.type = effect.type, .source = context.source, .description = effect.description, .bonusType = effect.bonusType, .expression = effect.expression, .condition = effect.condition, .requirements = {}, .skillRankBudget = effect.skillRankBudget});
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
            else if constexpr (std::is_same_v<Effect, SpellcastingGrantEffectDefinition>)
            {
                characterClassId(effect.spellListClass);
                switch (effect.mode)
                {
                case SpellcastingMode::Prepared: case SpellcastingMode::Spontaneous: case SpellcastingMode::Flexible: break;
                default: throw std::invalid_argument("invalid spellcasting mode");
                }
                return [effect, context](ResourceManager &resources)
                {
                    EffectCleanup cleanup = [&resources, id = context.instanceId]
                    {
                        resources.removeFromCollection(Spellcastings::Resource, id);
                    };
                    resources.addToCollection(Spellcastings::Resource, SpellcastingGrant{
                        context.instanceId, context.source, effect.mode, effect.spellListClass});
                    return cleanup;
                };
            }
            else if constexpr (std::is_same_v<Effect, SpellSlotGrantEffectDefinition>)
            {
                std::string resource;
                switch (effect.pool)
                {
                case SpellSlotPoolType::Casting: resource = spellSlotsResource(effect.castingId); break;
                case SpellSlotPoolType::Preparation: resource = preparationSlotsResource(effect.castingId); break;
                default: throw std::invalid_argument("invalid spell slot pool type");
                }
                auto grant = normalizeSpellSlotGrant({context.instanceId, context.source, effect.dailyCapacityByLevel, effect.allowedSpellIds});
                return [resource = std::move(resource), grant = std::move(grant)](ResourceManager &resources)
                {
                    EffectCleanup cleanup = [&resources, resource, id = grant.id]
                    {
                        resources.removeFromCollection(resource, id);
                    };
                    resources.addToCollection(resource, grant);
                    return cleanup;
                };
            }
            else if constexpr (std::is_same_v<Effect, SpellRepertoireQuotaEffectDefinition> || std::is_same_v<Effect, SpellRepertoireGrantEffectDefinition> || std::is_same_v<Effect, SpellRepertoireListEffectDefinition>)
            {
                const auto resource = [&]
                {
                    if constexpr (std::is_same_v<Effect, SpellRepertoireQuotaEffectDefinition>) return repertoireQuotasResource(effect.castingId);
                    else if constexpr (std::is_same_v<Effect, SpellRepertoireGrantEffectDefinition>) return repertoireGrantsResource(effect.castingId);
                    else return repertoireListsResource(effect.castingId);
                }();
                return [effect, context, resource](ResourceManager &resources)
                {
                    EffectCleanup cleanup = [&resources, resource, id = context.instanceId] { resources.removeFromCollection(resource, id); };
                    if constexpr (std::is_same_v<Effect, SpellRepertoireQuotaEffectDefinition>)
                        resources.addToCollection(resource, SpellRepertoireQuota{context.instanceId, context.source, effect.capacityByLevel, effect.allowedSpellIds});
                    else if constexpr (std::is_same_v<Effect, SpellRepertoireGrantEffectDefinition>)
                        resources.addToCollection(resource, SpellRepertoireGrant{context.instanceId, context.source, effect.spellIds});
                    else resources.addToCollection(resource, SpellRepertoireListGrant{context.instanceId, context.source});
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
            else if constexpr (std::is_same_v<Effect, HandUsagePermissionEffectDefinition>)
            {
                effect.strikeGrantId = normalize(effect.strikeGrantId);
                displayName(effect.handUsage);
                if (effect.routineId) effect.routineId = normalize(*effect.routineId);
                if (effect.slotId) effect.slotId = normalize(*effect.slotId);
                if (effect.slotId && !effect.routineId) throw std::invalid_argument("hand usage slot restriction requires a routine");
                return [effect = std::move(effect), context](ResourceManager &resources)
                {
                    EffectCleanup cleanup = [&resources, id = context.instanceId]
                    {
                        resources.removeFromCollection(HandUsagePermissionsResource, id);
                    };
                    resources.addToCollection(HandUsagePermissionsResource, HandUsagePermission{
                        context.instanceId, context.source, effect.strikeGrantId, effect.handUsage,
                        effect.requirements, effect.routineId, effect.slotId});
                    return cleanup;
                };
            }
            else if constexpr (std::is_same_v<Effect, SkillRankGrantEffectDefinition>)
            {
                auto endpoint = skillRankGrantsResource(effect.skill, effect.specializationId);
                effect.expression = normalize(effect.expression);
                return [endpoint = std::move(endpoint), expression = std::move(effect.expression), context](ResourceManager &resources)
                {
                    EffectCleanup cleanup = [&resources, endpoint, id = context.instanceId]
                    {
                        resources.removeFromCollection(endpoint, id);
                    };
                    resources.addToCollection(endpoint, SkillRankGrant{context.instanceId, context.source, expression});
                    return cleanup;
                };
            }
            else if constexpr (std::is_same_v<Effect, LanguageOptionsEffectDefinition>)
            {
                if (effect.origin != LanguageOrigin::Intelligence && effect.origin != LanguageOrigin::Linguistics)
                    throw std::invalid_argument("unknown language origin");
                std::set<std::string> ids;
                for (const auto &id : effect.languageIds)
                    if (!ids.insert(normalize(id)).second) throw std::invalid_argument("duplicate language option");
                return [ids = std::move(ids), origin = effect.origin, context](ResourceManager &resources)
                {
                    EffectCleanup cleanup = [&resources, id = context.instanceId] { resources.removeFromCollection(LanguageOptionsResource, id); };
                    resources.addToCollection(LanguageOptionsResource, LanguageOptionsGrant{context.instanceId, context.source, origin, ids});
                    return cleanup;
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
            else if constexpr (std::is_same_v<Effect, FavoredClassGrantEffectDefinition>)
            {
                if (effect.additionalChoices < 0) throw std::invalid_argument("favored class choice grant must not be negative");
                for (auto type : effect.classes) characterClassId(type);
                return [effect = std::move(effect), context](ResourceManager &resources) -> EffectCleanup
                {
                    resources.addToCollection(FavoredClasses::GrantsResource, FavoredClassGrant{context.instanceId, context.source, effect.additionalChoices, effect.classes, effect.allClasses});
                    return [&resources, id = context.instanceId] { resources.removeFromCollection(FavoredClasses::GrantsResource, id); };
                };
            }
            else if constexpr (std::is_same_v<Effect, FavoredClassBonusEffectDefinition>)
            {
                if (effect.selectionsPerBenefit < 1) throw std::invalid_argument("favored class benefit interval must be positive");
                for (auto type : effect.classes) characterClassId(type);
                for (auto base : effect.replaces)
                    if (base != FavoredClassBaseBonus::HitPoint && base != FavoredClassBaseBonus::SkillRank) throw std::invalid_argument("unknown favored class base bonus");
                for (const auto &benefit : effect.effects) std::visit([&](const auto &value) { compileEffect(value, context); }, benefit);
                return [effect = std::move(effect), context](ResourceManager &resources) -> EffectCleanup
                {
                    resources.addToCollection(FavoredClasses::OptionsResource, FavoredClassBonusOption{context.instanceId, effect.name, effect.description, effect.classes, effect.effects, effect.replaces, effect.selectionsPerBenefit, context.usagePool});
                    return [&resources, id = context.instanceId] { resources.removeFromCollection(FavoredClasses::OptionsResource, id); };
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
