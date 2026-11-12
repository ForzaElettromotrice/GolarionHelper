#include "collection_effect.hpp"
#include "golarion/character/flexible_spellcasting.hpp"
#include "golarion/character/spell_preparation_cost.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/util/string_utils.hpp"
#include <algorithm>

namespace
{
    using namespace golarion;
    bool matches(const SpellEffectFilter &filter, const SpellDefinition &spell)
    {
        return (filter.spellIds.empty() || filter.spellIds.contains(spell.id)) && (filter.schools.empty() || filter.schools.contains(spell.school));
    }
    void validateFilter(const SpellEffectFilter &filter)
    {
        for (const auto &id : filter.spellIds) if (normalize(id) != id) throw std::invalid_argument("spell filter IDs must be normalized");
        for (auto school : filter.schools)
            if (school < SpellSchool::Abjuration || school > SpellSchool::Universal) throw std::invalid_argument("invalid spell school");
    }
    template<class Entry> EffectCleanup add(ResourceManager &resources, const std::string &endpoint, Entry entry)
    {
        EffectCleanup cleanup = [&resources, endpoint, id = entry.id] { resources.removeFromCollection(endpoint, id); };
        resources.addToCollection(endpoint, std::move(entry));
        return cleanup;
    }
    std::optional<MetamagicUsageCost> usage(const std::optional<std::string> &poolId, int uses, const EffectContext &context)
    {
        if (uses <= 0 || (!poolId && uses != 1)) throw std::invalid_argument("invalid metamagic usage cost");
        if (!poolId) return std::nullopt;
        if (!context.usagePool) throw std::invalid_argument("metamagic requires a source usage pool resolver");
        auto pool = context.usagePool(normalize(*poolId));
        if (!pool || uses > pool->capacity()) throw std::invalid_argument("invalid metamagic usage pool");
        auto id = context.instanceId + ".usage." + *poolId;
        return MetamagicUsageCost{id, uses, std::make_shared<MetamagicUsagePool>(id, std::move(pool))};
    }
}

namespace golarion
{
    EffectApply compileCastingRuleEffect(CastingRuleEffectDefinition definition, EffectContext context)
    {
        definition.castingId = normalize(definition.castingId);
        return std::visit([&](auto rule) -> EffectApply
        {
            using T = std::decay_t<decltype(rule)>;
            if constexpr (requires { rule.expression; }) rule.expression = normalize(rule.expression);
            if constexpr (requires { rule.filter; })
            {
                if constexpr (std::is_same_v<T, MetamagicGrantEffect>) { if (rule.filter) validateFilter(*rule.filter); }
                else validateFilter(rule.filter);
            }
            if constexpr (std::is_same_v<T, MetamagicGrantEffect>)
            {
                if (rule.phase != MetamagicPhase::Preparation && rule.phase != MetamagicPhase::Casting) throw std::invalid_argument("invalid metamagic phase");
                auto cost = usage(rule.poolId, rule.uses, context);
                MetamagicGrant grant{.id = context.instanceId, .source = context.source, .type = rule.type,
                    .elementalEnergy = rule.elementalEnergy, .slotCost = rule.slotCost, .rodGrade = rule.rodGrade,
                    .usageCost = cost, .castingScope = rule.castingScope, .isApplicable = {}};
                if (rule.filter) grant.isApplicable = [filter = *rule.filter](const SpellDefinition &spell, const MetamagicApplication &) { return matches(filter, spell); };
                const auto endpoint = rule.phase == MetamagicPhase::Preparation ? preparationMetamagicGrantsResource(definition.castingId) : castingMetamagicGrantsResource(definition.castingId);
                return [grant, endpoint](ResourceManager &resources) { return add(resources, endpoint, grant); };
            }
            else if constexpr (std::is_same_v<T, MetamagicMixingEffect>)
            {
                MetamagicMixingPermission grant{context.instanceId, context.source,
                    [filter = rule.filter](const MetamagicMixingContext &query) { return matches(filter, query.spell); }, usage(rule.poolId, rule.uses, context)};
                return [grant, endpoint = metamagicMixingPermissionsResource(definition.castingId)](ResourceManager &resources) { return add(resources, endpoint, grant); };
            }
            else
            {
                if constexpr (std::is_same_v<T, SpontaneousConversionEffect>)
                {
                    validateFilter(rule.preparedFilter);
                    if (rule.groupIds.empty() || rule.replacementLevels.empty()) throw std::invalid_argument("conversion requires explicit groups and replacement spells");
                    for (const auto &id : rule.groupIds) normalize(id);
                    for (const auto &[id, level] : rule.replacementLevels)
                        if (normalize(id) != id || level < 0 || level > 9) throw std::invalid_argument("invalid conversion spell level");
                }
                return [rule, castingId = definition.castingId, context](ResourceManager &resources) -> EffectCleanup
                {
                    const auto &id = context.instanceId;
                    const auto &source = context.source;
                    if constexpr (std::is_same_v<T, SpellLevelOverrideEffect>)
                        return add(resources, spellLevelOverridesResource(castingId), SpellLevelOverride{id, source, rule.groupId, rule.spellId, rule.level});
                    else if constexpr (std::is_same_v<T, PreparationCostEffect>)
                        return add(resources, preparationCostRulesResource(castingId), SpellPreparationCostRule{id, source, rule.school, rule.requiredSlots});
                    else if constexpr (std::is_same_v<T, PreparationCostExemptionEffect>)
                        return add(resources, preparationCostExemptionsResource(castingId), SpellPreparationCostExemption{id, source, rule.ruleId});
                    else if constexpr (std::is_same_v<T, PreparationCheckEffect>)
                        return add(resources, preparationChecksResource(castingId), SpellPreparationCheck{id, source,
                            [&resources, rule](const SpellPreparationContext &query) { return !matches(rule.filter, query.spell) || resources.evaluateExpression(rule.expression) != 0; }});
                    else if constexpr (std::is_same_v<T, PreparationCheckExemptionEffect>)
                        return add(resources, preparationCheckExemptionsResource(castingId), SpellPreparationCheckExemption{id, source, rule.checkId,
                            [&resources, rule](const SpellPreparationContext &query) { return matches(rule.filter, query.spell) && resources.evaluateExpression(rule.expression) != 0; }});
                    else if constexpr (std::is_same_v<T, ConsumptionExemptionEffect>)
                        return add(resources, consumptionExemptionsResource(castingId), SpellConsumptionExemption{id, source,
                            [&resources, rule](const SpellConsumptionContext &query) { return matches(rule.filter, query.spell) && resources.evaluateExpression(rule.expression) != 0; }});
                    else if constexpr (std::is_same_v<T, SpontaneousConversionEffect>)
                        return add(resources, spontaneousConversionsResource(castingId), SpontaneousConversion{id, source,
                            [rule](const SpellConversionContext &query) -> std::optional<int>
                            {
                                if (!matches(rule.preparedFilter, query.preparedSpell)) return std::nullopt;
                                const auto found = rule.replacementLevels.find(query.replacementSpell.id);
                                if (found == rule.replacementLevels.end()) return std::nullopt;
                                for (const auto &slot : query.slots) if (!rule.groupIds.contains(slot.groupId)) return std::nullopt;
                                return found->second;
                            }});
                };
            }
        }, std::move(definition.entry));
    }
}
