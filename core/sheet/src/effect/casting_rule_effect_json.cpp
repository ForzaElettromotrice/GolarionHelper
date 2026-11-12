#include "collection_effect.hpp"
#include "golarion/util/string_utils.hpp"
#include <nlohmann/json.hpp>
#include <limits>

namespace
{
    using namespace golarion;
    using Json = nlohmann::json;
    std::string text(const Json &j, const char *key) { return normalize(j.at(key).get<std::string>()); }
    template<class E> E enumeration(const Json &j, const char *key, std::initializer_list<std::pair<std::string_view, E>> values)
    {
        const auto name = text(j, key);
        for (const auto &[label, value] : values) if (label == name) return value;
        throw std::invalid_argument("unknown casting rule enum: " + name);
    }
    int integer(const Json &j, const char *key, int fallback, int minimum = 0, int maximum = std::numeric_limits<int>::max())
    {
        if (!j.contains(key)) return fallback;
        const auto &value = j.at(key);
        if (!value.is_number_integer() || value < minimum || value > maximum) throw std::invalid_argument("invalid casting rule integer");
        return value.get<int>();
    }
    std::set<std::string> ids(const Json &j, const char *key)
    {
        if (!j.contains(key)) return {};
        const auto &values = j.at(key);
        if (!values.is_array()) throw std::invalid_argument("casting rule IDs must be an array");
        std::set<std::string> result;
        for (const auto &value : values)
            if (!result.insert(normalize(value.get<std::string>())).second) throw std::invalid_argument("duplicate casting rule ID");
        return result;
    }
    SpellSchool school(const Json &j)
    {
        using E = SpellSchool;
        return enumeration<E>(j, "school", {{"abjuration", E::Abjuration}, {"conjuration", E::Conjuration}, {"divination", E::Divination},
            {"enchantment", E::Enchantment}, {"evocation", E::Evocation}, {"illusion", E::Illusion}, {"necromancy", E::Necromancy}, {"transmutation", E::Transmutation}, {"universal", E::Universal}});
    }
    SpellEffectFilter filter(const Json &j, const char *key = "filter")
    {
        if (!j.contains(key)) return {};
        const auto &value = j.at(key);
        if (!value.is_object()) throw std::invalid_argument("spell filter must be an object");
        SpellEffectFilter result{ids(value, "spellIds"), {}};
        for (const auto &name : ids(value, "schools")) result.schools.insert(school(Json{{"school", name}}));
        return result;
    }
    template<class Entry> void cost(const Json &j, Entry &entry)
    {
        if (j.contains("poolId")) entry.poolId = text(j, "poolId");
        entry.uses = integer(j, "uses", 1, 1);
        if (!entry.poolId && entry.uses != 1) throw std::invalid_argument("usage count requires a pool");
    }
}

namespace golarion
{
    std::optional<CastingRuleEffectDefinition> castingRuleEffectFromJson(const Json &j)
    {
        const auto type = text(j, "type");
        const auto finish = [&](CastingRuleEffectEntry entry) -> std::optional<CastingRuleEffectDefinition>
        { return CastingRuleEffectDefinition{text(j, "id"), text(j, "castingId"), std::move(entry)}; };
        if (type == "spellLevelOverride") return finish(SpellLevelOverrideEffect{text(j, "groupId"), text(j, "spellId"), integer(j, "level", 0, 0, 9)});
        if (type == "preparationCost") return finish(PreparationCostEffect{school(j), integer(j, "requiredSlots", 2, 1)});
        if (type == "preparationCostExemption") return finish(PreparationCostExemptionEffect{text(j, "ruleId")});
        if (type == "preparationCheck") return finish(PreparationCheckEffect{filter(j), text(j, "expression")});
        if (type == "preparationCheckExemption") return finish(PreparationCheckExemptionEffect{text(j, "checkId"), filter(j), normalize(j.value("expression", "1"))});
        if (type == "consumptionExemption") return finish(ConsumptionExemptionEffect{filter(j), normalize(j.value("expression", "1"))});
        if (type == "spontaneousConversion")
        {
            SpontaneousConversionEffect entry{filter(j, "preparedFilter"), ids(j, "groupIds"), {}};
            const auto &replacements = j.at("replacements");
            if (!replacements.is_array() || replacements.empty() || entry.groupIds.empty()) throw std::invalid_argument("conversion needs groups and replacements");
            for (const auto &value : replacements)
                if (!entry.replacementLevels.emplace(text(value, "spellId"), integer(value, "level", 0, 0, 9)).second) throw std::invalid_argument("duplicate conversion replacement");
            return finish(entry);
        }
        if (type == "metamagicMixingPermission")
        {
            MetamagicMixingEffect entry{filter(j)};
            cost(j, entry);
            return finish(entry);
        }
        if (type == "metamagicGrant")
        {
            using E = MetamagicType;
            MetamagicGrantEffect entry{
                enumeration<MetamagicPhase>(j, "phase", {{"preparation", MetamagicPhase::Preparation}, {"casting", MetamagicPhase::Casting}}),
                enumeration<E>(j, "metamagic", {{"empower", E::Empower}, {"enlarge", E::Enlarge}, {"extend", E::Extend}, {"heighten", E::Heighten},
                    {"maximize", E::Maximize}, {"quicken", E::Quicken}, {"silent", E::Silent}, {"still", E::Still}, {"widen", E::Widen},
                    {"elemental", E::Elemental}, {"reach", E::Reach}, {"intensified", E::Intensified}, {"selective", E::Selective}})};
            if (j.contains("filter")) entry.filter = filter(j);
            if (j.contains("elementalEnergy")) entry.elementalEnergy = enumeration<DamageType>(j, "elementalEnergy", {
                {"acid", DamageType::Acid}, {"cold", DamageType::Cold}, {"electricity", DamageType::Electricity}, {"fire", DamageType::Fire}});
            if (j.contains("slotCost")) entry.slotCost = enumeration<MetamagicSlotCostPolicy>(j, "slotCost", {{"standard", MetamagicSlotCostPolicy::Standard}, {"waived", MetamagicSlotCostPolicy::Waived}});
            if (j.contains("rodGrade")) entry.rodGrade = enumeration<MetamagicRodGrade>(j, "rodGrade", {{"lesser", MetamagicRodGrade::Lesser}, {"normal", MetamagicRodGrade::Normal}, {"greater", MetamagicRodGrade::Greater}});
            if (j.contains("castingScope")) entry.castingScope = enumeration<CastingMetamagicScope>(j, "castingScope", {{"anyCast", CastingMetamagicScope::AnyCast}, {"spontaneousConversionOnly", CastingMetamagicScope::SpontaneousConversionOnly}});
            cost(j, entry);
            if (entry.phase == MetamagicPhase::Preparation && entry.castingScope != CastingMetamagicScope::AnyCast) throw std::invalid_argument("conversion scope requires casting phase");
            if (entry.rodGrade && (entry.phase != MetamagicPhase::Casting || entry.slotCost != MetamagicSlotCostPolicy::Waived || !entry.poolId)) throw std::invalid_argument("rod requires casting phase, waived slots and pool");
            return finish(entry);
        }
        return std::nullopt;
    }
}
