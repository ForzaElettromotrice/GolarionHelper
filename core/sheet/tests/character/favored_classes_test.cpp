#include "../fixtures/class_definition.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/persistence/json.hpp"
#include "golarion/effect/effect_json.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <nlohmann/json.hpp>
#include <cassert>
#include <filesystem>
#include <limits>
#include <stdexcept>

namespace
{
    template<class Action> void rejects(Action action)
    {
        bool rejected = false;
        try { action(); } catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }

    golarion::ClassDefinition fighterDefinition()
    {
        auto definition = golarion::test::classDefinition(golarion::CharacterClassType::Fighter);
        for (int level = 4; level <= 6; ++level)
        {
            auto row = definition.progression.back();
            row.level = level;
            definition.progression.push_back(row);
        }
        return definition;
    }
}

int main()
{
    using namespace golarion;
    const ClassDefinitionManager catalog({fighterDefinition(), test::classDefinition(CharacterClassType::Wizard)});
    ResourceManager resources;
    for (const auto *name : {"hp.max", "skill.ranks.max", "baseAttackBonus", "savingThrow.fortitude.base", "savingThrow.reflex.base", "savingThrow.will.base"})
        resources.registerAccumulatedResource(name);
    resources.registerTarget("conMod", [] { return 0; });
    resources.registerTarget("intMod", [] { return 0; });
    resources.registerTarget("intPermanentMod", [] { return 0; });
    resources.registerTarget("intSkillRanksMod", [] { return 0; });
    resources.registerEnhanceableResource("initiative");
    FavoredClasses favored(resources);
    Feats feats(resources);
    {
        Classes classes(resources, favored, catalog);
        classes.setFavoredClass(CharacterClassType::Fighter); // May be chosen before taking a level.
        rejects([&] { classes.setFavoredClasses({CharacterClassType::Fighter, CharacterClassType::Wizard}); });
        rejects([&] { classes.selectFavoredClassBonus(CharacterClassType::Fighter, 1, "base.hitPoint"); });
        for (int level = 1; level <= 6; ++level) classes.addLevel(CharacterClassType::Fighter, 4);
        classes.addLevel(CharacterClassType::Wizard, 4);
        classes.selectFavoredClassBonus(CharacterClassType::Fighter, 1, "base.hitPoint");
        classes.selectFavoredClassBonus(CharacterClassType::Fighter, 2, "base.skillRank");
        assert(resources.contributionTotal("hp.max") == 29 && resources.contributionTotal("skill.ranks.max") == 15);
        classes.selectFavoredClassBonus(CharacterClassType::Fighter, 1, "base.skillRank");
        assert(resources.contributionTotal("hp.max") == 28 && resources.contributionTotal("skill.ranks.max") == 16);
        rejects([&] { classes.selectFavoredClassBonus(CharacterClassType::Fighter, 7, "base.skillRank"); });
        rejects([&] { classes.selectFavoredClassBonus(CharacterClassType::Wizard, 1, "base.hitPoint"); });
        rejects([&] { classes.selectFavoredClassBonus(CharacterClassType::Fighter, 1, "missing"); });

        resources.addToCollection(FavoredClasses::GrantsResource, FavoredClassGrant{"halfElf", "Doti innate", 1});
        classes.setFavoredClasses({CharacterClassType::Fighter, CharacterClassType::Wizard});
        classes.selectFavoredClassBonus(CharacterClassType::Wizard, 1, "base.hitPoint");
        resources.removeFromCollection(FavoredClasses::GrantsResource, "halfElf");
        assert(classes.toView().favoredClasses.tooManySelectedClasses);
        assert(resources.contributionTotal("hp.max") == 28 && resources.contributionTotal("skill.ranks.max") == 14);
        assert(classes.toData().favoredClasses.bonuses.size() == 3);
        classes.setFavoredClass(CharacterClassType::Fighter);
        assert(resources.contributionTotal("skill.ranks.max") == 16);
        resources.addToCollection(FavoredClasses::GrantsResource, FavoredClassGrant{.id = "mastery", .source = "Padronanza", .allClasses = true});
        assert(resources.contributionTotal("hp.max") == 29); // The retained wizard selection becomes active again.
        resources.removeFromCollection(FavoredClasses::GrantsResource, "mastery");

        FavoredClassBonusOption combined{
            .id = "fastLearner", .name = "Apprendimento rapido", .description = "Entrambi i benefici standard.",
            .classes = {CharacterClassType::Fighter},
            .effects = {ContributionEffectDefinition{"hp", "hp.max", "1"}, ContributionEffectDefinition{"rank", "skill.ranks.max", "1"}},
            .replaces = {FavoredClassBaseBonus::HitPoint, FavoredClassBaseBonus::SkillRank}
        };
        resources.addToCollection(FavoredClasses::OptionsResource, combined);
        assert(resources.contributionTotal("skill.ranks.max") == 14); // No implicit conversion of previous choices.
        rejects([&] { classes.selectFavoredClassBonus(CharacterClassType::Fighter, 3, "base.hitPoint"); });
        classes.selectFavoredClassBonus(CharacterClassType::Fighter, 1, "fastLearner");
        assert(resources.contributionTotal("hp.max") == 29 && resources.contributionTotal("skill.ranks.max") == 15);
        resources.removeFromCollection(FavoredClasses::OptionsResource, "fastLearner");
        assert(resources.contributionTotal("hp.max") == 28 && resources.contributionTotal("skill.ranks.max") == 15);
        resources.addToCollection(FavoredClasses::OptionsResource, combined);
        assert(resources.contributionTotal("hp.max") == 29);
        resources.removeFromCollection(FavoredClasses::OptionsResource, "fastLearner");

        FavoredClassBonusOption fractional{
            .id = "racial.sixth", .name = "Beneficio frazionario", .description = "Un beneficio ogni sei scelte.",
            .classes = {CharacterClassType::Fighter},
            .effects = {ContributionEffectDefinition{"benefit", "hp.max", "3"}}, .selectionsPerBenefit = 6
        };
        resources.addToCollection(FavoredClasses::OptionsResource, fractional);
        for (int level = 1; level <= 5; ++level) classes.selectFavoredClassBonus(CharacterClassType::Fighter, level, "racial.sixth");
        assert(resources.contributionTotal("hp.max") == 28);
        classes.selectFavoredClassBonus(CharacterClassType::Fighter, 6, "racial.sixth");
        assert(resources.contributionTotal("hp.max") == 31);
        classes.clearFavoredClassBonus(CharacterClassType::Fighter, 6);
        assert(resources.contributionTotal("hp.max") == 28);
        classes.selectFavoredClassBonus(CharacterClassType::Fighter, 6, "racial.sixth");
        resources.removeFromCollection(FavoredClasses::OptionsResource, "racial.sixth");
        assert(resources.contributionTotal("hp.max") == 28);
        resources.addToCollection(FavoredClasses::OptionsResource, fractional);
        assert(resources.contributionTotal("hp.max") == 31);

        resources.addToCollection(FavoredClasses::OptionsResource, FavoredClassBonusOption{
            .id = "broken", .name = "Errore", .description = "Rollback.",
            .effects = {ContributionEffectDefinition{"first", "hp.max", "99"}, ContributionEffectDefinition{"second", "missing", "1"}}
        });
        rejects([&] { classes.selectFavoredClassBonus(CharacterClassType::Fighter, 1, "broken"); });
        assert(resources.contributionTotal("hp.max") == 31);
        assert(classes.toData().favoredClasses.bonuses.front().optionId == "racial.sixth");
        rejects([&] { resources.addToCollection(FavoredClasses::OptionsResource, fractional); });
        fractional.id = "invalid";
        fractional.selectionsPerBenefit = 0;
        rejects([&] { resources.addToCollection(FavoredClasses::OptionsResource, fractional); });
        rejects([&] { resources.removeFromCollection(FavoredClasses::OptionsResource, "base.hitPoint"); });
        resources.addToCollection(FavoredClasses::OptionsResource, FavoredClassBonusOption{
            .id = "nestedSlot", .name = "Talento", .description = "Talento aggiuntivo.",
            .effects = {FeatSlotGrantEffectDefinition{"slot"}}, .selectionsPerBenefit = 2
        });
        classes.selectFavoredClassBonus(CharacterClassType::Fighter, 1, "nestedSlot");
        assert(feats.toView().slots.empty());
        classes.selectFavoredClassBonus(CharacterClassType::Fighter, 2, "nestedSlot");
        assert(feats.toView().slots.size() == 1);
        const auto bonusSlot = feats.toView().slots.front().id;
        feats.selectFeat(bonusSlot, "improvedInitiative");
        classes.clearFavoredClassBonus(CharacterClassType::Fighter, 2);
        assert(!feats.toView().slots.front().available && !feats.toView().feats.front().active);
        classes.selectFavoredClassBonus(CharacterClassType::Fighter, 2, "nestedSlot");
        feats.recheckPrerequisites();
        assert(feats.toView().feats.front().active);
        rejects([&] { resources.addToCollection(FavoredClasses::GrantsResource, FavoredClassGrant{"overflow", "Errore", std::numeric_limits<int>::max()}); });
    }
    assert(resources.contributionTotal("hp.max") == 0 && resources.contributionTotal("skill.ranks.max") == 0);
    // Endpoints outlive Classes, allowing later source destructors to remove their grants safely.
    resources.removeFromCollection(FavoredClasses::OptionsResource, "racial.sixth");
    resources.removeFromCollection(FavoredClasses::OptionsResource, "broken");

    const auto path = std::filesystem::temp_directory_path() / "golarion_favored_classes_test.json";
    CharacterSheet sheet(catalog);
    sheet.setFavoredClass(CharacterClassType::Fighter);
    sheet.addClassLevel(CharacterClassType::Fighter, 4);
    sheet.addClassLevel(CharacterClassType::Fighter, 4);
    sheet.selectFavoredClassBonus(CharacterClassType::Fighter, 1, "base.hitPoint");
    sheet.selectFavoredClassBonus(CharacterClassType::Fighter, 2, "base.skillRank");
    assert(sheet.toView().hitPoints.max == 9 && sheet.toView().skills.maxRanks == 5);
    sheet.damage(2, DamageLethality::Lethal);
    sheet.save(path);
    auto restored = CharacterSheet::load(path, catalog);
    assert(restored.toView().hitPoints.current == 7 && restored.toView().skills.maxRanks == 5);
    assert(persistence::toJson(sheet.toSaveData()) == persistence::toJson(restored.toSaveData()));
    const auto valid = nlohmann::json::parse(persistence::toJson(sheet.toSaveData()));
    auto malformed = valid;
    malformed["favoredClasses"]["selectedClasses"] = {"fighter", "fighter"};
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = valid;
    malformed["favoredClasses"]["bonuses"][0]["classLevel"] = 1.5;
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = valid;
    malformed["favoredClasses"]["bonuses"].push_back(malformed["favoredClasses"]["bonuses"][0]);
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = valid;
    malformed["favoredClasses"]["bonuses"][0]["classLevel"] = 3;
    persistence::saveToFile(persistence::fromJson(malformed.dump()), path);
    rejects([&] { CharacterSheet::load(path, catalog); });
    auto legacy = valid;
    legacy["formatVersion"] = 32;
    legacy.erase("favoredClasses");
    persistence::saveToFile(persistence::fromJson(legacy.dump()), path);
    auto migrated = CharacterSheet::load(path, catalog);
    assert(migrated.toView().classes.favoredClasses.selectedClasses.empty());
    assert(migrated.toView().hitPoints.max == 8 && migrated.toView().skills.maxRanks == 4);

    // A definition-owned provider is reconstructed before the saved bonus selections.
    const auto optionJson = nlohmann::json::parse(R"({
        "type":"favoredClassBonus", "id":"bonus", "name":"Bonus sintetico", "description":"Effetto scalabile.",
        "classes":["fighter"], "replaces":["hitPoint","skillRank"], "selectionsPerBenefit":1,
        "effects":[{"type":"contribution","id":"hp","resource":"hp.max","expression":"@classLevel"}]
    })");
    auto provider = fighterDefinition();
    provider.features = {{.id = "provider", .name = "Fonte", .description = "Fonte sintetica.", .progression = {
        {.id = "source", .level = 1, .effects = {
            effectDefinitionFromJson(optionJson),
            effectDefinitionFromJson(nlohmann::json::parse(R"({"type":"favoredClassGrant","id":"eligible","classes":["fighter"]})"))
        }}
    }}};
    const ClassDefinitionManager providerCatalog({provider});
    CharacterSheet sourced(providerCatalog);
    sourced.addClassLevel(CharacterClassType::Fighter, 4);
    sourced.addClassLevel(CharacterClassType::Fighter, 4);
    std::string providedId;
    const auto sourcedView = sourced.toView();
    for (const auto &option : sourcedView.classes.favoredClasses.classes[0].options)
        if (option.available) providedId = option.id;
    assert(!providedId.empty());
    sourced.selectFavoredClassBonus(CharacterClassType::Fighter, 1, providedId);
    assert(sourced.toView().hitPoints.max == 10);
    sourced.save(path);
    auto sourcedAgain = CharacterSheet::load(path, providerCatalog);
    assert(sourcedAgain.toView().hitPoints.max == 10);
    assert(persistence::toJson(sourced.toSaveData()) == persistence::toJson(sourcedAgain.toSaveData()));
    for (const auto &field : {"classes", "replaces"})
    {
        auto bad = optionJson;
        bad[field] = {"unknown"};
        rejects([&] { effectDefinitionFromJson(bad); });
    }
    auto bad = optionJson;
    bad["selectionsPerBenefit"] = 1.5;
    rejects([&] { effectDefinitionFromJson(bad); });
    bad = optionJson;
    bad["effects"] = {optionJson};
    rejects([&] { effectDefinitionFromJson(bad); });
    bad = optionJson;
    bad["effects"].push_back(bad["effects"][0]);
    rejects([&] { effectDefinitionFromJson(bad); });
    std::filesystem::remove(path);
}
