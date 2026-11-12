#include "golarion/effect/effect_json.hpp"

#include <nlohmann/json.hpp>

#include <cassert>
#include <stdexcept>
#include <variant>

namespace
{
    template<typename Function>
    bool throwsInvalidArgument(Function function)
    {
        try
        {
            function();
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        }
    }
}

int main()
{
    using namespace golarion;
    using Json = nlohmann::json;

    const ModifierEffectDefinition modifier = std::get<ModifierEffectDefinition>(effectDefinitionFromJson(Json{
        {"id", "savingThrows"},
        {"type", "modifier"},
        {"resource", "savingThrow.all"},
        {"description", "+1 ai tiri salvezza"},
        {"modifierType", "bonus"},
        {"bonusType", "resistance"},
        {"expression", "1"}
    }));
    assert(modifier.id == "savingThrows");
    assert(modifier.resource == "savingThrow.all");
    assert(modifier.bonusType == BonusType::Resistance);

    const ContributionEffectDefinition contribution = std::get<ContributionEffectDefinition>(effectDefinitionFromJson(Json{
        {"id", "hitPoints"},
        {"type", "contribution"},
        {"resource", "hitPoints.max"},
        {"expression", "@conMod * @level"}
    }));
    assert(contribution.expression == "@conMod * @level");

    const SizeBaseEffectDefinition size = std::get<SizeBaseEffectDefinition>(effectDefinitionFromJson(Json{
        {"id", "baseSize"},
        {"type", "sizeBase"},
        {"category", "small"}
    }));
    assert(size.category == SizeCategory::Small);

    const MovementGrantEffectDefinition movement = std::get<MovementGrantEffectDefinition>(effectDefinitionFromJson(Json{
        {"id", "flight"},
        {"type", "movementGrant"},
        {"movementType", "fly"},
        {"baseSpeedExpression", "12"},
        {"maneuverability", "good"},
        {"affectedByArmor", false},
        {"affectedByLoad", false},
        {"supportsRunning", true}
    }));
    assert(movement.type == MovementType::Fly);
    assert(movement.maneuverability == Maneuverability::Good);

    const CarryingBodyTypeEffectDefinition bodyType = std::get<CarryingBodyTypeEffectDefinition>(effectDefinitionFromJson(Json{
        {"id", "bodyType"},
        {"type", "carryingBodyType"},
        {"bodyType", "quadruped"}
    }));
    assert(bodyType.type == CarryingBodyType::Quadruped);

    const ReminderEffectDefinition reminder = std::get<ReminderEffectDefinition>(effectDefinitionFromJson(Json{
        {"id", "remember"},
        {"type", "reminder"},
        {"message", "Ricorda questa regola."}
    }));
    assert(reminder.message == "Ricorda questa regola.");

    assert(throwsInvalidArgument([]
    {
        static_cast<void>(effectDefinitionFromJson(Json{
            {"id", "invalid"},
            {"type", "modifier"},
            {"resource", "savingThrow.all"},
            {"description", "Bonus non tipizzato"},
            {"modifierType", "bonus"},
            {"expression", "1"}
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(effectDefinitionFromJson(Json{
            {"id", "invalid"},
            {"type", "movementGrant"},
            {"movementType", "land"},
            {"baseSpeedExpression", "6"},
            {"maneuverability", "good"},
            {"affectedByArmor", true},
            {"affectedByLoad", true}
        }));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(effectDefinitionFromJson(Json{{"id", "invalid"}, {"type", "unknown"}}));
    }));

    const auto parseSlot = [](const Json &filter)
    {
        return std::get<FeatSlotGrantEffectDefinition>(effectDefinitionFromJson(Json{
            {"id", "bonusFeat"}, {"type", "featSlotGrant"}, {"filter", filter}
        }));
    };
    const auto unrestricted = std::get<FeatSlotGrantEffectDefinition>(effectDefinitionFromJson(Json{{"id", "bonusFeat"}, {"type", "featSlotGrant"}}));
    assert(std::holds_alternative<AnyFeat>(unrestricted.filter));
    assert(std::holds_alternative<AnyFeat>(parseSlot({{"kind", "any"}}).filter));
    assert(std::get<FeatTypeFilter>(parseSlot({{"kind", "type"}, {"featType", " combat "}}).filter).type == FeatType::Combat);
    const auto filtered = parseSlot({{"kind", "ids"}, {"featIds", {" improvedInitiative ", "other"}}});
    assert(std::get<FeatIdFilter>(filtered.filter).ids == std::vector<std::string>({"improvedInitiative", "other"}));
    for (const Json &invalid : {
        Json{{"kind", "unknown"}}, Json::array(),
        Json{{"kind", "any"}, {"featType", "combat"}},
        Json{{"kind", "type"}, {"featType", " "}},
        Json{{"kind", "type"}, {"featType", "custom"}},
        Json{{"kind", "ids"}, {"featIds", Json::array()}},
        Json{{"kind", "ids"}, {"featIds", "improvedInitiative"}},
        Json{{"kind", "ids"}, {"featIds", {"one", " one "}}},
        Json{{"kind", "type"}, {"featType", "combat"}, {"featIds", {"one"}}}
    }) assert(throwsInvalidArgument([&] { parseSlot(invalid); }));

    for (const auto &[name, mode] : {std::pair{"prepared", SpellcastingMode::Prepared}, {"spontaneous", SpellcastingMode::Spontaneous}, {"flexible", SpellcastingMode::Flexible}})
    {
        const auto effect = std::get<SpellcastingGrantEffectDefinition>(effectDefinitionFromJson(Json{
            {"id", "casting"}, {"type", "spellcastingGrant"}, {"mode", name}, {"spellListClass", "wizard"}}));
        assert(effect.id == "casting" && effect.mode == mode && effect.spellListClass == CharacterClassType::Wizard);
    }
    for (const auto &[mode, list] : {std::pair{"unknown", "wizard"}, {"prepared", "unknown"}, {" ", "wizard"}})
        assert(throwsInvalidArgument([&] { effectDefinitionFromJson(Json{
            {"id", "casting"}, {"type", "spellcastingGrant"}, {"mode", mode}, {"spellListClass", list}}); }));
    for (const auto field : {"mode", "spellListClass"})
    {
        Json invalid{{"id", "casting"}, {"type", "spellcastingGrant"}, {"mode", "prepared"}, {"spellListClass", "wizard"}};
        invalid.erase(field);
        bool rejected = false;
        try { effectDefinitionFromJson(invalid); } catch (const Json::exception &) { rejected = true; }
        assert(rejected);
    }
    Json slotEffect{{"id", "slots"}, {"type", "spellSlotGrant"}, {"castingId", "caster"}, {"pool", "casting"},
        {"slots", Json::array({Json{{"level", 0}, {"capacity", 3}}, Json{{"level", 3}, {"capacity", 2}}})}};
    auto parsedSlots = std::get<SpellSlotGrantEffectDefinition>(effectDefinitionFromJson(slotEffect));
    assert(parsedSlots.pool == SpellSlotPoolType::Casting && parsedSlots.castingId == "caster");
    assert(parsedSlots.dailyCapacityByLevel.at(0) == 3 && parsedSlots.dailyCapacityByLevel.at(3) == 2);
    assert(!parsedSlots.allowedSpellIds);
    slotEffect["pool"] = "preparation";
    slotEffect["allowedSpellIds"] = {" fireball "};
    parsedSlots = std::get<SpellSlotGrantEffectDefinition>(effectDefinitionFromJson(slotEffect));
    assert(parsedSlots.pool == SpellSlotPoolType::Preparation && parsedSlots.allowedSpellIds->contains("fireball"));
    slotEffect["allowedSpellIds"] = Json::array();
    assert(std::get<SpellSlotGrantEffectDefinition>(effectDefinitionFromJson(slotEffect)).allowedSpellIds->empty());
    for (int error = 0; error < 13; ++error)
    {
        auto invalid = slotEffect;
        switch (error)
        {
        case 0: invalid["pool"] = "unknown"; break;
        case 1: invalid["castingId"] = " "; break;
        case 2: invalid["slots"] = Json::array(); break;
        case 3: invalid["slots"][0]["level"] = -1; break;
        case 4: invalid["slots"][0]["level"] = 10; break;
        case 5: invalid["slots"][0]["level"] = 1.5; break;
        case 6: invalid["slots"][0]["capacity"] = 0; break;
        case 7: invalid["slots"][0]["capacity"] = 2.5; break;
        case 8: invalid["slots"].push_back(invalid["slots"][0]); break;
        case 9: invalid["allowedSpellIds"] = {"fireball", " fireball "}; break;
        case 10: invalid["allowedSpellIds"] = nullptr; break;
        case 11: invalid["slots"] = Json::object(); break;
        case 12: invalid["slots"][0]["capacity"] = 4294967296ULL; break;
        }
        assert(throwsInvalidArgument([&] { effectDefinitionFromJson(invalid); }));
    }
    Json budgetEffect{{"id", "intelligence"}, {"type", "modifier"}, {"resource", "intelligence.permanent"},
        {"description", "Bonus alla caratteristica"}, {"modifierType", "bonus"}, {"bonusType", "enhancement"}, {"expression", "4"}};
    assert(std::get<ModifierEffectDefinition>(effectDefinitionFromJson(budgetEffect)).skillRankBudget == SkillRankBudgetPolicy::Included);
    budgetEffect["skillRankBudget"] = "excluded";
    assert(std::get<ModifierEffectDefinition>(effectDefinitionFromJson(budgetEffect)).skillRankBudget == SkillRankBudgetPolicy::Excluded);
    budgetEffect["skillRankBudget"] = "included";
    assert(std::get<ModifierEffectDefinition>(effectDefinitionFromJson(budgetEffect)).skillRankBudget == SkillRankBudgetPolicy::Included);
    budgetEffect["skillRankBudget"] = "unknown";
    assert(throwsInvalidArgument([&] { effectDefinitionFromJson(budgetEffect); }));
    Json handEffect{{"id", "grip"}, {"type", "handUsagePermission"}, {"strikeGrantId", " weapon "}, {"handUsage", "oneHanded"}};
    auto hand = std::get<HandUsagePermissionEffectDefinition>(effectDefinitionFromJson(handEffect));
    assert(hand.strikeGrantId == "weapon" && hand.handUsage == HandUsage::OneHanded);
    assert(hand.requirements.empty() && !hand.routineId && !hand.slotId);
    handEffect["handUsage"] = "twoHanded";
    handEffect["routineId"] = "routine";
    handEffect["slotId"] = "slot";
    handEffect["requirements"] = Json::array({{{"expression", "@level >= 3"}, {"failureReason", "Livello insufficiente"}}});
    hand = std::get<HandUsagePermissionEffectDefinition>(effectDefinitionFromJson(handEffect));
    assert(hand.handUsage == HandUsage::TwoHanded && hand.requirements.size() == 1);
    assert(hand.requirements[0].expression() == "@level >= 3" && hand.slotId == "slot");
    for (int error = 0; error < 9; ++error)
    {
        auto invalid = handEffect;
        switch (error)
        {
        case 0: invalid["handUsage"] = "unknown"; break;
        case 1: invalid["strikeGrantId"] = " "; break;
        case 2: invalid.erase("routineId"); break;
        case 3: invalid["routineId"] = " "; break;
        case 4: invalid["slotId"] = " "; break;
        case 5: invalid["requirements"] = Json::object(); break;
        case 6: invalid["requirements"][0]["expression"] = " "; break;
        case 7: invalid["requirements"][0]["failureReason"] = " "; break;
        case 8: invalid["requirements"] = nullptr; break;
        }
        assert(throwsInvalidArgument([&] { effectDefinitionFromJson(invalid); }));
    }
    return 0;
}
