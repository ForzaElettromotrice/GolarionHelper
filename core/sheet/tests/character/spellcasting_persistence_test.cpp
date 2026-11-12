#include "golarion/character/character_sheet.hpp"
#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/character/spontaneous_spellcasting.hpp"
#include "golarion/character/flexible_spellcasting.hpp"
#include "golarion/persistence/json.hpp"

#include <nlohmann/json.hpp>
#include <cassert>
#include <limits>

namespace
{
    using namespace golarion;
    template<typename F> void rejects(F function)
    {
        bool rejected = false;
        try { function(); } catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }

    void reconstruct(ResourceManager &resources, const std::shared_ptr<MetamagicUsagePool> &pool)
    {
        for (const auto &[id, mode] : {std::pair{"a", SpellcastingMode::Prepared}, {"b", SpellcastingMode::Spontaneous}, {"c", SpellcastingMode::Flexible}})
        {
            resources.addToCollection(Spellcastings::Resource, SpellcastingGrant{id, "Class", mode, CharacterClassType::Wizard});
            resources.addToCollection(repertoireQuotasResource(id), SpellRepertoireQuota{"known", "Class", {{3, 1}}});
            resources.addToCollection(spellSlotsResource(id), SpellSlotGrant{"base", "Class", {{3, 2}, {4, 2}, {8, 2}}});
            resources.addToCollection(preparationMetamagicGrantsResource(id), MetamagicGrant{
                .id = "silent", .source = "Test", .type = MetamagicType::Silent, .usageCost = MetamagicUsageCost{"source", 1, pool}});
        }
        resources.addToCollection(preparationSlotsResource("c"), SpellSlotGrant{"prepared", "Class", {{4, 1}}});
        resources.addToCollection(preparationCostRulesResource("a"), SpellPreparationCostRule{"school", "Test", SpellSchool::Evocation, 2});
    }

    std::string serialize(const SpellcastingsData &data)
    {
        auto sheet = CharacterSheet().toSaveData();
        sheet.spellcastings = data;
        return persistence::toJson(sheet);
    }
}

int main()
{
    ResourceManager resources;
    Spellcastings castings(resources);
    const auto pool = std::make_shared<MetamagicUsagePool>("source", 2);
    reconstruct(resources, pool);
    for (const auto id : {"a", "b", "c"}) castings.get(id).selectRepertoireSpell({"known", 3, "fireball"});
    auto &prepared = dynamic_cast<PreparedSpellcasting &>(castings.get("a"));
    prepared.prepare(std::vector<SpellSlotId>{{"base", 4, 0}, {"base", 4, 1}}, {"fireball", {{MetamagicType::Silent, "silent", {}}}});
    prepared.consume({"base", 4, 0});
    auto &spontaneous = dynamic_cast<SpontaneousSpellcasting &>(castings.get("b"));
    spontaneous.consume({"base", 3, 0}, {"fireball"});
    auto &flexible = dynamic_cast<FlexibleSpellcasting &>(castings.get("c"));
    flexible.prepare({"prepared", 4, 0}, {"fireball", {{MetamagicType::Silent, "silent", {}}}});
    flexible.consume({"base", 4, 0}, {{"prepared", 4, 0}});
    assert(pool->remaining() == 0);
    const auto saved = castings.toData();
    const auto json = serialize(saved);
    const auto decoded = persistence::fromJson(json);
    assert(decoded.formatVersion == 41);
    assert(serialize(decoded.spellcastings) == json);
    assert(json.find("dailyCapacityByLevel") == std::string::npos);
    assert(json.find("usageCost") == std::string::npos);

    ResourceManager restoredResources;
    Spellcastings restored(restoredResources);
    const auto exhausted = std::make_shared<MetamagicUsagePool>("source", 9);
    reconstruct(restoredResources, exhausted);
    auto *original = &restored.get("a");
    restored.load(decoded.spellcastings);
    assert(serialize(restored.toData()) == json);
    assert(exhausted->remaining() == 0);
    dynamic_cast<FlexibleSpellcasting &>(restored.get("c")).consume({"base", 4, 1}, {{"prepared", 4, 0}});
    assert(exhausted->remaining() == 0);
    restored.load(decoded.spellcastings);
    assert(&restored.get("a") == original);

    // Failure in the last entry must not commit changes to earlier entries or repertoires.
    auto invalid = saved;
    exhausted->reset(7);
    invalid.castings[0].repertoireData->manualSpellIds.push_back("extra");
    std::get<PreparedSpellcastingData>(invalid.castings[0].state).preparations.clear();
    std::get<FlexibleSpellcastingData>(invalid.castings[2].state).consumedSlots.push_back({"missing", 4, 0});
    rejects([&] { restored.load(invalid); });
    assert(exhausted->remaining() == 7);
    exhausted->reset(0);
    assert(serialize(restored.toData()) == json);
    for (int error = 0; error < 7; ++error)
    {
        invalid = saved;
        switch (error)
        {
        case 0: invalid.castings.push_back(saved.castings[0]); break;
        case 1: invalid.castings[0].id = "unknown"; break;
        case 2: invalid.castings[0].state = SpontaneousSpellcastingData{}; break;
        case 3: invalid.castings[0].repertoireData->selections.push_back({"known", 3, " fireball "}); break;
        case 4: invalid.castings[0].repertoireData->selections[0].spellLevel = 10; break;
        case 5: invalid.castings[0].repertoireData->manualSpellIds.push_back(" "); break;
        case 6: invalid.castings.pop_back(); break;
        }
        rejects([&] { restored.load(invalid); });
        assert(serialize(restored.toData()) == json);
    }
    resources.removeFromCollection(preparationMetamagicGrantsResource("c"), "silent");
    resources.removeFromCollection(preparationMetamagicGrantsResource("a"), "silent");
    resources.removeFromCollection(preparationCostRulesResource("a"), "school");
    castings.load(saved);
    assert(serialize(castings.toData()) == json);
    assert(pool->remaining() == 0);
    flexible.consume({"base", 4, 1}, {{"prepared", 4, 0}});
    rejects([&] { flexible.prepare({"prepared", 4, 0}, {"fireball", {{MetamagicType::Silent, "silent", {}}}}); });
    castings.load(saved);

    // The snapshot retains both levels and linked-slot payment without the original grants/rules.
    auto unconsumed = std::get<PreparedSpellcastingData>(saved.castings[0].state);
    unconsumed.preparations[0].consumed = false;
    prepared.load(unconsumed);
    const auto restoredCast = prepared.consume({"base", 4, 0});
    assert(restoredCast.baseSpellLevel == 3 && restoredCast.effectiveSpellLevel == 3 && restoredCast.requiredSlotLevel == 4);
    assert(prepared.slotState({"base", 4, 1}).state == PreparedSpellSlotState::Consumed);
    assert(pool->remaining() == 0);
    for (int error = 0; error < 5; ++error)
    {
        auto bad = unconsumed;
        auto &entry = bad.preparations.front();
        if (error == 0) entry.evaluation->requiredSlotLevel = 2;
        if (error == 1) entry.evaluation->nominalSlotLevel = 99;
        if (error == 2) entry.evaluation->effectiveSpellLevel = 9;
        if (error == 3) entry.additionalSlots.push_back(entry.slot);
        if (error == 4) entry.additionalSlots.front().groupId = "missing";
        rejects([&] { prepared.load(bad); });
        assert(prepared.slotState({"base", 4, 0}).state == PreparedSpellSlotState::Consumed);
    }

    auto wire = nlohmann::json::parse(json);
    for (int error = 0; error < 16; ++error)
    {
        auto bad = wire;
        auto &entry = bad["spellcastings"][0];
        switch (error)
        {
        case 0: entry["mode"] = "unknown"; break;
        case 1: entry["preparations"][0]["metamagic"][0]["type"] = "unknown"; break;
        case 2: entry["preparations"][0]["slot"]["index"] = -1; break;
        case 3: entry["preparations"][0]["slot"]["spellLevel"] = 3.5; break;
        case 4: entry["preparations"][0]["slot"]["spellLevel"] = std::numeric_limits<std::uint64_t>::max(); break;
        case 5: entry["repertoire"] = nullptr; break;
        case 6: bad["spellcastings"] = nlohmann::json::object(); break;
        case 7: entry["preparations"][0]["metamagic"][0]["options"] = {{"rangeSteps", 1}}; break;
        case 8: entry["preparations"][0]["consumed"] = 1; break;
        case 9: entry.erase("preparations"); break;
        case 10: entry["preparations"][0].erase("evaluation"); break;
        case 11: entry["preparations"][0]["evaluation"] = nullptr; break;
        case 12: entry["preparations"][0]["evaluation"]["baseSpellLevel"] = 2.5; break;
        case 13: bad["metamagicUses"]["source"] = -1; break;
        case 14: bad["metamagicUses"]["source"] = 0.5; break;
        case 15: bad.erase("metamagicUses"); break;
        }
        rejects([&] { persistence::fromJson(bad.dump()); });
    }

    // Every choice-bearing metamagic survives JSON independently of catalog expansion.
    auto choices = saved;
    auto &applications = std::get<PreparedSpellcastingData>(choices.castings[0].state).preparations[0].metamagic;
    applications = {{MetamagicType::Heighten, "heighten", HeightenMetamagicOptions{5}},
        {MetamagicType::Reach, "reach", ReachMetamagicOptions{2}},
        {MetamagicType::Elemental, "elemental", ElementalMetamagicOptions{DamageType::Cold, ElementalDamageMode::Split}}};
    const auto choiceJson = serialize(choices);
    assert(serialize(persistence::fromJson(choiceJson).spellcastings) == choiceJson);
    applications[0].options = ReachMetamagicOptions{1};
    rejects([&] { serialize(choices); });

    auto legacy = wire;
    legacy["formatVersion"] = 33;
    legacy.erase("metamagicUses");
    const auto legacyData = persistence::fromJson(legacy.dump()).spellcastings;
    assert(!legacyData.metamagicUses);
    assert(!std::get<PreparedSpellcastingData>(legacyData.castings[0].state).preparations[0].evaluation);
    // Old files cannot reconstruct historical results after grants disappear.
    rejects([&] { castings.load(legacyData); });
    restored.load(legacyData); // Reconstructed sources still support legacy saves.

    wire["formatVersion"] = 26;
    wire.erase("spellcastings");
    assert(persistence::fromJson(wire.dump()).spellcastings.castings.empty());
    auto empty = CharacterSheet().toSaveData();
    assert(empty.formatVersion == 41 && empty.spellcastings.castings.empty());
    assert(persistence::fromJson(persistence::toJson(empty)).spellcastings.castings.empty());
}
