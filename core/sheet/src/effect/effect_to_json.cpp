#include "golarion/effect/effect_json.hpp"
#include "collection_effect.hpp"
#include "definition_json.hpp"

namespace golarion
{
    namespace
    {
        using namespace definition_json;
        Json featFilter(const FeatSlotFilter &filter)
        {
            return std::visit([](const auto &entry) -> Json
            {
                using T = std::decay_t<decltype(entry)>;
                if constexpr (std::is_same_v<T, AnyFeat>) return {{"kind", "any"}};
                else if constexpr (std::is_same_v<T, FeatTypeFilter>) return {{"kind", "type"}, {"featType", enumName(entry.type)}};
                else return {{"kind", "ids"}, {"featIds", entry.ids}};
            }, filter);
        }
    }

    nlohmann::json effectDefinitionToJson(const EffectDefinition &definition)
    {
        return std::visit([](const auto &entry) -> Json
        {
            using T = std::decay_t<decltype(entry)>;
            Json j{{"id", entry.id}};
            if constexpr (std::is_same_v<T, CollectionEffectDefinition>) return collectionEffectToJson(entry);
            else if constexpr (std::is_same_v<T, CastingRuleEffectDefinition>) return castingRuleEffectToJson(entry);
            else if constexpr (std::is_same_v<T, ModifierEffectDefinition>)
            {
                j["type"] = "modifier";
                put(j, "modifierType", entry.type); put(j, "bonusType", entry.bonusType);
                put(j, "condition", entry.condition); put(j, "skillRankBudget", entry.skillRankBudget);
                j["description"] = entry.description;
            }
            else if constexpr (std::is_same_v<T, ContributionEffectDefinition>) j["type"] = "contribution";
            else if constexpr (std::is_same_v<T, SizeBaseEffectDefinition>) { j["type"] = "sizeBase"; put(j, "category", entry.category); }
            else if constexpr (std::is_same_v<T, MovementGrantEffectDefinition>)
            {
                j["type"] = "movementGrant";
                put(j, "movementType", entry.type); put(j, "maneuverability", entry.maneuverability);
                j["baseSpeedExpression"] = entry.baseSpeedExpression;
                j["affectedByArmor"] = entry.affectedByArmor; j["affectedByLoad"] = entry.affectedByLoad;
                j["supportsRunning"] = entry.supportsRunning;
            }
            else if constexpr (std::is_same_v<T, CarryingBodyTypeEffectDefinition>) { j["type"] = "carryingBodyType"; put(j, "bodyType", entry.type); }
            else if constexpr (std::is_same_v<T, ReminderEffectDefinition>) { j["type"] = "reminder"; j["message"] = entry.message; }
            else if constexpr (std::is_same_v<T, LanguageGrantEffectDefinition>) { j["type"] = "languageGrant"; j["languageId"] = entry.languageId; }
            else if constexpr (std::is_same_v<T, LanguageOptionsEffectDefinition>)
            { j["type"] = "languageOptions"; put(j, "origin", entry.origin); j["languageIds"] = entry.languageIds; }
            else if constexpr (std::is_same_v<T, SkillRankGrantEffectDefinition>)
            { j["type"] = "skillRankGrant"; j["skill"] = resourceName(entry.skill); put(j, "specializationId", entry.specializationId); }
            else if constexpr (std::is_same_v<T, HandUsagePermissionEffectDefinition>)
            {
                j["type"] = "handUsagePermission"; j["strikeGrantId"] = entry.strikeGrantId;
                put(j, "handUsage", entry.handUsage); put(j, "routineId", entry.routineId); put(j, "slotId", entry.slotId);
                j["requirements"] = requirements(entry.requirements);
            }
            else if constexpr (std::is_same_v<T, FeatSlotGrantEffectDefinition>) { j["type"] = "featSlotGrant"; j["filter"] = featFilter(entry.filter); }
            else if constexpr (std::is_same_v<T, FeatPrerequisiteAlternativeEffectDefinition>)
            { j["type"] = "featPrerequisiteAlternative"; j["target"] = entry.target; j["filter"] = featFilter(entry.filter); }
            else if constexpr (std::is_same_v<T, SpellGrantEffectDefinition>)
            {
                validateSpellGrantEffectDefinition(entry);
                j["type"] = "spellGrant"; j["spellId"] = entry.spellId;
                j["usage"] = {{"type", enumName(entry.usage)}};
                if (entry.usage == GrantedSpellUsage::Limited) j["usage"]["capacity"] = entry.capacity;
                if (entry.usage == GrantedSpellUsage::SharedPool)
                { put(j["usage"], "poolId", entry.poolId); j["usage"]["cost"] = entry.cost; }
            }
            else if constexpr (std::is_same_v<T, SpellcastingGrantEffectDefinition>)
            { j["type"] = "spellcastingGrant"; put(j, "mode", entry.mode); put(j, "spellListClass", entry.spellListClass); }
            else if constexpr (std::is_same_v<T, SpellSlotGrantEffectDefinition>)
            {
                j["type"] = "spellSlotGrant"; j["castingId"] = entry.castingId; put(j, "pool", entry.pool);
                put(j, "allowedSpellIds", entry.allowedSpellIds); j["slots"] = Json::array();
                for (const auto &[level, capacity] : entry.dailyCapacityByLevel) j["slots"].push_back({{"level", level}, {"capacity", capacity}});
            }
            else if constexpr (std::is_same_v<T, SpellRepertoireQuotaEffectDefinition>)
            {
                j["type"] = "spellRepertoireQuota"; j["castingId"] = entry.castingId;
                put(j, "allowedSpellIds", entry.allowedSpellIds); j["quotas"] = Json::array();
                for (const auto &[level, capacity] : entry.capacityByLevel) j["quotas"].push_back({{"level", level}, {"capacity", capacity}});
            }
            else if constexpr (std::is_same_v<T, SpellRepertoireGrantEffectDefinition>)
            { j["type"] = "spellRepertoireGrant"; j["castingId"] = entry.castingId; j["spellIds"] = entry.spellIds; }
            else if constexpr (std::is_same_v<T, SpellRepertoireListEffectDefinition>)
            { j["type"] = "spellRepertoireList"; j["castingId"] = entry.castingId; }
            else if constexpr (std::is_same_v<T, FavoredClassGrantEffectDefinition>)
            {
                j["type"] = "favoredClassGrant"; j["additionalChoices"] = entry.additionalChoices;
                j["allClasses"] = entry.allClasses; put(j, "classes", entry.classes);
            }
            else if constexpr (std::is_same_v<T, FavoredClassBonusEffectDefinition>)
            {
                j["type"] = "favoredClassBonus"; j["name"] = entry.name; j["description"] = entry.description;
                put(j, "classes", entry.classes); put(j, "replaces", entry.replaces);
                j["selectionsPerBenefit"] = entry.selectionsPerBenefit; j["effects"] = Json::array();
                for (const auto &effect : entry.effects) j["effects"].push_back(std::visit([](const auto &nested) { return effectDefinitionToJson(nested); }, effect));
            }
            else static_assert(sizeof(T) == 0, "missing effect serializer");
            if constexpr (requires { entry.resource; }) j["resource"] = entry.resource;
            if constexpr (requires { entry.expression; }) j["expression"] = entry.expression;
            return j;
        }, definition);
    }
}
