#pragma once

#include "golarion/character/spell_types.hpp"
#include "golarion/character/metamagic_types.hpp"
#include "golarion/character/damage.hpp"
#include "golarion/character/feat_slot_filter.hpp"
#include <map>
#include <optional>
#include <set>
#include <string>
#include <variant>

namespace golarion
{
    struct SpellEffectFilter
    {
        std::set<std::string> spellIds;
        std::set<SpellSchool> schools;
    };
    struct SpellLevelOverrideEffect { std::string groupId; std::string spellId; int level; };
    struct PreparationCostEffect { SpellSchool school; int requiredSlots; };
    struct PreparationCostExemptionEffect { std::string ruleId; };
    struct PreparationCheckEffect { SpellEffectFilter filter; std::string expression; };
    struct PreparationCheckExemptionEffect { std::string checkId; SpellEffectFilter filter; std::string expression = "1"; };
    struct ConsumptionExemptionEffect { SpellEffectFilter filter; std::string expression = "1"; };
    struct SpontaneousConversionEffect
    {
        SpellEffectFilter preparedFilter;
        std::set<std::string> groupIds;
        std::map<std::string, int> replacementLevels;
    };
    struct MetamagicGrantEffect
    {
        MetamagicPhase phase;
        MetamagicType type;
        std::optional<DamageType> elementalEnergy;
        MetamagicSlotCostPolicy slotCost = MetamagicSlotCostPolicy::Standard;
        std::optional<MetamagicRodGrade> rodGrade;
        std::optional<std::string> poolId;
        int uses = 1;
        CastingMetamagicScope castingScope = CastingMetamagicScope::AnyCast;
        // Explicit eligibility, including cases whose applicability is not in spell metadata.
        std::optional<SpellEffectFilter> filter;
    };
    struct MetamagicMixingEffect
    {
        SpellEffectFilter filter;
        std::optional<std::string> poolId;
        int uses = 1;
    };
    using CastingRuleEffectEntry = std::variant<SpellLevelOverrideEffect, PreparationCostEffect, PreparationCostExemptionEffect,
        PreparationCheckEffect, PreparationCheckExemptionEffect, ConsumptionExemptionEffect, SpontaneousConversionEffect,
        MetamagicGrantEffect, MetamagicMixingEffect>;
    struct CastingRuleEffectDefinition
    {
        std::string id;
        std::string castingId;
        CastingRuleEffectEntry entry;
    };
    struct FeatPrerequisiteAlternativeEffectDefinition
    {
        std::string id;
        std::string target;
        std::string expression;
        FeatSlotFilter filter = AnyFeat{};
    };
}
