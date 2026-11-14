#include "golarion/character/character_sheet.hpp"
#include "golarion/character/feat_definition_catalog.hpp"
#include "golarion/character/feat_definition_json.hpp"
#include "golarion/persistence/json.hpp"
#include "../fixtures/advance_sheet.hpp"
#include "../fixtures/class_definition.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <iostream>
#include <set>
#include <string_view>
#include <utility>

namespace
{
    using namespace golarion;
    using Json = nlohmann::json;
    constexpr std::string_view Marker = "Ehi, appena hai tempo vedi di correggermi se necessario!";

    void checkMarkers(const Json &value)
    {
        if (value.is_object())
        {
            if (value.value("type", "") == "reminder")
                assert(value.at("message").get<std::string>().ends_with(Marker));
            for (const auto &[key, child] : value.items())
            {
                static_cast<void>(key);
                checkMarkers(child);
            }
        }
        else if (value.is_array())
            for (const auto &child : value) checkMarkers(child);
    }

    void checkRequirements(const std::vector<FeatRequirementDefinition> &requirements, const FeatDefinition &owner, const std::set<std::string> &ids)
    {
        for (const auto &requirement : requirements)
        {
            assert(!requirement.description.empty());
            if (requirement.feat)
            {
                assert(!requirement.expression);
                assert(ids.contains(requirement.feat->id));
                assert(requirement.feat->id != owner.id);
            }
            else if (!requirement.expression)
                assert(requirement.description.ends_with(Marker));
        }
    }

    void checkAcquisition(const FeatDefinition &definition, const FeatChoices &choices, const std::filesystem::path &path)
    {
        CharacterSheet sheet;
        const auto baseline = persistence::toJson(sheet.toSaveData());
        sheet.addExtraFeatSlot("catalog.extra");
        sheet.selectFeat("catalog.extra", definition.id, choices);
        const auto view = sheet.toView();
        assert(view.feats.feats.size() == 1 && view.feats.feats.front().active);
        const auto selected = persistence::toJson(sheet.toSaveData());
        sheet.save(path);
        auto restored = CharacterSheet::load(path);
        assert(persistence::toJson(restored.toSaveData()) == selected);
        assert(restored.toView().feats.feats.front().active);
        restored.removeExtraFeatSlot("catalog.extra");
        assert(persistence::toJson(restored.toSaveData()) == baseline);
        sheet.removeExtraFeatSlot("catalog.extra");
        assert(persistence::toJson(sheet.toSaveData()) == baseline);
    }

    void checkChoiceBranches(const FeatDefinition &definition, const std::filesystem::path &path)
    {
        FeatChoices defaults;
        for (const auto &choice : definition.choices)
        {
            assert(choice.selectionCount > 0 && choice.selectionCount <= choice.options.size());
            for (std::size_t index = 0; index < choice.selectionCount; ++index)
                defaults[choice.id].push_back(choice.options[index].id);
        }
        checkAcquisition(definition, defaults, path);
        // Exercise every option with valid cardinality, without exponentially
        // enumerating unrelated combinations of independent choices.
        for (const auto &choice : definition.choices)
            for (std::size_t option = choice.selectionCount; option < choice.options.size(); ++option)
            {
                auto selected = defaults;
                selected.at(choice.id).back() = choice.options[option].id;
                checkAcquisition(definition, selected, path);
            }
    }

    void checkPowerAttackRequirements()
    {
        ResourceManager resources;
        int strength = 12;
        int bab = 0;
        resources.registerTarget("str", [&] { return strength; });
        resources.registerTarget("bab", [&] { return bab; });
        Feats feats(resources);
        feats.setLevel(1);
        const auto &powerAttack = FeatDefinitionManager::instance().get("powerAttack");
        assert(!feats.qualifies("base.feat.level1", powerAttack));
        strength = 13;
        assert(!feats.qualifies("base.feat.level1", powerAttack));
        bab = 1;
        assert(feats.qualifies("base.feat.level1", powerAttack));
        ClassDefinitionManager classes({test::classDefinition(CharacterClassType::Fighter)});
        CharacterSheet sheet(classes);
        test::advanceSheetFixture(sheet, CharacterClassType::Fighter, 8);
        sheet.setAbilityBaseValue(AbilityType::Strength, 13);
        sheet.selectFeat("base.feat.level1", "powerAttack");
        assert(sheet.toView().feats.feats.front().active);
        sheet.setAbilityBaseValue(AbilityType::Strength, 12);
        assert(!sheet.toView().feats.feats.front().active);
        sheet.setAbilityBaseValue(AbilityType::Strength, 13);
        assert(sheet.toView().feats.feats.front().active);
    }

    void checkScalingExamples()
    {
        auto definition = test::levelOnlyClassDefinition();
        while (definition.progression.size() < 10)
        {
            auto level = definition.progression.back();
            ++level.level;
            definition.progression.push_back(level);
        }
        ClassDefinitionManager classes({std::move(definition)});
        for (const int level : {0, 5, 10})
        {
            CharacterSheet sheet(classes);
            for (int current = 0; current < level; ++current)
                test::advanceSheetFixture(sheet, CharacterClassType::Fighter, 8);
            const auto baseline = sheet.toView();
            sheet.addExtraFeatSlot("example.toughness");
            sheet.selectFeat("example.toughness", "toughness");
            assert(sheet.toView().hitPoints.max == baseline.hitPoints.max + std::max(3, level));
            sheet.removeExtraFeatSlot("example.toughness");
            assert(sheet.toView().hitPoints.max == baseline.hitPoints.max);
            sheet.addExtraFeatSlot("example.reflexes");
            sheet.selectFeat("example.reflexes", "lightningReflexes");
            const auto improved = sheet.toView();
            for (std::size_t index = 0; index < improved.savingThrows.savingThrows.size(); ++index)
            {
                const auto &save = improved.savingThrows.savingThrows[index];
                const int increase = save.type == SavingThrowType::Reflex ? 2 : 0;
                assert(save.abilityOptions.front().totalValue == baseline.savingThrows.savingThrows[index].abilityOptions.front().totalValue + increase);
            }
            sheet.removeExtraFeatSlot("example.reflexes");
        }
    }
}

int main()
{
    const auto catalog = Json::parse(golarion::embedded::FeatDefinitionCatalogJson);
    assert(catalog.is_array() && catalog.size() == 1648);
    std::set<std::string> ids;
    for (const auto &entry : catalog) assert(ids.insert(entry.at("id").get<std::string>()).second);
    const auto path = std::filesystem::temp_directory_path() / "golarion-feat-catalog-test.json";
    for (const auto &entry : catalog)
    {
        const auto definition = featDefinitionFromJson(entry);
        std::cerr << "Feat catalog: " << definition.id << '\n';
        assert(!definition.name.empty() && !definition.description.empty() && !definition.types.empty());
        const auto canonical = featDefinitionToJson(definition);
        assert(featDefinitionToJson(featDefinitionFromJson(canonical)) == canonical);
        assert(featDefinitionToJson(FeatDefinitionManager::instance().get(definition.id)) == canonical);
        checkMarkers(canonical);
        checkRequirements(definition.requirements, definition, ids);
        for (const auto &choice : definition.choices)
            for (const auto &option : choice.options)
                checkRequirements(option.requirements, definition, ids);
        checkChoiceBranches(definition, path);
    }
    std::filesystem::remove(path);
    checkPowerAttackRequirements();
    checkScalingExamples();
}
