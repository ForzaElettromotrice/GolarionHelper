#include "../fixtures/class_definition.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/persistence/json.hpp"

#include <nlohmann/json.hpp>
#include <cassert>
#include <filesystem>
#include <stdexcept>

namespace
{
    template<class Action> void rejects(Action action)
    {
        bool rejected = false;
        try { action(); } catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    auto fighter = test::classDefinition(CharacterClassType::Fighter);
    fighter.features = {{.id = "training", .name = "Addestramento", .description = "Bonus sintetico.", .progression = {
        {.id = "extraRanks", .level = 3, .effects = {ContributionEffectDefinition{"ranks", "skill.ranks.max", "5"}}}
    }}};
    auto rogue = test::classDefinition(CharacterClassType::Rogue);
    rogue.skillRanksPerLevel = 8;
    const ClassDefinitionManager catalog({fighter, rogue});
    CharacterSheet sheet(catalog);
    sheet.setAbilityBaseValue(AbilityType::Constitution, 8);
    sheet.setAbilityBaseValue(AbilityType::Intelligence, 4);
    rejects([&] { sheet.addClassLevel(CharacterClassType::Fighter, 9); });
    assert(sheet.level() == 0);
    sheet.addClassLevel(CharacterClassType::Fighter, 2);
    sheet.addClassLevel(CharacterClassType::Fighter, 5);
    sheet.addClassLevel(CharacterClassType::Rogue, 8);
    assert(sheet.toView().hitPoints.max == 12);
    assert(sheet.toView().skills.maxRanks == 7); // 2 * max(1, 2 - 3) + max(1, 8 - 3).
    sheet.setAbilityBaseValue(AbilityType::Constitution, 4);
    assert(sheet.toView().hitPoints.max == 8); // Each die still grants at least one HP.
    sheet.setAbilityBaseValue(AbilityType::Constitution, 14);
    assert(sheet.toView().hitPoints.max == 21); // Constitution is counted exactly once.
    sheet.damage(3, DamageLethality::Lethal);
    sheet.setClassHitDieResult(CharacterClassType::Fighter, 1, 4);
    assert(sheet.toView().hitPoints.max == 23 && sheet.toView().hitPoints.current == 20);
    rejects([&] { sheet.setClassHitDieResult(CharacterClassType::Fighter, 3, 4); });
    rejects([&] { sheet.setClassHitDieResult(CharacterClassType::Fighter, 1, 0); });
    rejects([&] { sheet.addClassLevel(CharacterClassType::Fighter, 9); });
    assert(sheet.level() == 3 && sheet.toView().hitPoints.max == 23);

    sheet.setAbilityBaseValue(AbilityType::Intelligence, 8);
    assert(sheet.toView().skills.maxRanks == 9);
    sheet.setAbilityBaseValue(AbilityType::Intelligence, 14);
    assert(sheet.toView().skills.maxRanks == 18);
    sheet.setSkillRanks(SkillType::Climb, 2);
    sheet.setSkillSpecializationRanks(SkillType::Craft, "alchemy", 1);
    assert(sheet.toView().skills.spentRanks == 3 && sheet.toView().skills.remainingRanks == 15);
    sheet.setSkillRanks(SkillType::Climb, 20);
    assert(sheet.toView().skills.remainingRanks == -3); // Report excess without destroying selections.

    const auto path = std::filesystem::temp_directory_path() / "golarion_class_advancement_test.json";
    sheet.save(path);
    auto restored = CharacterSheet::load(path, catalog);
    assert(restored.toView().hitPoints.max == 23 && restored.toView().hitPoints.current == 20);
    assert(restored.toView().skills.maxRanks == 18);
    assert(restored.toView().classes.classes[0].hitDieResults.at(1) == 4);
    assert(persistence::toJson(restored.toSaveData()) == persistence::toJson(sheet.toSaveData()));
    restored.addClassLevel(CharacterClassType::Fighter, 1);
    assert(restored.toView().skills.maxRanks == 27); // Class contribution plus independent endpoint grant.

    const auto valid = nlohmann::json::parse(persistence::toJson(sheet.toSaveData()));
    auto malformed = valid;
    malformed["classes"][0]["hitDieResults"].push_back(malformed["classes"][0]["hitDieResults"][0]);
    rejects([&] { persistence::fromJson(malformed.dump()); });
    for (auto invalid : {nlohmann::json(0), nlohmann::json(1.5), nlohmann::json(13)})
    {
        malformed = valid;
        malformed["classes"][0]["hitDieResults"][0]["result"] = invalid;
        rejects([&] { persistence::fromJson(malformed.dump()); });
    }
    malformed = valid;
    malformed["classes"][0]["hitDieResults"][0]["result"] = 9; // Valid integer but exceeds this class's d8.
    persistence::saveToFile(persistence::fromJson(malformed.dump()), path);
    rejects([&] { CharacterSheet::load(path, catalog); });

    auto legacy = valid;
    legacy["formatVersion"] = 31;
    for (auto &entry : legacy["classes"]) entry.erase("hitDieResults");
    persistence::saveToFile(persistence::fromJson(legacy.dump()), path);
    auto migrated = CharacterSheet::load(path, catalog);
    assert(migrated.toView().classes.classes[0].hitDieResults.empty());
    migrated.setClassHitDieResult(CharacterClassType::Fighter, 1, 4);
    assert(migrated.toView().classes.classes[0].hitDieResults.at(1) == 4);
    std::filesystem::remove(path);
}
