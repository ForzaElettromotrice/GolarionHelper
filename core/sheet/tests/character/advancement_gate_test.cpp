#include "golarion/character/character_sheet.hpp"
#include "golarion/persistence/json.hpp"
#include "../fixtures/class_definition.hpp"
#include <cassert>
#include <filesystem>
#include <stdexcept>

namespace
{
    using namespace golarion;
    void selectHumanFeat(CharacterSheet &sheet)
    {
        sheet.addCustomFeatDefinition({.id = "gate.human", .name = "Talento umano di prova", .description = "Nessun effetto.", .types = {FeatType::General}});
        for (const auto &slot : sheet.toView().feats.slots)
            if (slot.origin == FeatSlotOrigin::Source && !slot.featId) sheet.selectFeat(slot.id, "gate.human");
    }
    void blocked(CharacterSheet &sheet, CharacterClassType type)
    {
        const auto before = persistence::toJson(sheet.toSaveData());
        bool rejected = false;
        try { sheet.addClassLevel(type); }
        catch (const std::invalid_argument &error)
        {
            rejected = std::string(error.what()) == "cannot advance while required choices are incomplete";
        }
        assert(rejected);
        assert(persistence::toJson(sheet.toSaveData()) == before);
    }
}

int main()
{
    using namespace golarion;
    auto fighter = test::classDefinition(CharacterClassType::Fighter);
    fighter.features = {{"training", "Addestramento", "Prova", {
        {.id = "first", .level = 1, .choices = {
            {"required", "Scelta obbligatoria", 1, {{"a", "A", "A"}}},
            {"optional", "Scelta non bloccante", 1, {{"a", "A", "A"}}, false}
        }},
        {.id = "second", .level = 2, .effects = {
            FeatSlotGrantEffectDefinition{"bonus"}, FavoredClassGrantEffectDefinition{"favored", 1}
        }}
    }}};
    ClassDefinitionManager catalog({fighter, test::classDefinition(CharacterClassType::Rogue)});
    CharacterSheet sheet(catalog);
    blocked(sheet, CharacterClassType::Fighter);
    sheet.setAlignment(Alignment::Neutral);
    blocked(sheet, CharacterClassType::Fighter);
    sheet.setRace("human");
    blocked(sheet, CharacterClassType::Fighter);
    sheet.setFavoredClass(CharacterClassType::Fighter);
    blocked(sheet, CharacterClassType::Fighter); // Racial ability choice still missing.
    sheet.setRacialChoice("human.abilityScores", "ability", {"charisma"});
    blocked(sheet, CharacterClassType::Fighter); // Human Bonus Feat must also be selected.
    selectHumanFeat(sheet);
    sheet.addClassLevel(CharacterClassType::Fighter);
    assert(sheet.level() == 1);
    sheet.addExtraFeatSlot("extra");
    blocked(sheet, CharacterClassType::Rogue); // Gate applies across class types.
    sheet.selectFeat("base.feat.level1", "improvedInitiative");
    blocked(sheet, CharacterClassType::Fighter);
    sheet.selectClassOption(CharacterClassType::Fighter, {"first", "required", "a"});
    // Missing HP, ranks, favored bonus, optional class choice and extra feat do not block.
    sheet.addClassLevel(CharacterClassType::Fighter);
    assert(sheet.level() == 2);
    blocked(sheet, CharacterClassType::Rogue);
    sheet.setFavoredClasses({CharacterClassType::Fighter, CharacterClassType::Rogue});
    blocked(sheet, CharacterClassType::Rogue); // Source-owned feat slot is still empty.
    sheet.addCustomFeatDefinition({.id = "gate.feat", .name = "Talento di prova", .description = "Nessun effetto.", .types = {FeatType::General}});
    for (const auto &slot : sheet.toView().feats.slots)
        if (slot.origin == FeatSlotOrigin::Source && !slot.id.starts_with("base.") && !slot.featId) sheet.selectFeat(slot.id, "gate.feat");
    sheet.addClassLevel(CharacterClassType::Rogue);
    assert(sheet.level() == 3);
    blocked(sheet, CharacterClassType::Fighter); // New level-three feat slot.
    const auto path = std::filesystem::temp_directory_path() / "golarion-advancement-gate-test.json";
    const auto before = persistence::toJson(sheet.toSaveData());
    sheet.save(path);
    auto restored = CharacterSheet::load(path, catalog);
    assert(persistence::toJson(restored.toSaveData()) == before);
    blocked(restored, CharacterClassType::Rogue);
    restored.removeClassLevel(CharacterClassType::Rogue); // Incomplete choices never prevent -1.
    assert(restored.level() == 2);
    restored.removeClassLevel(CharacterClassType::Fighter);
    restored.removeClassLevel(CharacterClassType::Fighter);
    assert(restored.level() == 0);
    std::filesystem::remove(path);

    // Known-spell quotas block every casting mode, including free prepared additions.
    for (const auto mode : {SpellcastingMode::Prepared, SpellcastingMode::Spontaneous, SpellcastingMode::Flexible})
    {
        const std::string castingId = "class.effect.6:wizard5:start7:casting";
        const std::string quotaId = "class.effect.6:wizard5:start5:known";
        auto wizard = test::classDefinition(CharacterClassType::Wizard);
        wizard.features = {{"spells", "Incantesimi", "Prova", {
            {.id = "start", .level = 1, .effects = {
                SpellcastingGrantEffectDefinition{"casting", mode, CharacterClassType::Wizard},
                SpellRepertoireQuotaEffectDefinition{"known", castingId, {{3, 1}}}
            }}
        }}};
        ClassDefinitionManager spellCatalog({wizard, test::classDefinition(CharacterClassType::Rogue)});
        CharacterSheet caster(spellCatalog);
        caster.setAlignment(Alignment::Neutral);
        caster.setRace("human");
        caster.setRacialChoice("human.abilityScores", "ability", {"charisma"});
        selectHumanFeat(caster);
        caster.setFavoredClass(CharacterClassType::Wizard);
        caster.addClassLevel(CharacterClassType::Wizard);
        caster.selectFeat("base.feat.level1", "improvedInitiative");
        if (mode != SpellcastingMode::Spontaneous)
            caster.addSpellToRepertoire(castingId, "fireball"); // Manual knowledge does not fill the quota.
        blocked(caster, CharacterClassType::Wizard);
        blocked(caster, CharacterClassType::Rogue);
        caster.selectRepertoireSpell(castingId, {quotaId, 3, "fireball"});
        caster.addClassLevel(CharacterClassType::Wizard);
        assert(caster.level() == 2);
        caster.removeRepertoireSelection(castingId, {quotaId, 3, "fireball"});
        blocked(caster, CharacterClassType::Wizard);
        caster.removeClassLevel(CharacterClassType::Wizard);
        assert(caster.level() == 1);
    }
}
