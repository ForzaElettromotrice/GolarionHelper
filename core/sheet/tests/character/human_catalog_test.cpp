#include "../fixtures/advance_sheet.hpp"
#include "../fixtures/class_definition.hpp"
#include "golarion/persistence/json.hpp"
#include "golarion/character/race_definition_manager.hpp"
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <set>

namespace
{
    using namespace golarion;
    template<class F> void rejects(F action)
    {
        bool rejected = false;
        try { action(); } catch (const std::invalid_argument &) { rejected = true; }
        assert(rejected);
    }
    SkillView skill(CharacterSheet &sheet, SkillType type)
    {
        auto view = sheet.toView();
        return *std::ranges::find(view.skills.skills, type, &SkillView::type);
    }
}

int main()
{
    using namespace golarion;
    const auto &human = RaceDefinitionManager::instance().get("human");
    const std::string reviewMarker = "Ehi, appena hai tempo vedi di correggermi se necessario!";
    for (const auto *elements : {&human.qualities, &human.standardFeatures, &human.alternateFeatures})
        for (const auto &element : *elements)
            for (const auto &effect : element.effects)
                if (const auto *reminder = std::get_if<ReminderEffectDefinition>(&effect))
                    assert(reminder->message.ends_with(reviewMarker));
    const std::set<std::string> expected{
        "dualTalent", "heartOfTheSun", "heartOfTheFields", "heartOfTheStreets", "heartOfTheSnows",
        "heartOfTheSea", "heartOfTheMountains", "heartOfTheSlums", "heartOfTheWilderness", "piety",
        "psychicDefense", "aquaticAncestry", "giantAncestry", "reptilianAncestry", "heroic",
        "silverTongued", "adoptiveParentage", "rationalize", "mixedHeritage", "eyeForTalent", "focusedStudy"
    };
    std::set<std::string> actual;
    for (const auto &trait : human.alternateFeatures) actual.insert(trait.id.substr(6));
    assert(actual == expected);
    const auto &favored = *std::ranges::find(human.qualities, "human.favoredClassOptions", &RacialElementDefinition::id);
    assert(favored.effects.size() == 40);
    std::set<CharacterClassType> coveredClasses;
    for (const auto &effect : favored.effects)
    {
        const auto &bonus = std::get<FavoredClassBonusEffectDefinition>(effect);
        assert(bonus.classes.size() == 1);
        const auto type = *bonus.classes.begin();
        coveredClasses.insert(type);
        for (const auto &benefit : bonus.effects)
            if (const auto *reminder = std::get_if<ReminderEffectDefinition>(&benefit))
                assert(reminder->message.ends_with(reviewMarker));
        auto definition = test::classDefinition(type);
        for (int level = 4; level <= 6; ++level)
        {
            auto progression = definition.progression.back();
            progression.level = level;
            definition.progression.push_back(progression);
        }
        const auto other = type == CharacterClassType::Fighter ? CharacterClassType::Wizard : CharacterClassType::Fighter;
        ClassDefinitionManager manager({definition, test::classDefinition(other)});
        CharacterSheet character(manager);
        for (int level = 0; level < 6; ++level) test::advanceSheetFixture(character, type);
        test::advanceSheetFixture(character, other);
        character.setRace("human");
        const auto classView = [&] (CharacterSheet &sheet) {
            auto classes = sheet.toView().classes.favoredClasses.classes;
            return *std::ranges::find(classes, type, &FavoredClassView::type);
        };
        auto view = classView(character);
        assert(!view.favored);
        const auto option = std::ranges::find(view.options, bonus.name, &FavoredClassBonusOptionView::name);
        assert(option != view.options.end() && !option->available);
        const auto id = option->id;
        rejects([&] { character.selectFavoredClassBonus(type, 1, id); });
        character.setFavoredClass(other);
        rejects([&] { character.selectFavoredClassBonus(type, 1, id); });
        rejects([&] { character.selectFavoredClassBonus(other, 1, id); });
        character.setFavoredClass(type);
        for (int level = 1; level <= bonus.selectionsPerBenefit; ++level)
        {
            character.selectFavoredClassBonus(type, level, id);
            view = classView(character);
            const auto selected = std::ranges::find(view.options, id, &FavoredClassBonusOptionView::id);
            assert(selected->benefits == level / bonus.selectionsPerBenefit);
        }
        const auto checkConditional = [&](SkillType skillType) {
            const auto modifiers = skill(character, skillType).modifiers;
            assert(modifiers.permanentTotal == 0);
            assert(modifiers.conditionalTotals.size() == 1 && modifiers.conditionalTotals.front().value == 1);
        };
        if (bonus.id == "druidAttitude") { checkConditional(SkillType::Diplomacy); checkConditional(SkillType::Intimidate); }
        if (bonus.id == "inquisitorPerception") checkConditional(SkillType::Perception);
        if (bonus.id == "occultistRitual") checkConditional(SkillType::Acrobatics);
        if (bonus.id == "vigilanteSeamlessGuise") checkConditional(SkillType::Disguise);
        if (type == CharacterClassType::Warpriest)
            assert(std::ranges::any_of(character.toView().feats.slots, [](const auto &slot) {
                return slot.available && slot.allowedType == FeatType::Combat;
            }));
        const auto path = std::filesystem::temp_directory_path() / "golarion-human-favored-test.json";
        character.save(path);
        auto restored = CharacterSheet::load(path, manager);
        std::filesystem::remove(path);
        assert(persistence::toJson(restored.toSaveData()) == persistence::toJson(character.toSaveData()));
        restored.clearRace();
        view = classView(restored);
        assert(!view.selections.front().active);
        restored.setRace("human");
        view = classView(restored);
        assert(view.selections.front().active);
        const auto selected = std::ranges::find(view.options, id, &FavoredClassBonusOptionView::id);
        assert(selected != view.options.end() && selected->benefits == 1);
    }
    assert(coveredClasses.size() == 36);
    // Every catalog trait must apply and reverse without missing resource endpoints.
    for (const auto &trait : human.alternateFeatures)
    {
        CharacterSheet sheet;
        sheet.setRace("human");
        sheet.setRacialChoice("human.abilityScores", "ability", {"strength"});
        sheet.selectAlternateRacialFeature(trait.id);
        const auto view = sheet.toView();
        assert(std::ranges::find(view.race.activeElements, trait.id, &RacialElementView::id) != view.race.activeElements.end());
        for (const auto &replaced : trait.replaces)
            assert(std::ranges::find(view.race.activeElements, replaced, &RacialElementView::id) == view.race.activeElements.end());
        sheet.removeAlternateRacialFeature(trait.id);
        assert(sheet.toView().race.activeElements.size() == 8);
    }

    ClassDefinitionManager classes({test::classDefinition(CharacterClassType::Fighter)});
    CharacterSheet sheet(classes);
    for (int i = 0; i < 3; ++i) test::advanceSheetFixture(sheet, CharacterClassType::Fighter);
    sheet.setRace("human");
    sheet.setRacialChoice("human.abilityScores", "ability", {"strength"});
    const int baseRanks = sheet.toView().skills.maxRanks;
    sheet.selectAlternateRacialFeature("human.aquaticAncestry");
    assert(skill(sheet, SkillType::Swim).modifiers.permanentTotal == 1);
    assert(skill(sheet, SkillType::Swim).classSkill);
    assert(sheet.toView().skills.maxRanks == baseRanks - 3);
    rejects([&] { sheet.selectAlternateRacialFeature("human.heartOfTheSea"); });
    sheet.selectAlternateRacialFeature("human.psychicDefense"); // Replaces a different baseline feature.
    sheet.removeAlternateRacialFeature("human.aquaticAncestry");
    assert(!skill(sheet, SkillType::Swim).classSkill);
    assert(sheet.toView().skills.maxRanks == baseRanks);
    sheet.selectAlternateRacialFeature("human.heartOfTheSlums");
    assert(skill(sheet, SkillType::Stealth).modifiers.permanentTotal == 2);
    assert(skill(sheet, SkillType::SleightOfHand).modifiers.permanentTotal == 2);
    const auto survival = skill(sheet, SkillType::Survival).modifiers;
    assert(survival.permanentTotal == 0);
    assert(survival.conditionalTotals.size() == 1 && survival.conditionalTotals[0].value == 4);
    sheet.removeAlternateRacialFeature("human.heartOfTheSlums");
    sheet.selectAlternateRacialFeature("human.giantAncestry");
    assert(skill(sheet, SkillType::Stealth).modifiers.permanentTotal == -2);
    sheet.removeAlternateRacialFeature("human.giantAncestry");
    sheet.selectAlternateRacialFeature("human.heartOfTheWilderness");
    assert(skill(sheet, SkillType::Survival).modifiers.permanentTotal == 1);

    sheet.setAbilityBaseValue(AbilityType::Intelligence, 14);
    auto languages = sheet.toView().languages;
    assert(languages.options.size() == 2);
    const auto &options = languages.options[0].languageIds;
    for (const auto *id : {"draconic", "kelish", "ancientOsiriani", "necril", "tengu"})
        assert(std::ranges::find(options, id) != options.end());
    for (const auto *id : {"druidic", "darkFolk", "sakvroth", "drowSignLanguage"})
    {
        assert(std::ranges::find(options, id) == options.end());
        rejects([&] { sheet.addLanguage(id, LanguageOrigin::Intelligence); });
    }
    sheet.addLanguage("necril", LanguageOrigin::Intelligence);
    sheet.addLanguage("kelish", LanguageOrigin::Intelligence);
    rejects([&] { sheet.addLanguage("draconic", LanguageOrigin::Intelligence); });
    sheet.setSkillRanks(SkillType::Linguistics, 1);
    sheet.addLanguage("draconic", LanguageOrigin::Linguistics);
    rejects([&] { sheet.addLanguage("tengu", LanguageOrigin::Linguistics); });
    const auto before = persistence::toJson(sheet.toSaveData());
    const auto path = std::filesystem::temp_directory_path() / "golarion-human-catalog-test.json";
    sheet.save(path);
    auto restored = CharacterSheet::load(path, classes);
    std::filesystem::remove(path);
    assert(persistence::toJson(restored.toSaveData()) == before);
    assert(restored.toView().languages.spoken.size() == 4);
    assert(skill(restored, SkillType::Survival).modifiers.permanentTotal == 1);
    restored.clearRace();
    assert(restored.toView().languages.options.empty());
    assert(restored.toView().languages.spoken.size() == 3); // Learned languages survive source withdrawal.
}
