#include "../fixtures/advance_sheet.hpp"
#include "golarion/character/character_sheet.hpp"
#include "golarion/effect/effect_json.hpp"
#include "golarion/character/spell_definition_json.hpp"
#include "golarion/equipment/item_definition_json.hpp"
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
    using Json = nlohmann::json;
    auto custom = SpellDefinitionManager::instance().get("fireball");
    custom.id = "custom.flame";
    custom.name = "Fiamma personale";
    custom.maxTargetsExpression = "2";
    custom.spellLevelByDomain = {{DomainType::Healing, 1}, {SubdomainType::Healing, 2}};
    custom.spellLevelByBloodline = {{BloodlineType::Arcane, 3}};
    custom.effects.push_back(effectDefinitionFromJson(Json::parse(R"({"id":"note","type":"reminder","message":"Effetto da applicare manualmente"})")));
    auto encoded = spellDefinitionToJson(custom);
    assert(spellDefinitionToJson(spellDefinitionFromJson(encoded)) == encoded);
    assert(encoded["spellLevelByDomain"]["domain"]["HEALING"] == 1);
    assert(encoded["spellLevelByDomain"]["subdomain"]["HEALING"] == 2);
    assert(encoded["effects"].size() == 1);
    assert(spellDefinitionToJson(spellDefinitionFromJson(spellDefinitionToJson(SpellDefinitionManager::instance().get("fireball")))) == spellDefinitionToJson(SpellDefinitionManager::instance().get("fireball")));
    auto detailed = custom;
    detailed.school = SpellSchool::Conjuration;
    detailed.subschool = SpellSubschool::Creation;
    detailed.components.insert(SpellComponentType::Focus);
    detailed.focusComponentDescription = "Cristallo";
    detailed.materialComponentCostInGoldPieces = 10;
    detailed.target = "Una creatura";
    detailed.effect = "Una fiamma";
    detailed.durationType = SpellDurationType::Concentration;
    detailed.dismissible = true;
    const auto detailedJson = spellDefinitionToJson(detailed);
    assert(spellDefinitionToJson(spellDefinitionFromJson(detailedJson)) == detailedJson);
    assert(detailedJson["subschool"] == "CREATION");
    assert(detailedJson["focusComponentDescription"] == "Cristallo");
    assert(detailedJson["materialComponentCostInGoldPieces"] == 10);
    SpellCatalog local;
    local.add(custom);
    assert(local.get("custom.flame").name == custom.name);
    assert(&local.get("fireball") == &SpellDefinitionManager::instance().get("fireball"));
    rejects([&] { SpellCatalog{}.get("custom.flame"); });
    rejects([&] { SpellDefinitionManager::instance().get("custom.flame"); });
    rejects([&] { local.add(custom); });
    auto invalid = custom; invalid.id = " fireball ";
    rejects([&] { local.add(invalid); });
    invalid.id = "invalid"; invalid.school = static_cast<SpellSchool>(999);
    rejects([&] { local.add(invalid); });
    invalid = custom; invalid.id = "invalid"; invalid.spellLevelByClass[CharacterClassType::Wizard] = -1;
    rejects([&] { local.add(invalid); });
    assert(local.customDefinitions().size() == 1);
    ClassDefinition definition{.type = CharacterClassType::Wizard, .name = "Test", .description = "Test", .hitDie = ClassHitDie::D6,
        .skillRanksPerLevel = 2, .progression = {{1, 0, {{SavingThrowType::Fortitude, 0}, {SavingThrowType::Reflex, 0}, {SavingThrowType::Will, 2}}}}, .features = {{"casting", "Incantesimi", "Test", {{.id = "start", .level = 1}}}}};
    auto &effects = definition.features[0].progression[0].effects;
    for (const auto &[id, mode] : {std::pair{"p", SpellcastingMode::Prepared}, {"s", SpellcastingMode::Spontaneous}, {"f", SpellcastingMode::Flexible}})
    {
        effects.push_back(SpellcastingGrantEffectDefinition{id, mode, CharacterClassType::Wizard});
        effects.push_back(SpellRepertoireQuotaEffectDefinition{std::string(id) + "known", key(id), {{3, 1}}});
        effects.push_back(SpellSlotGrantEffectDefinition{std::string(id) + "slots", key(id), SpellSlotPoolType::Casting, {{3, 2}}});
    }
    effects.push_back(SpellSlotGrantEffectDefinition{"preparations", key("f"), SpellSlotPoolType::Preparation, {{3, 1}}});
    effects.push_back(effectDefinitionFromJson(Json::parse(R"({"id":"gift","type":"spellGrant","spellId":"custom.flame","usage":{"type":"limited","capacity":2}})")));
    effects.push_back(effectDefinitionFromJson(Json{
        {"id", "conversion"}, {"type", "spontaneousConversion"}, {"castingId", key("p")},
        {"groupIds", Json::array({key("pslots")})},
        {"replacements", Json::array({Json{{"spellId", "custom.unused"}, {"level", 3}}})}
    }));
    ClassDefinitionManager classes({definition});
    CharacterSheet sheet(classes);
    sheet.addCustomSpellDefinition(custom);
    auto unused = custom; unused.id = "custom.unused";
    sheet.addCustomSpellDefinition(unused);
    assert(sheet.toView().customSpellDefinitions.size() == 2);
    assert(sheet.toView().spellcastings.castings.empty());
    assert(sheet.toView().grantedSpells.empty());
    test::advanceSheetFixture(sheet, CharacterClassType::Wizard, 6);
    sheet.consumeGrantedSpell(key("gift"));
    sheet.addCustomItemDefinition(itemDefinitionFromJson(Json::parse(R"({
        "id":"custom.ring","name":"Anello","weightGrams":10,"volumeMilliliters":1,"tags":["magic"],"slot":"ring",
        "usagePools":[{"id":"charges","capacity":3}],
        "effects":[{"id":"spell","activation":"equipped","type":"spellGrant","spellId":"custom.flame","usage":{"type":"sharedPool","poolId":"charges","cost":1}}]
    })")));
    sheet.addItem({"ring", "custom.ring"});
    sheet.equipItem("ring");
    for (const auto &grant : sheet.toView().grantedSpells)
        if (grant.usage == GrantedSpellUsage::SharedPool) sheet.consumeGrantedSpell(grant.id);
    for (const auto *id : {"p", "s", "f"}) sheet.selectRepertoireSpell(key(id), {key(std::string(id) + "known"), 3, "custom.flame"});
    const SpellSlotId p{key("pslots"), 3, 0}, s{key("sslots"), 3, 0}, f{key("fslots"), 3, 0}, preparation{key("preparations"), 3, 0};
    sheet.prepareSpell(key("p"), {p}, {"custom.flame"});
    sheet.prepareFlexibleSpell(key("f"), preparation, {"custom.flame"});
    sheet.consumePreparedSpell(key("p"), p);
    sheet.consumeSpontaneousSpell(key("s"), s, {"custom.flame"});
    sheet.consumeFlexibleSpell(key("f"), f, {preparation});
    const auto before = persistence::toJson(sheet.toSaveData());
    const auto path = std::filesystem::temp_directory_path() / "golarion-custom-spells-test.json";
    sheet.save(path);
    auto restored = CharacterSheet::load(path, classes);
    assert(persistence::toJson(restored.toSaveData()) == before);
    assert(restored.toView().customSpellDefinitions.size() == 2);
    for (const auto &grant : restored.toView().grantedSpells)
        assert(grant.remaining == (grant.usage == GrantedSpellUsage::Limited ? 1 : 2));
    rejects([&] { restored.consumeSpontaneousSpell(key("s"), s, {"custom.flame"}); });
    restored.clearSpellSlot(key("p"), p);
    restored.prepareSpell(key("p"), {p}, {"custom.flame"});
    restored.consumePreparedSpell(key("p"), p, {"custom.unused"});
    restored.clearSpellSlot(key("s"), s);
    restored.consumeSpontaneousSpell(key("s"), s, {"custom.flame"});
    restored.clearSpellSlot(key("f"), f);
    restored.consumeFlexibleSpell(key("f"), f, {preparation});
    CharacterSheet isolated;
    isolated.addCustomSpellDefinition(custom);
    assert(isolated.toView().customSpellDefinitions.size() == 1);
    auto malformed = Json::parse(before);
    malformed["customSpellDefinitions"][0]["spellLevelByClass"]["WIZARD"] = 1.5;
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = Json::parse(before); malformed["customSpellDefinitions"] = Json::object();
    rejects([&] { persistence::fromJson(malformed.dump()); });
    malformed = Json::parse(before); malformed["customSpellDefinitions"].push_back(malformed["customSpellDefinitions"][0]);
    persistence::saveToFile(persistence::fromJson(malformed.dump()), path);
    rejects([&] { CharacterSheet::load(path, classes); });
    malformed = Json::parse(before); malformed["customSpellDefinitions"] = Json::array();
    persistence::saveToFile(persistence::fromJson(malformed.dump()), path);
    rejects([&] { CharacterSheet::load(path, classes); });
    auto legacy = Json::parse(persistence::toJson(CharacterSheet{}.toSaveData()));
    legacy["formatVersion"] = 37; legacy.erase("customSpellDefinitions");
    persistence::saveToFile(persistence::fromJson(legacy.dump()), path);
    assert(CharacterSheet::load(path).toSaveData().customSpellDefinitions.empty());
    std::filesystem::remove(path);
}
