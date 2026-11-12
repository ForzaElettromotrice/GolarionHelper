#include "../fixtures/advance_sheet.hpp"
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
    rejects([&] { test::advanceSheetFixture(sheet, CharacterClassType::Fighter, 9); });
    assert(sheet.level() == 0);
    test::advanceSheetFixture(sheet, CharacterClassType::Fighter, 2);
    test::advanceSheetFixture(sheet, CharacterClassType::Fighter, 5);
    test::advanceSheetFixture(sheet, CharacterClassType::Rogue, 8);
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
    rejects([&] { test::advanceSheetFixture(sheet, CharacterClassType::Fighter, 9); });
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
    test::advanceSheetFixture(restored, CharacterClassType::Fighter, 1);
    assert(restored.toView().skills.maxRanks == 27); // Class contribution plus independent endpoint grant.
    restored.setFavoredClass(CharacterClassType::Fighter);
    restored.selectFavoredClassBonus(CharacterClassType::Fighter, 3, "base.hitPoint");
    restored.selectFeat("base.feat.level3", "improvedInitiative");
    restored.removeClassLevel(CharacterClassType::Fighter);
    assert(restored.level() == 3 && restored.toView().skills.maxRanks == 18);
    assert(restored.toView().hitPoints.max == 23 && restored.toView().hitPoints.current == 20);
    assert(!restored.toSaveData().classes.classes.front().hitDieResults.contains(3));
    assert(restored.toSaveData().classes.favoredClasses.bonuses.empty());
    restored.removeClassLevel(CharacterClassType::Rogue);
    assert(restored.level() == 2 && restored.toView().classes.classes.size() == 1);
    assert(!restored.toView().feats.feats.front().active);
    restored.save(path);
    auto reduced = CharacterSheet::load(path, catalog);
    assert(persistence::toJson(reduced.toSaveData()) == persistence::toJson(restored.toSaveData()));
    rejects([&] { reduced.removeClassLevel(CharacterClassType::Rogue); });
    reduced.removeClassLevel(CharacterClassType::Fighter);
    reduced.removeClassLevel(CharacterClassType::Fighter);
    assert(reduced.level() == 0 && reduced.toView().classes.classes.empty());
    rejects([&] { reduced.removeClassLevel(CharacterClassType::Fighter); });
    test::advanceSheetFixture(reduced, CharacterClassType::Fighter, 4);
    assert(reduced.level() == 1 && reduced.toSaveData().classes.classes.front().hitDieResults.size() == 1);

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
    auto intelligenceClass = test::classDefinition(CharacterClassType::Wizard);
    intelligenceClass.skillRanksPerLevel = 2;
    intelligenceClass.features = {{.id = "intelligence", .name = "Intelligenza", .description = "Bonus di prova", .progression = {
        {.id = "bonuses", .level = 1, .effects = {
            ModifierEffectDefinition{"permanent", std::string(PermanentIntelligenceResource), "Bonus permanente", ModifierType::Bonus, BonusType::Enhancement, "4", std::nullopt, SkillRankBudgetPolicy::Excluded},
            ModifierEffectDefinition{"included", std::string(PermanentIntelligenceResource), "Bonus ordinario", ModifierType::Bonus, BonusType::Enhancement, "2"},
            ModifierEffectDefinition{"temporary", std::string(TemporaryIntelligenceResource), "Bonus temporaneo", ModifierType::Bonus, BonusType::Enhancement, "6"},
            SkillRankGrantEffectDefinition{"ranks", SkillType::Spellcraft, "@classLevel + 3"}
        }}
    }}};
    const ClassDefinitionManager intelligenceCatalog({intelligenceClass});
    CharacterSheet intelligenceSheet(intelligenceCatalog);
    test::advanceSheetFixture(intelligenceSheet, CharacterClassType::Wizard, 4);
    assert(intelligenceSheet.toView().skills.maxRanks == 3); // 2 base + 1 from the included enhancement.
    assert(intelligenceSheet.toView().languages.remainingIntelligence == 2);
    intelligenceSheet.setSkillRanks(SkillType::Spellcraft, 1);
    assert(intelligenceSheet.toView().skills.spentRanks == 1);
    for (const auto &skill : intelligenceSheet.toView().skills.skills)
        if (skill.type == SkillType::Spellcraft) assert(skill.ranks == 1 && skill.effectiveRanks == 4);
    intelligenceSheet.save(path);
    auto restoredIntelligence = CharacterSheet::load(path, intelligenceCatalog);
    assert(restoredIntelligence.toView().skills.maxRanks == 3);
    assert(restoredIntelligence.toView().languages.remainingIntelligence == 2);
    assert(restoredIntelligence.toView().skills.spentRanks == 1);
    for (const auto &skill : restoredIntelligence.toView().skills.skills)
        if (skill.type == SkillType::Spellcraft) assert(skill.ranks == 1 && skill.effectiveRanks == 4);
    assert(persistence::toJson(restoredIntelligence.toSaveData()) == persistence::toJson(intelligenceSheet.toSaveData()));
    std::filesystem::remove(path);
}
