#include "golarion/character/character_sheet.hpp"
#include "golarion/persistence/json.hpp"
#include <nlohmann/json.hpp>
#include <cassert>
#include <filesystem>

namespace
{
    using namespace golarion;
    template<class F> void rejects(F action)
    {
        bool rejected = false;
        try { action(); } catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }
    EffectDefinition bonus(std::string id, std::string expression, BonusType type = BonusType::Enhancement)
    {
        return ModifierEffectDefinition{std::move(id), "str", "Forza personalizzata", ModifierType::Bonus, type, std::move(expression)};
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
        enhancements.addGroup({"a", "Primo", {bonus("z", "@input"), bonus("a", "4")}});
        assert(resources.modifierTotal("str") == 4);
        rejects([&] { enhancements.add("a", bonus("new", "1")); });
        rejects([&] { enhancements.remove("a", "z"); });
        enhancements.setGroupActive("a", false);
        enhancements.setGroupActive("a", false);
        assert(resources.modifierTotal("str") == 0);
        auto snapshot = enhancements.toView();
        assert(!snapshot.groups[0].active && !snapshot.groups[0].enhancements[0].resolvedValue);
        assert(effectId(snapshot.groups[0].enhancements[0].definition) == "z");
        enhancements.add("a", ContributionEffectDefinition{"hp", "hp.max", "@input * 2"});
        rejects([&] { enhancements.add("a", bonus(" z ", "1")); });
        input = 6;
        enhancements.setGroupActive("a", true);
        enhancements.setGroupActive("a", true);
        assert(resources.modifierTotal("str") == 6 && resources.contributionTotal("hp.max") == 12);
        assert(resources.modifierSetView("str").modifiers.size() == 2);
        assert(!snapshot.groups[0].active);
        enhancements.setGroupActive("a", false);
        enhancements.remove("a", "hp");
        enhancements.setGroupActive("a", true);
        assert(resources.contributionTotal("hp.max") == 0);
        for (const auto expression : {"-1", "@missing", "1 +", "@strength"})
        {
            rejects([&] { enhancements.addGroup({"bad", "Errore", {bonus("valid", "3", BonusType::Generic), bonus("invalid", expression)}}); });
            assert(enhancements.toData().groups.size() == 1);
            assert(resources.modifierTotal("str") == 6);
        }
        enhancements.addGroup({"delayed", "Inattivo", {bonus("invalid", "@missing")}, false});
        rejects([&] { enhancements.setGroupActive("delayed", true); });
        assert(!enhancements.toData().groups[1].active);
        enhancements.removeGroup("delayed");
        enhancements.removeGroup("a");
        enhancements.addGroup({"a", "Prima fonte", {bonus("x", "2", BonusType::Circumstance), bonus("y", "3", BonusType::Circumstance)}});
        enhancements.addGroup({"b", "Seconda fonte", {bonus("x", "4", BonusType::Circumstance)}});
        assert(resources.modifierTotal("str") == 7);
        enhancements.removeGroup("a");
        assert(resources.modifierTotal("str") == 4);
    }
    assert(resources.modifierTotal("str") == 0);
    // Dependent effects are applied in declaration order, not ID order.
    Spellcastings castings(resources);
    const auto castingId = enhancementEffectId("castingGroup", "zProvider");
    const auto slotsId = enhancementEffectId("castingGroup", "aSlots");
    {
        Enhancements enhancements(resources);
        const std::vector<EffectDefinition> effects{
            SpellcastingGrantEffectDefinition{"zProvider", SpellcastingMode::Prepared, CharacterClassType::Wizard},
            SpellSlotGrantEffectDefinition{"aSlots", castingId, SpellSlotPoolType::Casting, {{3, 1}}}
        };
        enhancements.addGroup({"castingGroup", "Incantesimi", effects});
        assert(castings.get(castingId).slotPools().contains(slotsId));
        enhancements.setGroupActive("castingGroup", false); // Removes slots before their provider.
        assert(castings.toView().castings.empty());
        enhancements.setGroupActive("castingGroup", true);
        auto broken = effects;
        broken.push_back(ContributionEffectDefinition{"bad", "missing", "1"});
        enhancements.removeGroup("castingGroup");
        rejects([&] { enhancements.addGroup({"castingGroup", "Errore", broken}); });
        assert(castings.toView().castings.empty());
        enhancements.addGroup({"castingGroup", "Nuovo", effects});
    }
    assert(castings.toView().castings.empty());

    CharacterSheet sheet;
    const int initial = sheet.toView().abilities[0].totalValue;
    sheet.addEnhancementGroup({"custom", "Potenziamenti", {
        bonus("strength", "4"), ContributionEffectDefinition{"hp", "hp.max", "10"},
        LanguageGrantEffectDefinition{"language", "draconic"}, FeatSlotGrantEffectDefinition{"feat"}
    }, true, "Descrizione", EnhancementOrigin{EnhancementOriginType::Spell, "spell.custom", "caster"}});
    sheet.addEnhancementGroup({"disabled", "Inattivo", {ContributionEffectDefinition{"unavailable", "missing", "@missing"}}, false});
    assert(sheet.toView().abilities[0].totalValue == initial + 4);
    assert(!sheet.toView().enhancements.groups[0].enhancements[2].resolvedValue);
    const auto featSlot = enhancementEffectId("custom", "feat");
    sheet.selectFeat(featSlot, "improvedInitiative");
    const auto json = persistence::toJson(sheet.toSaveData());
    assert(persistence::toJson(persistence::fromJson(json)) == json);
    const auto path = std::filesystem::temp_directory_path() / "golarion-enhancements-test.json";
    sheet.save(path);
    {
        auto restored = CharacterSheet::load(path);
        assert(persistence::toJson(restored.toSaveData()) == json);
        assert(restored.toView().enhancements.groups[0].origin->sourceSheetId == "caster");
        restored.setEnhancementGroupActive("custom", false);
        assert(!restored.toView().feats.feats.front().active);
        assert(restored.toView().abilities[0].totalValue == initial);
        restored.setEnhancementGroupActive("custom", true);
        assert(restored.toView().feats.feats.front().active);
        restored.setEnhancementGroupActive("disabled", false);
        restored.removeEnhancement("disabled", "unavailable");
        restored.addEnhancement("disabled", bonus("new", "2"));
        rejects([&] { restored.addEnhancement("custom", bonus("bad", "2")); });
    }
    for (int error = 0; error < 4; ++error)
    {
        auto malformed = nlohmann::json::parse(json);
        auto &group = malformed["enhancements"][0];
        if (error == 0) group["origin"]["type"] = "unknown";
        if (error == 1) group["effects"][0]["active"] = true;
        if (error == 2) group["active"] = 1;
        if (error == 3) { group["enhancements"] = group["effects"]; group.erase("effects"); }
        rejects([&] { persistence::fromJson(malformed.dump()); });
    }
    auto obsolete = sheet.toSaveData();
    obsolete.formatVersion = 41;
    persistence::saveToFile(obsolete, path);
    rejects([&] { CharacterSheet::load(path); });
    std::filesystem::remove(path);
}
