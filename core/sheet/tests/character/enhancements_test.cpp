#include "golarion/character/character_sheet.hpp"
#include "golarion/persistence/json.hpp"

#include <nlohmann/json.hpp>
#include <cassert>
#include <filesystem>
#include <type_traits>

namespace
{
    using namespace golarion;

    static_assert(!std::is_same_v<EnhancementDefinition, EnhancementData>);
    static_assert(!std::is_same_v<EnhancementGroupDefinition, EnhancementGroupData>);
    static_assert(!std::is_convertible_v<EnhancementData, EnhancementDefinition>);

    template<class F> void rejects(F action)
    {
        bool rejected = false;
        try { action(); } catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }

    EnhancementDefinition bonus(std::string id, std::string expression, BonusType type = BonusType::Enhancement, bool active = true)
    {
        return {ModifierEffectDefinition{std::move(id), "str", "Forza personalizzata", ModifierType::Bonus, type, std::move(expression)}, active};
    }
}

int main()
{
    using namespace golarion;
    ResourceManager resources;
    resources.registerEnhanceableResource("str");
    resources.registerAccumulatedResource("hp.max");
    int input = 2;
    resources.registerTarget("input", [&] { return input; });
    resources.registerTarget("strength", [&] { return resources.modifierTotal("str"); });
    {
        Enhancements enhancements(resources);
        enhancements.addGroup({"a", "Primo gruppo", {bonus("first", "@input"), bonus("second", "4")}});
        assert(resources.modifierTotal("str") == 4);
        enhancements.setActive("a", "second", false);
        enhancements.setActive("a", "second", false);
        assert(resources.modifierTotal("str") == 2);
        auto view = enhancements.toView();
        assert(view.groups[0].enhancements.size() == 2);
        assert(!view.groups[0].enhancements[1].data.active);
        assert(!view.groups[0].enhancements[1].resolvedValue);
        input = 6;
        assert(resources.modifierTotal("str") == 6);
        enhancements.setActive("a", "second", true);
        enhancements.setActive("a", "second", true);
        assert(resources.modifierSetView("str").modifiers.size() == 2);
        enhancements.add("a", {ContributionEffectDefinition{"hp", "hp.max", "@input * 2"}, true});
        assert(resources.contributionTotal("hp.max") == 12);
        enhancements.setActive("a", "hp", false);
        assert(resources.contributionTotal("hp.max") == 0);
        enhancements.setActive("a", "hp", true);
        enhancements.remove("a", "hp");
        assert(resources.contributionTotal("hp.max") == 0);

        rejects([&] { enhancements.addGroup({" a ", "Duplicato", {}}); });
        rejects([&] { enhancements.add("a", bonus("first", "1")); });
        rejects([&] { enhancements.add("a", bonus("bad", "-1")); });
        rejects([&] { enhancements.add("a", bonus("bad", "@missing")); });
        rejects([&] { enhancements.add("a", bonus("bad", "1 +")); });
        rejects([&] { enhancements.add("a", bonus("bad", "@strength")); });
        rejects([&] { enhancements.add("a", {ContributionEffectDefinition{"bad", "str", "1"}, true}); });
        rejects([&] { enhancements.add("missing", bonus("new", "1")); });
        rejects([&] { enhancements.setActive("a", "missing", true); });
        rejects([&] { enhancements.removeGroup("missing"); });
        assert(resources.modifierTotal("str") == 6);

        enhancements.add("a", bonus("delayed", "@strength", BonusType::Generic, false));
        rejects([&] { enhancements.setActive("a", "delayed", true); });
        assert(!enhancements.toData().groups[0].enhancements[0].active);
        enhancements.remove("a", "delayed");
        rejects([&] { enhancements.addGroup({"broken", "Non valido", {bonus("valid", "3", BonusType::Generic), bonus("invalid", "1 / 0")}}); });
        assert(enhancements.toData().groups.size() == 1);
        assert(resources.modifierTotal("str") == 6);
        enhancements.removeGroup("a");
        assert(resources.modifierTotal("str") == 0);

        enhancements.addGroup({"a", "Prima sorgente", {bonus("x", "2", BonusType::Circumstance), bonus("y", "3", BonusType::Circumstance)}});
        enhancements.addGroup({"b", "Seconda sorgente", {bonus("x", "4", BonusType::Circumstance)}});
        assert(resources.modifierTotal("str") == 7);
        enhancements.add("b", {ContributionEffectDefinition{"hp", "hp.max", "-2"}, true});
    }
    assert(resources.modifierTotal("str") == 0);
    assert(resources.contributionTotal("hp.max") == 0);

    CharacterSheet sheet;
    const int initial = sheet.toView().abilities[0].totalValue;
    sheet.addEnhancementGroup({"custom", "Potenziamenti personalizzati", {bonus("strength", "4"), bonus("inactive", "8", BonusType::Generic, false)}});
    sheet.addEnhancement("custom", {ContributionEffectDefinition{"hp", "hp.max", "10"}, true});
    sheet.addEnhancement("custom", {ContributionEffectDefinition{"unavailable", "missing", "@missing"}, false});
    assert(sheet.toView().abilities[0].totalValue == initial + 4);
    assert(sheet.toView().hitPoints.max == 10);
    sheet.damage(3, DamageLethality::Lethal);
    auto data = sheet.toSaveData();
    const auto json = persistence::toJson(data);
    assert(persistence::toJson(persistence::fromJson(json)) == json);
    const auto path = std::filesystem::temp_directory_path() / "golarion-enhancements-test.json";
    sheet.save(path);
    {
        auto restored = CharacterSheet::load(path);
        assert(persistence::toJson(restored.toSaveData()) == json);
        assert(restored.toView().abilities[0].totalValue == initial + 4);
        assert(restored.toView().hitPoints.max == 10);
        rejects([&] { restored.setEnhancementActive("custom", "unavailable", true); });
        restored.setEnhancementActive("custom", "inactive", true);
        assert(restored.toView().abilities[0].totalValue == initial + 12);
        restored.removeEnhancement("custom", "strength");
        assert(restored.toView().abilities[0].totalValue == initial + 8);
        restored.removeEnhancementGroup("custom");
        assert(restored.toView().enhancements.groups.empty());
        assert(restored.toView().abilities[0].totalValue == initial);
    }
    auto malformed = nlohmann::json::parse(json);
    auto &entries = malformed["enhancements"][0]["enhancements"];
    entries[1]["bonusType"] = "unknown";
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = nlohmann::json::parse(json);
    malformed["enhancements"] = nlohmann::json::object();
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = nlohmann::json::parse(json);
    malformed["enhancements"][0]["enhancements"][0]["type"] = "spellGrant";
    rejects([&] { persistence::fromJson(malformed.dump()); });

    auto legacy = nlohmann::json::parse(persistence::toJson(CharacterSheet{}.toSaveData()));
    legacy["formatVersion"] = 35;
    legacy.erase("enhancements");
    auto legacyData = persistence::fromJson(legacy.dump());
    assert(legacyData.enhancements.groups.empty());
    persistence::saveToFile(legacyData, path);
    auto restoredLegacy = CharacterSheet::load(path);
    assert(restoredLegacy.toSaveData().enhancements.groups.empty());
    std::filesystem::remove(path);
}
