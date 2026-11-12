#include "collection_effect.hpp"
#include "definition_json.hpp"

namespace golarion
{
    namespace
    {
        using namespace definition_json;
        Json filterJson(const SpellEffectFilter &filter)
        { return {{"spellIds", filter.spellIds}, {"schools", value(filter.schools)}}; }
    }

    nlohmann::json castingRuleEffectToJson(const CastingRuleEffectDefinition &definition)
    {
        return std::visit([&](const auto &e) -> Json
        {
            using T = std::decay_t<decltype(e)>;
            Json j{{"id", definition.id}, {"castingId", definition.castingId}};
            if constexpr (std::is_same_v<T, SpellLevelOverrideEffect>)
            { j["type"] = "spellLevelOverride"; j["groupId"] = e.groupId; j["spellId"] = e.spellId; j["level"] = e.level; }
            else if constexpr (std::is_same_v<T, PreparationCostEffect>)
            { j["type"] = "preparationCost"; put(j, "school", e.school); j["requiredSlots"] = e.requiredSlots; }
            else if constexpr (std::is_same_v<T, PreparationCostExemptionEffect>) { j["type"] = "preparationCostExemption"; j["ruleId"] = e.ruleId; }
            else if constexpr (std::is_same_v<T, PreparationCheckEffect>) j["type"] = "preparationCheck";
            else if constexpr (std::is_same_v<T, PreparationCheckExemptionEffect>) { j["type"] = "preparationCheckExemption"; j["checkId"] = e.checkId; }
            else if constexpr (std::is_same_v<T, ConsumptionExemptionEffect>) j["type"] = "consumptionExemption";
            else if constexpr (std::is_same_v<T, SpontaneousConversionEffect>)
            {
                j["type"] = "spontaneousConversion"; j["preparedFilter"] = filterJson(e.preparedFilter);
                j["groupIds"] = e.groupIds; j["replacements"] = Json::array();
                for (const auto &[id, level] : e.replacementLevels) j["replacements"].push_back({{"spellId", id}, {"level", level}});
            }
            else if constexpr (std::is_same_v<T, MetamagicGrantEffect>)
            {
                j["type"] = "metamagicGrant"; put(j, "phase", e.phase); put(j, "metamagic", e.type);
                put(j, "elementalEnergy", e.elementalEnergy); put(j, "slotCost", e.slotCost); put(j, "rodGrade", e.rodGrade);
                put(j, "castingScope", e.castingScope);
            }
            else if constexpr (std::is_same_v<T, MetamagicMixingEffect>) j["type"] = "metamagicMixingPermission";
            else static_assert(sizeof(T) == 0, "missing casting rule serializer");
            if constexpr (requires { e.expression; }) j["expression"] = e.expression;
            if constexpr (requires { e.poolId; }) { put(j, "poolId", e.poolId); j["uses"] = e.uses; }
            if constexpr (std::is_same_v<T, MetamagicGrantEffect>) { if (e.filter) j["filter"] = filterJson(*e.filter); }
            else if constexpr (requires { e.filter; }) j["filter"] = filterJson(e.filter);
            return j;
        }, definition.entry);
    }
}
