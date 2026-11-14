#include "../fixtures/class_definition.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/character/race_definition_catalog.hpp"
#include "golarion/character/race_definition_manager.hpp"
#include "golarion/persistence/json.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <set>

namespace
{
    using namespace golarion;
    constexpr std::string_view Marker = "Ehi, appena hai tempo vedi di correggermi se necessario!";

    void checkReminders(const nlohmann::json &value)
    {
        if (value.is_object() && value.value("type", "") == "reminder")
            assert(value.at("message").get<std::string>().ends_with(Marker));
        if (value.is_object() || value.is_array())
            for (const auto &child : value) checkReminders(child);
    }

    void exerciseChoices(CharacterSheet &sheet, const RacialElementDefinition &element)
    {
        for (const auto &choice : element.choices)
            for (std::size_t start = 0; start < choice.options.size(); ++start)
            {
                std::vector<std::string> selected;
                for (std::size_t index = 0; index < choice.selectionCount; ++index)
                    selected.push_back(choice.options[(start + index) % choice.options.size()].id);
                sheet.setRacialChoice(element.id, choice.id, selected);
                static_cast<void>(sheet.toView());
            }
    }

    void checkOtherRaceExamples()
    {
        CharacterSheet sheet;
        sheet.setRace("centaur");
        auto view = sheet.toView();
        assert(view.carryingCapacity.bodyType == CarryingBodyType::Quadruped);
        assert(view.carryingCapacity.size == SizeCategory::Large);
        assert(view.movement.grants.front().baseUnits == 10);
        sheet.setRace("drider");
        view = sheet.toView();
        assert(view.specialDefenses.spellResistances.front().effectiveValue == 11);
        const auto climb = std::ranges::find(view.skills.skills, SkillType::Climb, &SkillView::type);
        assert(climb->modifiers.permanentTotal == 8);
        assert(view.movement.grants.size() == 2);
        sheet.setRace("gargoyle");
        view = sheet.toView();
        assert(view.specialDefenses.damageReductions.front().baseValue == 10);
        const auto flight = std::ranges::find(view.movement.grants, MovementType::Fly, &MovementGrantView::type);
        assert(flight->baseUnits == 10 && flight->maneuverability == Maneuverability::Average);
        sheet.setRace("ghoran");
        view = sheet.toView();
        const auto arcana = std::ranges::find(view.skills.skills, SkillType::KnowledgeArcana, &SkillView::type);
        assert(arcana->classSkill);
        assert(view.specialDefenses.immunities.empty()); // Not the usual plant immunities.
        sheet.setRace("android");
        view = sheet.toView();
        assert(view.specialDefenses.immunities.size() == 2);
        const auto senseMotive = std::ranges::find(view.skills.skills, SkillType::SenseMotive, &SkillView::type);
        assert(senseMotive->modifiers.permanentTotal == -4);
        sheet.setRace("yaddithian");
        view = sheet.toView();
        assert(view.specialDefenses.energyResistances.front().energy == DamageType::Cold);
        assert(view.specialDefenses.energyResistances.front().effectiveValue == 5);
        sheet.clearRace();
        view = sheet.toView();
        assert(view.specialDefenses.energyResistances.empty());
        assert(view.specialDefenses.spellResistances.empty());
        assert(view.specialDefenses.damageReductions.empty());
    }

    void checkCoreRaceExamples()
    {
        CharacterSheet sheet;
        sheet.setRace("elf");
        sheet.selectAlternateRacialFeature("elf.woodcraft");
        auto view = sheet.toView();
        for (auto type : {SkillType::KnowledgeNature, SkillType::Survival})
        {
            const auto skill = std::ranges::find(view.skills.skills, type, &SkillView::type);
            assert(skill->modifiers.permanentTotal == 1);
            assert(skill->modifiers.conditionalTotals.size() == 1);
            assert(skill->modifiers.permanentTotal + skill->modifiers.conditionalTotals.front().value == 2);
        }
        sheet.setRace("drowNoble");
        view = sheet.toView();
        for (const auto &options : view.languages.options)
        {
            const bool signLanguage = std::ranges::find(options.languageIds, "drowSignLanguage") != options.languageIds.end();
            assert(signLanguage == (options.origin == LanguageOrigin::Intelligence));
        }
        sheet.setRace("halfling");
        for (const auto &[trait, type] : {
            std::pair{"halfling.watchful", SkillType::SenseMotive},
            std::pair{"halfling.greed", SkillType::Appraise}})
        {
            sheet.selectAlternateRacialFeature(trait);
            view = sheet.toView();
            const auto skill = std::ranges::find(view.skills.skills, type, &SkillView::type);
            assert(skill->modifiers.permanentTotal == 2);
            assert(skill->modifiers.conditionalTotals.size() == 1);
            assert(skill->modifiers.permanentTotal + skill->modifiers.conditionalTotals.front().value == 4);
            sheet.removeAlternateRacialFeature(trait);
        }
        sheet.setRace("human");
        for (const auto &options : sheet.toView().languages.options)
        {
            for (const auto *id : {"reptoid", "rougarou", "yaddithian"})
                assert(std::ranges::find(options.languageIds, id) != options.languageIds.end());
            assert(std::ranges::find(options.languageIds, "darkFolk") == options.languageIds.end());
            assert(std::ranges::find(options.languageIds, "drowSignLanguage") == options.languageIds.end());
        }
    }

    void checkFavored(const RaceDefinition &race, const CharacterSheetSaveData &baseline, const std::filesystem::path &path)
    {
        for (const auto *elements : {&race.qualities, &race.standardFeatures})
            for (const auto &element : *elements)
                for (const auto &effect : element.effects)
                {
                    const auto *bonus = std::get_if<FavoredClassBonusEffectDefinition>(&effect);
                    if (!bonus) continue;
                    assert(!bonus->classes.empty());
                    for (auto type : bonus->classes)
                    {
                        std::cerr << "  favored: " << bonus->id << '\n';
                        auto definition = test::classDefinition(type);
                        while (definition.progression.size() < static_cast<std::size_t>(bonus->selectionsPerBenefit))
                        {
                            auto next = definition.progression.back();
                            ++next.level;
                            definition.progression.push_back(next);
                        }
                        ClassDefinitionManager manager({definition});
                        auto data = baseline;
                        data.classes.classes = {{.type = type, .level = bonus->selectionsPerBenefit, .choices = {}}};
                        data.classes.favoredClasses.selectedClasses = {type};
                        persistence::saveToFile(data, path);
                        auto sheet = CharacterSheet::load(path, manager);
                        auto view = sheet.toView().classes.favoredClasses.classes.front();
                        const auto found = std::ranges::find_if(view.options, [&](const auto &option) {
                            return option.id.ends_with(std::to_string(bonus->id.size()) + ":" + bonus->id);
                        });
                        assert(found != view.options.end() && found->available);
                        const auto id = found->id;
                        for (int level = 1; level <= bonus->selectionsPerBenefit; ++level)
                        {
                            sheet.selectFavoredClassBonus(type, level, id);
                            view = sheet.toView().classes.favoredClasses.classes.front();
                            const auto current = std::ranges::find(view.options, id, &FavoredClassBonusOptionView::id);
                            assert(current->benefits == level / bonus->selectionsPerBenefit);
                        }
                        sheet.save(path);
                        auto restored = CharacterSheet::load(path, manager);
                        assert(persistence::toJson(restored.toSaveData()) == persistence::toJson(sheet.toSaveData()));
                        restored.clearRace();
                        assert(!restored.toView().classes.favoredClasses.classes.front().selections.front().active);
                    }
                }
    }
}

int main()
{
    using namespace golarion;
    const auto catalog = nlohmann::json::parse(embedded::RaceDefinitionCatalogJson);
    const auto path = std::filesystem::temp_directory_path() / "golarion-race-catalog-test.json";
    std::set<std::string> ids;
    for (const auto &entry : catalog)
    {
        const auto id = entry.at("id").get<std::string>();
        std::cerr << "race: " << id << '\n';
        assert(ids.insert(id).second);
        checkReminders(entry);
        const auto &race = RaceDefinitionManager::instance().get(id);
        CharacterSheet sheet;
        sheet.setRace(id);
        for (const auto *elements : {&race.qualities, &race.standardFeatures})
            for (const auto &element : *elements) exerciseChoices(sheet, element);
        const auto baseline = sheet.toSaveData();
        sheet.save(path);
        auto restored = CharacterSheet::load(path);
        assert(persistence::toJson(restored.toSaveData()) == persistence::toJson(baseline));
        for (const auto &alternate : race.alternateFeatures)
        {
            std::cerr << "  alternate: " << alternate.id << '\n';
            sheet.selectAlternateRacialFeature(alternate.id);
            exerciseChoices(sheet, alternate);
            auto view = sheet.toView().race;
            for (const auto &replaced : alternate.replaces)
                assert(std::ranges::find(view.activeElements, replaced, &RacialElementView::id) == view.activeElements.end());
            sheet.save(path);
            auto alternateRestored = CharacterSheet::load(path);
            assert(persistence::toJson(alternateRestored.toSaveData()) == persistence::toJson(sheet.toSaveData()));
            sheet.removeAlternateRacialFeature(alternate.id);
        }
        sheet.clearRace();
        assert(sheet.toView().race.activeElements.empty());
        assert(sheet.toView().languages.spoken.empty());
        assert(sheet.toView().specialDefenses.immunities.empty());
        checkFavored(race, baseline, path);
    }
    assert(ids.size() == 63);
    assert(ids.contains("human") && ids.contains("android") && ids.contains("skinwalker"));
    checkOtherRaceExamples();
    checkCoreRaceExamples();
    std::filesystem::remove(path);
}
