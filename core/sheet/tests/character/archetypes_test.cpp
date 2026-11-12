#include "../fixtures/class_definition.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/persistence/json.hpp"

#include <nlohmann/json.hpp>
#include <cassert>
#include <filesystem>

namespace
{
    template<class Action> void rejects(Action action)
    {
        bool rejected = false;
        try { action(); } catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }

    golarion::ClassChoiceDefinition choice(const std::string &acquisition, int amount)
    {
        using namespace golarion;
        return {.id = "choice", .prompt = "Scegli", .selectionCount = 1, .options = {{
            .id = "option", .name = "Opzione", .description = "Opzione di prova", .progression = {{
                .id = acquisition, .level = 1,
                .effects = {ContributionEffectDefinition{"value", "baseAttackBonus", std::to_string(amount)}}
            }}
        }}};
    }
}

int main()
{
    using namespace golarion;
    auto fighter = test::classDefinition(CharacterClassType::Fighter);
    fighter.features = {
        {.id = "keep", .name = "Persistente", .description = "Privilegio invariato", .progression = {{
            .id = "keep.1", .level = 1, .effects = {
                FeatSlotGrantEffectDefinition{"feat"},
                SpellGrantEffectDefinition{.id = "spell", .spellId = "fireball", .usage = GrantedSpellUsage::Limited, .capacity = 2}
            }, .choices = {choice("keep.option", 0)}
        }}},
        {.id = "original", .name = "Originale", .description = "Progressione originale", .progression = {
            {.id = "original.1", .level = 1, .effects = {ContributionEffectDefinition{"value", "baseAttackBonus", "1"}}},
            {.id = "original.2", .level = 2, .effects = {ContributionEffectDefinition{"value", "baseAttackBonus", "2"}}, .choices = {choice("original.option", 3)}}
        }},
        {.id = "other", .name = "Altro", .description = "Un altro privilegio", .progression = {{
            .id = "other.1", .level = 1, .effects = {ContributionEffectDefinition{"value", "baseAttackBonus", "5"}}
        }}}
    };
    const ArchetypeDefinition partial{
        .id = "partial", .classType = CharacterClassType::Fighter, .name = "Parziale", .description = "Modifica una soglia",
        .affectedFeatures = {{"original", ArchetypeFeatureChangeType::Alter}}, .removedAcquisitionIds = {"original.2"},
        .addedFeatures = {{.id = "partial.feature", .name = "Nuovo", .description = "Nuova scelta", .progression = {{
            .id = "partial.2", .level = 2, .choices = {choice("partial.option", 7)}
        }}}}
    };
    const ArchetypeDefinition replacement{
        .id = "replacement", .classType = CharacterClassType::Fighter, .name = "Sostituzione", .description = "Sostituisce un privilegio",
        .affectedFeatures = {{"other", ArchetypeFeatureChangeType::Replace}}, .removedFeatureIds = {"other"},
        .addedFeatures = {{.id = "replacement.feature", .name = "Nuovo", .description = "Nuovo privilegio", .progression = {{
            .id = "replacement.1", .level = 1, .effects = {ContributionEffectDefinition{"value", "baseAttackBonus", "10"}}
        }}}}
    };
    auto conflict = partial;
    conflict.id = "conflict";
    conflict.removedAcquisitionIds = {"original.1"};
    conflict.addedFeatures.clear();
    auto failing = replacement;
    failing.id = "failure";
    failing.affectedFeatures = {{"keep", ArchetypeFeatureChangeType::Replace}};
    failing.removedFeatureIds = {"keep"};
    failing.addedFeatures[0].id = "failure.feature";
    failing.addedFeatures[0].progression[0].id = "failure.1";
    failing.addedFeatures[0].progression[0].effects.push_back(ContributionEffectDefinition{"failure", "missing", "1"});
    auto wrongClass = replacement;
    wrongClass.id = "wrongClass";
    wrongClass.classType = CharacterClassType::Wizard;
    const ClassDefinitionManager catalog({fighter, test::classDefinition(CharacterClassType::Wizard)}, {partial, replacement, conflict, failing, wrongClass});
    CharacterSheet sheet(catalog);
    rejects([&] { sheet.addClassArchetype(CharacterClassType::Fighter, "partial"); });
    sheet.addClassLevel(CharacterClassType::Fighter);
    sheet.addClassLevel(CharacterClassType::Fighter);
    sheet.selectClassOption(CharacterClassType::Fighter, "original.2", "choice", "option");
    sheet.selectClassOption(CharacterClassType::Fighter, "keep.1", "choice", "option");
    assert(sheet.toView().baseAttackBonus.total == 13);
    std::string featSlot;
    for (const auto &slot : sheet.toView().feats.slots) if (!slot.id.starts_with("base.")) featSlot = slot.id;
    sheet.selectFeat(featSlot, "improvedInitiative");
    const auto spellId = sheet.toView().grantedSpells[0].id;
    sheet.consumeGrantedSpell(spellId);
    const auto before = persistence::toJson(sheet.toSaveData());
    rejects([&] { sheet.addClassArchetype(CharacterClassType::Fighter, "failure"); });
    assert(persistence::toJson(sheet.toSaveData()) == before);
    assert(sheet.toView().baseAttackBonus.total == 13);
    rejects([&] { sheet.addClassArchetype(CharacterClassType::Fighter, "wrongClass"); });
    rejects([&] { sheet.addClassArchetype(CharacterClassType::Fighter, "missing"); });
    sheet.addClassArchetype(CharacterClassType::Fighter, "partial");
    assert(sheet.toView().baseAttackBonus.total == 8);
    rejects([&] { sheet.selectClassOption(CharacterClassType::Fighter, "original.2", "choice", "option"); });
    rejects([&] { sheet.addClassArchetype(CharacterClassType::Fighter, "partial"); });
    rejects([&] { sheet.addClassArchetype(CharacterClassType::Fighter, "conflict"); });
    sheet.addClassArchetype(CharacterClassType::Fighter, "replacement");
    sheet.selectClassOption(CharacterClassType::Fighter, "partial.2", "choice", "option");
    assert(sheet.toView().baseAttackBonus.total == 20);
    assert(sheet.toView().feats.feats[0].slotId == featSlot);
    assert(sheet.toView().grantedSpells[0].remaining == 1);
    assert(sheet.toView().classes.classes[0].archetypes.size() == 2);
    const auto &data = sheet.toSaveData().classes;
    assert(data.classes[0].choices.size() == 3); // Retain the suppressed original choice.
    const auto path = std::filesystem::temp_directory_path() / "golarion_archetypes_test.json";
    sheet.save(path);
    auto restored = CharacterSheet::load(path, catalog);
    assert(persistence::toJson(restored.toSaveData()) == persistence::toJson(sheet.toSaveData()));
    assert(restored.toView().baseAttackBonus.total == 20);
    assert(restored.toView().grantedSpells[0].remaining == 1);
    restored.unselectClassOption(CharacterClassType::Fighter, "partial.2", "choice", "option");
    assert(restored.toView().baseAttackBonus.total == 13);
    restored.addClassLevel(CharacterClassType::Fighter);
    assert(restored.toView().baseAttackBonus.total == 14);

    CharacterSheet early(catalog);
    early.addClassLevel(CharacterClassType::Fighter);
    early.addClassArchetype(CharacterClassType::Fighter, "partial");
    rejects([&] { early.addClassArchetype(CharacterClassType::Fighter, "conflict"); });
    rejects([&] { early.selectClassOption(CharacterClassType::Fighter, "partial.2", "choice", "option"); });
    early.addClassLevel(CharacterClassType::Fighter);
    early.selectClassOption(CharacterClassType::Fighter, "partial.2", "choice", "option");

    std::vector<ArchetypeDefinition> invalid;
    auto bad = partial;
    bad.affectedFeatures.clear();
    invalid.push_back(bad);
    bad = replacement;
    bad.removedFeatureIds.clear();
    invalid.push_back(bad);
    bad = partial;
    bad.removedAcquisitionIds = {"missing"};
    invalid.push_back(bad);
    bad = partial;
    bad.addedFeatures[0].id = "original";
    invalid.push_back(bad);
    bad = partial;
    bad.addedFeatures[0].progression[0].id = "original.1";
    invalid.push_back(bad);
    bad = partial;
    bad.addedFeatures[0].progression[0].replaces = {"keep.1"};
    invalid.push_back(bad);
    bad = partial;
    bad.addedFeatures[0].progression[0].choices[0].selectionCount = 0;
    invalid.push_back(bad);
    for (const auto &candidate : invalid)
    {
        const ClassDefinitionManager invalidCatalog({fighter}, {candidate});
        CharacterSheet target(invalidCatalog);
        target.addClassLevel(CharacterClassType::Fighter);
        const auto previous = persistence::toJson(target.toSaveData());
        rejects([&] { target.addClassArchetype(CharacterClassType::Fighter, candidate.id); });
        assert(persistence::toJson(target.toSaveData()) == previous);
        assert(target.toView().baseAttackBonus.total == 7);
    }

    auto json = nlohmann::json::parse(persistence::toJson(sheet.toSaveData()));
    json["classes"][0]["archetypeIds"] = {"partial", "partial"};
    rejects([&] { persistence::fromJson(json.dump()); });
    json["classes"][0]["archetypeIds"] = {"missing"};
    persistence::saveToFile(persistence::fromJson(json.dump()), path);
    rejects([&] { CharacterSheet::load(path, catalog); });
    // Format 30 had class selections but no archetypes.
    CharacterSheet old(catalog);
    old.addClassLevel(CharacterClassType::Fighter);
    json = nlohmann::json::parse(persistence::toJson(old.toSaveData()));
    json["formatVersion"] = 30;
    json["classes"][0].erase("archetypeIds");
    persistence::saveToFile(persistence::fromJson(json.dump()), path);
    auto legacy = CharacterSheet::load(path, catalog);
    assert(legacy.level() == 1 && legacy.toView().classes.classes[0].archetypes.empty());

    // Removal forgets archetype choices and resets the affected original feature only.
    sheet.removeClassArchetype(CharacterClassType::Fighter, "partial");
    assert(sheet.toView().baseAttackBonus.total == 15);
    auto afterRemoval = sheet.toView().classes.classes[0];
    assert(afterRemoval.archetypes.size() == 1 && afterRemoval.archetypes[0].id == "replacement");
    assert(afterRemoval.features[1].acquisitions[1].active);
    assert(afterRemoval.features[1].acquisitions[1].choices[0].remainingSelections == 1);
    assert(sheet.toSaveData().classes.classes[0].choices.size() == 1);
    assert(sheet.toSaveData().classes.classes[0].choices[0].acquisitionId == "keep.1");
    assert(sheet.toView().grantedSpells[0].remaining == 1);
    assert(sheet.toView().feats.feats[0].slotId == featSlot);
    rejects([&] { sheet.removeClassArchetype(CharacterClassType::Fighter, "partial"); });
    rejects([&] { sheet.removeClassArchetype(CharacterClassType::Wizard, "replacement"); });
    sheet.save(path);
    auto removalRestored = CharacterSheet::load(path, catalog);
    assert(persistence::toJson(removalRestored.toSaveData()) == persistence::toJson(sheet.toSaveData()));
    sheet.removeClassArchetype(CharacterClassType::Fighter, "replacement");
    assert(sheet.toView().baseAttackBonus.total == 10);
    assert(sheet.toView().classes.classes[0].archetypes.empty());
    sheet.selectClassOption(CharacterClassType::Fighter, "original.2", "choice", "option");
    assert(sheet.toView().baseAttackBonus.total == 13);
    sheet.addClassArchetype(CharacterClassType::Fighter, "partial");
    assert(sheet.toView().baseAttackBonus.total == 8); // Old archetype option was not resurrected.
    sheet.removeClassArchetype(CharacterClassType::Fighter, "partial");
    assert(sheet.toView().baseAttackBonus.total == 10);

    // A failed restoration preserves the archetype, its choices and effects.
    ResourceManager resources;
    for (const auto *name : {"baseAttackBonus", "original", "replacement", "hp.max", "skill.ranks.max"}) resources.registerAccumulatedResource(name);
    resources.registerTarget("intMod", [] { return 0; });
    resources.registerTarget("intPermanentMod", [] { return 0; });
    resources.registerTarget("intSkillRanksMod", [] { return 0; });
    for (auto type : {SavingThrowType::Fortitude, SavingThrowType::Reflex, SavingThrowType::Will}) resources.registerAccumulatedResource(baseResourceName(type));
    auto base = test::levelOnlyClassDefinition();
    base.features = {{.id = "base", .name = "Base", .description = "Privilegio originale", .progression = {{
        .id = "base.1", .level = 1, .effects = {ContributionEffectDefinition{"value", "original", "1"}}
    }}}};
    CharacterClass instance(resources, "rollback", base);
    instance.addArchetype({
        .id = "replacement", .classType = CharacterClassType::Fighter, .name = "Alternativo", .description = "Privilegio alternativo",
        .affectedFeatures = {{"base", ArchetypeFeatureChangeType::Replace}}, .removedFeatureIds = {"base"},
        .addedFeatures = {{.id = "new", .name = "Nuovo", .description = "Nuovo privilegio", .progression = {{
            .id = "new.1", .level = 1, .effects = {ContributionEffectDefinition{"value", "replacement", "5"}}, .choices = {choice("new.option", 7)}
        }}}}
    });
    instance.selectOption("new.1", "choice", "option");
    resources.unregisterAccumulatedResource("original");
    rejects([&] { instance.removeArchetype("replacement"); });
    assert(instance.toData().archetypeIds == std::vector<std::string>{"replacement"});
    assert(instance.toData().choices.size() == 1);
    assert(resources.contributionTotal("replacement") == 5 && resources.contributionTotal("baseAttackBonus") == 7);
    resources.registerAccumulatedResource("original");
    instance.removeArchetype("replacement");
    assert(instance.toData().archetypeIds.empty() && instance.toData().choices.empty());
    assert(resources.contributionTotal("original") == 1 && resources.contributionTotal("replacement") == 0);
    assert(resources.contributionTotal("baseAttackBonus") == 0);
    std::filesystem::remove(path);
    return 0;
}
