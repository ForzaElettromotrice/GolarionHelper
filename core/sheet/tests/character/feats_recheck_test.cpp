#include "golarion/character/feat_definition_manager.hpp"
#include "../fixtures/class_definition.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/character/feats.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <stdexcept>

// Test-local catalog implementation: do not add synthetic feats to the shipped catalog.
namespace golarion
{
    FeatDefinitionManager &FeatDefinitionManager::instance()
    {
        static FeatDefinitionManager manager;
        return manager;
    }

    const FeatDefinition &FeatDefinitionManager::get(std::string_view id)
    {
        static const FeatDefinition fixture{
            .id = "test", .name = "Talento di prova", .description = "Descrizione", .types = {FeatType::Combat},
            .requirements = {{"Forza 13", "@strength >= 13"}},
            .effects = {ModifierEffectDefinition{.id = "bonus", .resource = "initiative", .description = "Bonus", .type = ModifierType::Bonus, .bonusType = BonusType::Generic, .expression = "4"}},
            .choices = {{"choice", "Scegli", 1, {{"option", "Opzione", {{"Destrezza 13", "@dexterity >= 13"}}, {}}}}}
        };
        static const auto catalog = [&]
        {
            std::map<std::string, FeatDefinition> definitions{{fixture.id, fixture}};
            auto root = fixture;
            root.id = "root";
            root.choices.clear();
            definitions.emplace(root.id, root);
            auto nested = root;
            nested.id = "nested";
            nested.effects = {FeatSlotGrantEffectDefinition{"slot", AnyFeat{}}};
            definitions.emplace(nested.id, nested);
            auto brokenNested = nested;
            brokenNested.id = "brokenNested";
            brokenNested.effects.push_back(ContributionEffectDefinition{"failure", "missing.resource", "1"});
            definitions.emplace(brokenNested.id, brokenNested);
            auto favoredProvider = root;
            favoredProvider.id = "favoredProvider";
            favoredProvider.effects = {FavoredClassGrantEffectDefinition{.id = "eligibility", .allClasses = true}};
            definitions.emplace(favoredProvider.id, favoredProvider);
            auto dependent = root;
            dependent.id = "dependent";
            dependent.requirements = {{"Talento base", std::nullopt, FeatDependencyDefinition{"root", {}}}};
            definitions.emplace(dependent.id, dependent);
            auto leaf = dependent;
            leaf.id = "leaf";
            leaf.requirements[0].feat->id = "dependent";
            definitions.emplace(leaf.id, leaf);
            auto cycle = dependent;
            cycle.id = "cycleA";
            cycle.requirements[0].feat->id = "cycleB";
            definitions.emplace(cycle.id, cycle);
            cycle.id = "cycleB";
            cycle.requirements[0].feat->id = "cycleA";
            definitions.emplace(cycle.id, cycle);
            auto repeated = fixture;
            repeated.id = "repeated";
            repeated.requirements.clear();
            repeated.choices[0].options[0].requirements.clear();
            repeated.choices[0].options.push_back({"other", "Altra opzione", {}, {}});
            repeated.repeatability = FeatRepeatability::DifferentChoices;
            definitions.emplace(repeated.id, repeated);
            auto matching = repeated;
            matching.id = "matching";
            matching.requirements = {{"Stessa scelta", std::nullopt, FeatDependencyDefinition{"repeated", {{"choice", "choice"}}}}};
            definitions.emplace(matching.id, matching);
            repeated.id = "unlimited";
            repeated.choices.clear();
            repeated.repeatability = FeatRepeatability::Unlimited;
            definitions.emplace(repeated.id, repeated);
            auto oscillating = root;
            oscillating.id = "oscillating";
            oscillating.requirements = {{"Soglia", "@initiativeTotal < 4"}};
            definitions.emplace(oscillating.id, oscillating);
            auto sheetFeat = root;
            sheetFeat.id = "sheet";
            sheetFeat.requirements = {{"Forza", "@str >= 13"}, {"Informazione manuale"}};
            definitions.emplace(sheetFeat.id, sheetFeat);
            return definitions;
        }();
        const auto found = catalog.find(std::string(id));
        if (found == catalog.end()) throw std::invalid_argument("unknown test feat");
        return found->second;
    }
}

int main()
{
    using namespace golarion;
    ResourceManager resources;
    resources.registerEnhanceableResource("initiative");
    int strength = 13;
    int dexterity = 13;
    bool exempt = false;
    resources.registerTarget("strength", [&] { return strength; });
    resources.registerTarget("dexterity", [&] { return dexterity; });
    Feats feats(resources);
    resources.addToCollection(FeatSlotsResource, FeatSlot{
        .id = "slot", .source = "Fonte",
        .exempts = [&](const auto &, const auto &) { return exempt; }
    });
    const FeatChoices choices{{"choice", {"option"}}};
    feats.selectFeat("slot", "test", choices);
    const auto check = [&](bool active)
    {
        const auto view = feats.toView();
        assert(view.feats.size() == 1 && view.feats[0].active == active);
        assert(view.feats[0].choices[0].selectedOptions[0].name == "Opzione");
        assert(view.slots[0].featId == "test");
        assert(feats.toData().selections[0].choices == choices);
        assert(resources.modifierTotal("initiative") == (active ? 4 : 0));
    };
    check(true);
    resources.removeFromCollection(FeatSlotsResource, "slot");
    check(false);
    assert(!feats.toView().slots[0].available);
    resources.addToCollection(FeatSlotsResource, FeatSlot{
        .id = "slot", .source = "Fonte",
        .exempts = [&](const auto &, const auto &) { return exempt; }
    });
    feats.recheckPrerequisites();
    check(true);
    const auto originalModifier = resources.modifierSetView("initiative").modifiers.front().id;
    feats.recheckPrerequisites();
    check(true);
    assert(resources.modifierSetView("initiative").modifiers.front().id == originalModifier);
    strength = 12;
    feats.recheckPrerequisites();
    check(false);
    feats.recheckPrerequisites();
    check(false);
    const auto saved = feats.toData();
    feats.removeFeat("slot");
    feats.load(saved);
    check(false);
    strength = 13;
    feats.recheckPrerequisites();
    check(true);
    dexterity = 12;
    feats.recheckPrerequisites();
    check(false);
    exempt = true;
    feats.recheckPrerequisites();
    check(true);
    exempt = false;
    resources.addToCollection(FeatPrerequisiteAlternativesResource, FeatPrerequisiteAlternative{
        .id = "alternative", .source = "Fonte", .target = "dexterity",
        .applies = [](const auto &) { return true; },
        .calculate = [](const auto &, const auto &) { return 13; }
    });
    feats.recheckPrerequisites();
    check(true);
    resources.removeFromCollection(FeatPrerequisiteAlternativesResource, "alternative");
    feats.recheckPrerequisites();
    check(false);
    // Failed reactivation retains the acquired selection and remains retryable.
    resources.unregisterEnhanceableResource("initiative");
    dexterity = 13;
    bool failed = false;
    try { feats.recheckPrerequisites(); }
    catch (const std::invalid_argument &) { failed = true; }
    assert(failed && !feats.toView().feats[0].active);
    resources.registerEnhanceableResource("initiative");
    feats.recheckPrerequisites();
    check(true);
    dexterity = 12;
    feats.recheckPrerequisites();
    feats.removeFeat("slot");
    assert(feats.toView().feats.empty() && feats.toView().slots.size() == 1);
    assert(resources.modifierTotal("initiative") == 0);

    feats.setLevel(5);
    strength = 13;
    failed = false;
    try { feats.selectFeat("base.feat.level1", "brokenNested"); }
    catch (const std::invalid_argument &) { failed = true; }
    assert(failed && feats.toData().selections.empty() && feats.toView().slots.size() == 4);
    feats.selectFeat("base.feat.level1", "nested");
    std::string childSlot;
    for (const auto &slot : feats.toView().slots)
        if (slot.id.starts_with("feat:")) childSlot = slot.id;
    assert(!childSlot.empty());
    feats.selectFeat(childSlot, "root");
    const auto nestedSave = feats.toData();
    strength = 12;
    feats.recheckPrerequisites();
    assert(feats.toData().selections.size() == 2);
    for (const auto &feat : feats.toView().feats) assert(!feat.active);
    strength = 13;
    feats.recheckPrerequisites();
    for (const auto &feat : feats.toView().feats) assert(feat.active);
    assert(resources.modifierTotal("initiative") == 4);
    feats.removeFeat("base.feat.level1");
    assert(resources.modifierTotal("initiative") == 0);
    feats.removeFeat(childSlot);
    feats.load(nestedSave);
    assert(resources.modifierTotal("initiative") == 4);
    feats.removeFeat("base.feat.level1");
    feats.removeFeat(childSlot);
    assert(feats.toView().slots.size() == 4);
    // Deliberately load the chain in reverse dependency/slot order.
    feats.load({{{"base.feat.level1", "leaf", {}}, {"base.feat.level3", "dependent", {}}, {"base.feat.level5", "root", {}}}});
    assert(resources.modifierTotal("initiative") == 12);
    feats.setLevel(3);
    assert(feats.toData().selections.size() == 3);
    assert(resources.modifierTotal("initiative") == 0);
    for (const auto &feat : feats.toView().feats) assert(!feat.active);
    feats.setLevel(5);
    assert(resources.modifierTotal("initiative") == 12);
    strength = 12;
    feats.recheckPrerequisites();
    assert(resources.modifierTotal("initiative") == 0);
    for (const auto &feat : feats.toView().feats) assert(!feat.active);
    strength = 13;
    feats.recheckPrerequisites();
    assert(resources.modifierTotal("initiative") == 12);
    feats.removeFeat("base.feat.level5");
    assert(resources.modifierTotal("initiative") == 0);
    feats.removeFeat("base.feat.level1");
    feats.removeFeat("base.feat.level3");
    feats.load({{{"base.feat.level1", "cycleA", {}}, {"base.feat.level3", "cycleB", {}}}});
    for (const auto &feat : feats.toView().feats) assert(!feat.active);
    feats.removeFeat("base.feat.level1");
    feats.removeFeat("base.feat.level3");

    feats.selectFeat("base.feat.level1", "repeated", choices);
    failed = false;
    try { feats.selectFeat("base.feat.level3", "repeated", choices); }
    catch (const std::invalid_argument &) { failed = true; }
    assert(failed);
    feats.selectFeat("base.feat.level3", "repeated", {{"choice", {"other"}}});
    feats.selectFeat("base.feat.level5", "matching", {{"choice", {"other"}}});
    assert(resources.modifierTotal("initiative") == 12);
    feats.removeFeat("base.feat.level3");
    assert(resources.modifierTotal("initiative") == 4);
    const auto repeatedSave = feats.toData();
    feats.removeFeat("base.feat.level1");
    feats.removeFeat("base.feat.level5");
    feats.load(repeatedSave);
    assert(feats.toView().feats[0].slotId == "base.feat.level1");
    assert(!feats.toView().feats[1].active);
    feats.removeFeat("base.feat.level1");
    feats.removeFeat("base.feat.level5");
    feats.selectFeat("base.feat.level1", "unlimited");
    feats.selectFeat("base.feat.level3", "unlimited");
    assert(resources.modifierTotal("initiative") == 8);
    const auto unlimitedSave = feats.toData();
    feats.removeFeat("base.feat.level1");
    assert(resources.modifierTotal("initiative") == 4);
    feats.removeFeat("base.feat.level3");
    feats.load(unlimitedSave);
    assert(resources.modifierTotal("initiative") == 8);
    feats.removeFeat("base.feat.level1");
    feats.removeFeat("base.feat.level3");

    resources.registerTarget("initiativeTotal", [&] { return resources.modifierTotal("initiative"); });
    failed = false;
    try { feats.selectFeat("base.feat.level1", "oscillating"); }
    catch (const std::logic_error &) { failed = true; }
    assert(failed && feats.toData().selections.empty());
    assert(resources.modifierTotal("initiative") == 0);

    const ClassDefinitionManager classCatalog({test::classDefinition(CharacterClassType::Fighter)});
    {
        ResourceManager ownedResources;
        ownedResources.registerEnhanceableResource("initiative");
        ownedResources.registerTarget("strength", [] { return 13; });
        {
            Feats owned(ownedResources);
            owned.setLevel(1);
            owned.selectFeat("base.feat.level1", "nested");
            for (const auto &slot : owned.toView().slots)
                if (slot.id.starts_with("feat:")) owned.selectFeat(slot.id, "root");
        }
        assert(ownedResources.modifierTotal("initiative") == 0);
    }
    {
        ResourceManager nestedResources;
        nestedResources.registerEnhanceableResource("initiative");
        for (const auto *name : {"hp.max", "skill.ranks.max", "baseAttackBonus", "savingThrow.fortitude.base", "savingThrow.reflex.base", "savingThrow.will.base"})
            nestedResources.registerAccumulatedResource(name);
        nestedResources.registerTarget("intMod", [] { return 0; });
        nestedResources.registerTarget("intPermanentMod", [] { return 0; });
        nestedResources.registerTarget("intSkillRanksMod", [] { return 0; });
        nestedResources.registerTarget("conMod", [] { return 0; });
        nestedResources.registerTarget("strength", [&] { return strength; });
        FavoredClasses favored(nestedResources);
        Feats nestedFeats(nestedResources);
        Classes classes(nestedResources, favored, classCatalog);
        classes.addLevel(CharacterClassType::Fighter, 4);
        classes.addLevel(CharacterClassType::Fighter, 4);
        nestedFeats.setLevel(2);
        nestedResources.addToCollection(FavoredClasses::OptionsResource, FavoredClassBonusOption{
            .id = "bonusSlot", .name = "Talento", .description = "Ogni due scelte",
            .effects = {FeatSlotGrantEffectDefinition{"slot"}}, .selectionsPerBenefit = 2
        });
        favored.load({{}, {{CharacterClassType::Fighter, 1, "bonusSlot"}, {CharacterClassType::Fighter, 2, "bonusSlot"}}});
        strength = 13;
        nestedFeats.selectFeat("base.feat.level1", "favoredProvider");
        std::string grantedSlot;
        for (const auto &slot : nestedFeats.toView().slots)
            if (slot.id.starts_with("favored.")) grantedSlot = slot.id;
        assert(!grantedSlot.empty());
        nestedFeats.selectFeat(grantedSlot, "root");
        strength = 12;
        nestedFeats.recheckPrerequisites();
        for (const auto &feat : nestedFeats.toView().feats) assert(!feat.active);
        strength = 13;
        nestedFeats.recheckPrerequisites();
        for (const auto &feat : nestedFeats.toView().feats) assert(feat.active);
        // Destruction also removes source-owned slots without recursively reactivating the provider.
    }
    CharacterSheet sheet(classCatalog);
    sheet.addClassLevel(CharacterClassType::Fighter);
    sheet.setAbilityBaseValue(AbilityType::Strength, 13);
    sheet.selectFeat("base.feat.level1", "sheet");
    assert(sheet.toView().feats.feats[0].active);
    sheet.setAbilityBaseValue(AbilityType::Strength, 12);
    assert(!sheet.toView().feats.feats[0].active);
    sheet.setAbilityBaseValue(AbilityType::Strength, 13);
    assert(sheet.toView().feats.feats[0].active);
}
