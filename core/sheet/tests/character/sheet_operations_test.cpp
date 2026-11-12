#include "../fixtures/advance_sheet.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/effect/effect_json.hpp"
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
    std::string key(const std::string &effect) { return "class.effect.6:wizard5:start" + std::to_string(effect.size()) + ":" + effect; }
}

int main()
{
    using namespace golarion;
    ClassDefinition definition{.type = CharacterClassType::Wizard, .name = "Test", .description = "Test", .hitDie = ClassHitDie::D6,
        .skillRanksPerLevel = 2, .progression = {{1, 0, {{SavingThrowType::Fortitude, 0}, {SavingThrowType::Reflex, 0}, {SavingThrowType::Will, 2}}}}, .features = {{"casting", "Incantesimi", "Test", {{.id = "start", .level = 1}}}}};
    auto &effects = definition.features[0].progression[0].effects;
    for (const auto &[id, mode] : {std::pair{"p", SpellcastingMode::Prepared}, {"s", SpellcastingMode::Spontaneous}, {"f", SpellcastingMode::Flexible}})
    {
        effects.push_back(SpellcastingGrantEffectDefinition{id, mode, CharacterClassType::Wizard});
        effects.push_back(SpellRepertoireQuotaEffectDefinition{std::string(id) + "known", key(id), {{3, 1}}});
        effects.push_back(SpellSlotGrantEffectDefinition{std::string(id) + "slots", key(id), SpellSlotPoolType::Casting, {{3, 2}, {4, 2}}});
    }
    effects.push_back(SpellSlotGrantEffectDefinition{"preparations", key("f"), SpellSlotPoolType::Preparation, {{3, 1}, {4, 1}}});
    auto silent = nlohmann::json{{"id", "silent"}, {"type", "metamagicGrant"}, {"castingId", key("p")}, {"phase", "preparation"}, {"metamagic", "silent"}};
    effects.push_back(effectDefinitionFromJson(silent));
    ClassDefinitionManager catalog({definition});
    CharacterSheet sheet(catalog);
    test::advanceSheetFixture(sheet, CharacterClassType::Wizard, 6);
    sheet.addContainer({.id = "home", .name = "Casa", .allowsPossessionEffects = false});
    sheet.addItem({.id = "cloak", .itemDefinitionId = "cloakOfResistance1", .quantity = 1}, "home");
    sheet.moveItem("cloak");
    sheet.equipItem("cloak");
    assert(sheet.toView().inventory.items.front().equipped);
    rejects([&] { sheet.removeItem("cloak"); });
    sheet.unequipItem("cloak", "home");
    assert(!sheet.toView().inventory.items.front().equipped);
    sheet.equipItem("cloak");

    for (const auto *id : {"p", "s", "f"}) sheet.selectRepertoireSpell(key(id), {key(std::string(id) + "known"), 3, "fireball"});
    const SpellSlotId p{key("pslots"), 4, 0}, s{key("sslots"), 3, 0}, f{key("fslots"), 3, 0}, preparation{key("preparations"), 3, 0};
    rejects([&] { sheet.prepareSpell(key("s"), {s}, {"fireball"}); });
    rejects([&] { sheet.addSpellToRepertoire("missing", "fireball"); });
    const SpellPreparationDefinition preparationDefinition{.spellId = "fireball", .metamagic = {{MetamagicType::Silent, key("silent"), {}}}};
    sheet.prepareSpell(key("p"), {p}, preparationDefinition);
    assert(preparationDefinition.spellId == "fireball" && preparationDefinition.metamagic.size() == 1);
    sheet.prepareFlexibleSpell(key("f"), preparation, {"fireball"});
    auto result = sheet.consumePreparedSpell(key("p"), p);
    assert(result.requiredSlotLevel == 4);
    sheet.consumeSpontaneousSpell(key("s"), s, {"fireball"});
    sheet.consumeFlexibleSpell(key("f"), f, {preparation});
    rejects([&] { sheet.consumeSpontaneousSpell(key("s"), s, {"fireball"}); });
    rejects([&] { sheet.consumePreparedSpell(key("f"), f); });
    const auto before = persistence::toJson(sheet.toSaveData());
    const auto path = std::filesystem::temp_directory_path() / "golarion-sheet-operations-test.json";
    sheet.save(path);
    auto restored = CharacterSheet::load(path, catalog);
    std::filesystem::remove(path);
    assert(persistence::toJson(restored.toSaveData()) == before);
    assert(restored.toView().inventory.items.front().equipped);
    for (const auto &entry : restored.toView().spellcastings.castings) assert(entry.repertoire == std::vector<std::string>{"fireball"});
    rejects([&] { restored.consumeFlexibleSpell(key("f"), f, {preparation}); });
    restored.clearSpellSlot(key("p"), p);
    restored.clearSpellSlot(key("s"), s);
    restored.clearSpellSlot(key("f"), f);
    restored.consumeSpontaneousSpell(key("s"), s, {"fireball"});
    restored.consumeFlexibleSpell(key("f"), f, {preparation});
    restored.clearFlexiblePreparation(key("f"), preparation);
    restored.unequipItem("cloak");
    restored.removeItem("cloak");
    assert(restored.toView().inventory.items.empty());
}
