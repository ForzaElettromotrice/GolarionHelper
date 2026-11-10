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
    fighter.features = {{
        .id = "training", .name = "Addestramento", .description = "Privilegio sintetico.",
        .progression = {
            {.id = "permanent", .level = 1, .effects = {
                FeatSlotGrantEffectDefinition{"feat"},
                SpellGrantEffectDefinition{.id = "spell", .spellId = "fireball", .usage = GrantedSpellUsage::Limited, .capacity = 2}
            }},
            {.id = "selection", .level = 1, .choices = {{
                .id = "path", .prompt = "Scegli un percorso", .selectionCount = 1,
                .options = {{.id = "first", .name = "Primo", .description = "Prima opzione", .progression = {
                    {.id = "selected", .level = 1, .effects = {ContributionEffectDefinition{"bonus", "baseAttackBonus", "@classLevel"}}}
                }}}
            }}},
            {.id = "upgrade", .level = 3, .replaces = {"selection"}}
        }
    }};
    const ClassDefinitionManager catalog({fighter, test::classDefinition(CharacterClassType::Wizard)});
    const auto path = std::filesystem::temp_directory_path() / "golarion_classes_persistence_test.json";
    CharacterSheet sheet(catalog);
    assert(sheet.level() == 0);
    assert(sheet.toView().classes.classes.empty() && sheet.toView().feats.slots.empty());
    sheet.addClassLevel(CharacterClassType::Fighter);
    auto view = sheet.toView();
    assert(view.classes.level == 1 && view.classes.classes[0].level == 1);
    auto choice = view.classes.classes[0].features[0].acquisitions[1].choices[0];
    assert(choice.remainingSelections == 1 && !choice.options[0].selected);
    sheet.selectClassOption(CharacterClassType::Fighter, "selection", "path", "first");
    assert(sheet.toView().baseAttackBonus.total == 2);
    std::string bonusSlot;
    for (const auto &slot : view.feats.slots) if (!slot.id.starts_with("base.")) bonusSlot = slot.id;
    assert(!bonusSlot.empty());
    sheet.selectFeat(bonusSlot, "improvedInitiative");
    const auto spellId = view.grantedSpells[0].id;
    sheet.consumeGrantedSpell(spellId);
    sheet.addClassLevel(CharacterClassType::Wizard);
    sheet.addClassLevel(CharacterClassType::Fighter);
    assert(sheet.level() == 3 && sheet.toView().feats.slots.size() == 3);
    assert(sheet.toView().baseAttackBonus.total == 5);
    sheet.save(path);
    auto restored = CharacterSheet::load(path, catalog);
    assert(restored.level() == 3);
    assert(restored.toView().baseAttackBonus.total == 5);
    assert(restored.toView().feats.feats[0].slotId == bonusSlot);
    assert(restored.toView().grantedSpells[0].remaining == 1);
    assert(persistence::toJson(restored.toSaveData()) == persistence::toJson(sheet.toSaveData()));
    rejects([&] { CharacterSheet::load(path); }); // Missing catalog must not discard classes.

    // Replacement suppresses the branch, but its saved choice survives.
    sheet.addClassLevel(CharacterClassType::Fighter);
    sheet.save(path);
    auto inactive = CharacterSheet::load(path, catalog);
    auto stage = inactive.toView().classes.classes[0].features[0].acquisitions[1];
    assert(!stage.active && stage.choices[0].options[0].selected);
    assert(stage.choices[0].remainingSelections == 0);
    assert(inactive.toView().baseAttackBonus.total == 4);
    assert(inactive.toSaveData().classes.classes[0].choices.size() == 1);

    const auto valid = nlohmann::json::parse(persistence::toJson(sheet.toSaveData()));
    auto malformed = valid;
    malformed["classes"][0]["type"] = "unknown";
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = valid;
    malformed["classes"][0]["level"] = 1.5;
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = valid;
    malformed["classes"].push_back(malformed["classes"][0]);
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = valid;
    malformed["classes"][0]["choices"][0]["optionIds"] = {"unknown"};
    persistence::saveToFile(persistence::fromJson(malformed.dump()), path);
    rejects([&] { CharacterSheet::load(path, catalog); });
    malformed = valid;
    malformed["classes"][0]["choices"][0]["optionIds"] = {"first", "first"};
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = valid;
    malformed["classes"][0]["level"] = 99;
    persistence::saveToFile(persistence::fromJson(malformed.dump()), path);
    rejects([&] { CharacterSheet::load(path, catalog); });

    // Legacy saves retain an acquired level-one feat, but do not invent class levels.
    CharacterSheet empty;
    auto legacy = empty.toSaveData();
    legacy.formatVersion = 29;
    legacy.feats.selections = {{"base.feat.level1", "improvedInitiative", {}}};
    persistence::saveToFile(legacy, path);
    auto migrated = CharacterSheet::load(path, catalog);
    assert(migrated.level() == 0 && migrated.toView().legacyFirstLevelFeatSlot);
    assert(migrated.toView().feats.feats.size() == 1);
    migrated.save(path);
    auto migratedAgain = CharacterSheet::load(path, catalog);
    assert(migratedAgain.level() == 0 && migratedAgain.toView().legacyFirstLevelFeatSlot);
    migratedAgain.addClassLevel(CharacterClassType::Wizard);
    assert(!migratedAgain.toView().legacyFirstLevelFeatSlot && migratedAgain.level() == 1);
    assert(migratedAgain.toView().feats.slots.size() == 1 && migratedAgain.toView().feats.feats.size() == 1);
    migratedAgain.save(path);
    auto assigned = CharacterSheet::load(path, catalog);
    assert(assigned.level() == 1 && !assigned.toView().legacyFirstLevelFeatSlot);
    std::filesystem::remove(path);
    return 0;
}
