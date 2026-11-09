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
    for (const auto id : {"a", "b", "c"}) castings.get(id).addToRepertoire("fireball");
    auto &prepared = dynamic_cast<PreparedSpellcasting &>(castings.get("a"));
    prepared.prepare(std::vector<SpellSlotId>{{"base", 4, 0}, {"base", 4, 1}}, "fireball", {{MetamagicType::Silent, "silent", {}}});
    prepared.consume({"base", 4, 0});
    auto &spontaneous = dynamic_cast<SpontaneousSpellcasting &>(castings.get("b"));
    spontaneous.consume({"base", 3, 0}, "fireball");
    auto &flexible = dynamic_cast<FlexibleSpellcasting &>(castings.get("c"));
    flexible.prepare({"prepared", 4, 0}, "fireball", {{MetamagicType::Silent, "silent", {}}});
    flexible.consume({"base", 4, 0}, {"prepared", 4, 0});
    assert(pool->remaining() == 0);
    const auto saved = castings.toData();
    const auto json = serialize(saved);
    const auto decoded = persistence::fromJson(json);
    assert(decoded.formatVersion == 29);
    assert(serialize(decoded.spellcastings) == json);
    assert(json.find("dailyCapacityByLevel") == std::string::npos);
    assert(json.find("usageCost") == std::string::npos);

    ResourceManager restoredResources;
    Spellcastings restored(restoredResources);
    const auto exhausted = std::make_shared<MetamagicUsagePool>("source", 0);
    reconstruct(restoredResources, exhausted);
    auto *original = &restored.get("a");
    restored.load(decoded.spellcastings);
    assert(serialize(restored.toData()) == json);
    assert(exhausted->remaining() == 0);
    dynamic_cast<FlexibleSpellcasting &>(restored.get("c")).consume({"base", 4, 1}, {"prepared", 4, 0});
    assert(exhausted->remaining() == 0);
    restored.load(decoded.spellcastings);
    assert(&restored.get("a") == original);

    // Failure in the last entry must not commit changes to earlier entries or repertoires.
    auto invalid = saved;
    invalid.castings[0].repertoire.push_back("extra");
    std::get<PreparedSpellcastingData>(invalid.castings[0].state).preparations.clear();
    std::get<FlexibleSpellcastingData>(invalid.castings[2].state).consumedSlots.push_back({"missing", 4, 0});
    rejects([&] { restored.load(invalid); });
    assert(serialize(restored.toData()) == json);
    for (int error = 0; error < 7; ++error)
    {
        invalid = saved;
        switch (error)
        {
        case 0: invalid.castings.push_back(saved.castings[0]); break;
        case 1: invalid.castings[0].id = "unknown"; break;
        case 2: invalid.castings[0].state = SpontaneousSpellcastingData{}; break;
        case 3: invalid.castings[0].repertoire.push_back(" fireball "); break;
        case 4: invalid.castings[0].repertoire.clear(); break;
        case 5: invalid.castings[0].repertoire.push_back(" "); break;
        case 6: invalid.castings.pop_back(); break;
        }
        rejects([&] { restored.load(invalid); });
        assert(serialize(restored.toData()) == json);
    }
    resources.removeFromCollection(preparationMetamagicGrantsResource("c"), "silent");
    rejects([&] { castings.load(saved); });
    assert(serialize(castings.toData()) == json);
    assert(pool->remaining() == 0);

    auto wire = nlohmann::json::parse(json);
    for (int error = 0; error < 10; ++error)
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

    wire["formatVersion"] = 26;
    wire.erase("spellcastings");
    assert(persistence::fromJson(wire.dump()).spellcastings.castings.empty());
    auto empty = CharacterSheet().toSaveData();
    assert(empty.formatVersion == 29 && empty.spellcastings.castings.empty());
    assert(persistence::fromJson(persistence::toJson(empty)).spellcastings.castings.empty());
}
