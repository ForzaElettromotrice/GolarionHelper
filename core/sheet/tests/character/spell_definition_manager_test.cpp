#include "golarion/character/spell_definition_json.hpp"
#include "golarion/character/spell_definition_manager.hpp"

#include <nlohmann/json.hpp>

#include <cassert>
#include <stdexcept>

namespace
{
    using Json = nlohmann::json;

    Json fixture()
    {
        return Json::parse(R"({
            "id": " test ", "name": " Prova ", "school": "CONJURATION", "subschool": "CREATION",
            "descriptors": ["WATER", "COLD"],
            "spellLevelByClass": {"CLERIC": 0, "DRUID": 1},
            "spellLevelByDomain": {"domain": {"HEALING": 1}, "subdomain": {"HEALING": 2}},
            "spellLevelByBloodline": {"AQUATIC": 1},
            "castingTimeCategory": "ROUND", "castingTimeDescription": "Un round",
            "components": ["VERBAL", "MATERIAL_OR_DIVINE_FOCUS", "FOCUS_OR_DIVINE_FOCUS"],
            "materialComponentDescription": " Materiale ", "materialComponentCostInGoldPieces": 100,
            "focusComponentDescription": "Focus", "rangeType": "OTHER", "rangeDescription": "Vedi testo",
            "target": "Creatura", "effect": "Manifestazione", "area": "Area di prova",
            "durationType": "CONCENTRATION", "durationDescription": "Concentrazione", "dismissible": true,
            "savingThrows": [
                {"savingThrowType": "WILL", "effect": "NEGATES", "description": "Volontà nega."},
                {"savingThrowType": "FORTITUDE", "effect": "PARTIAL", "description": "Tempra parziale."}
            ],
            "spellResistanceType": "SEE_TEXT", "spellResistanceDescription": "Vedi testo",
            "description": "Descrizione sintetica per i test."
        })");
    }

    template<typename Function> void rejects(Function function)
    {
        bool rejected = false;
        try { function(); }
        catch (const std::invalid_argument &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    auto &manager = SpellDefinitionManager::instance();
    const auto &spell = manager.get(" fireball ");
    assert(&spell == &manager.get("fireball"));
    assert(spell.name == "Palla di Fuoco");
    assert(spell.school == SpellSchool::Evocation);
    assert(spell.spellLevelByClass.at(CharacterClassType::Wizard) == 3);
    assert(spell.spellLevelByDomain.at(DomainType::Fire) == 3);
    assert(spell.savingThrows[0].type == SavingThrowType::Reflex);
    rejects([&] { manager.get("missing"); });
    rejects([&] { manager.get(" "); });
    assert(&spell == &manager.get("fireball"));

    const auto parsed = spellDefinitionFromJson(fixture());
    assert(parsed.effects.empty() && spell.effects.empty());
    auto effectJson = fixture();
    effectJson["effects"] = Json::array({
        {{"id", " note "}, {"type", "reminder"}, {"message", "Effetto di prova."}},
        {{"id", "language"}, {"type", "languageGrant"}, {"languageId", "draconic"}}
    });
    const auto withEffects = spellDefinitionFromJson(effectJson);
    assert(withEffects.effects.size() == 2);
    assert(std::get<ReminderEffectDefinition>(withEffects.effects[0]).id == "note");
    assert(std::get<LanguageGrantEffectDefinition>(withEffects.effects[1]).languageId == "draconic");
    assert(withEffects.effect == "Manifestazione"); // The existing descriptive field remains independent.
    for (int error = 0; error < 5; ++error)
    {
        auto bad = effectJson;
        if (error == 0) bad["effects"] = Json::object();
        if (error == 1) bad["effects"][1]["id"] = " note ";
        if (error == 2) bad["effects"][0]["type"] = "unknown";
        if (error == 3) bad["effects"][0]["id"] = " ";
        if (error == 4) bad["effects"][0].erase("message");
        rejects([&] { spellDefinitionFromJson(bad); });
    }
    auto duplicateEffects = withEffects;
    duplicateEffects.effects.push_back(ReminderEffectDefinition{" note ", "Duplicato"});
    rejects([&] { validateSpellDefinition(duplicateEffects); });
    effectJson["effects"] = nullptr;
    assert(spellDefinitionFromJson(effectJson).effects.empty());
    effectJson["effects"] = Json::array();
    assert(spellDefinitionFromJson(effectJson).effects.empty());
    auto adeptJson = fixture();
    adeptJson["spellLevelByClass"] = {{"ADEPT", 2}};
    assert(spellDefinitionFromJson(adeptJson).spellLevelByClass.at(CharacterClassType::Adept) == 2);
    assert(parsed.id == "test" && parsed.name == "Prova");
    assert(parsed.school == SpellSchool::Conjuration && parsed.subschool == SpellSubschool::Creation);
    assert(parsed.descriptors.contains(SpellDescriptor::Water));
    assert(parsed.spellLevelByClass.at(CharacterClassType::Cleric) == 0);
    assert(parsed.spellLevelByDomain.at(DomainType::Healing) == 1);
    assert(parsed.spellLevelByDomain.at(SubdomainType::Healing) == 2);
    assert(parsed.spellLevelByBloodline.at(BloodlineType::Aquatic) == 1);
    assert(parsed.components.contains(SpellComponentType::MaterialOrDivineFocus));
    assert(parsed.materialComponentDescription == "Materiale");
    assert(parsed.materialComponentCostInGoldPieces == 100);
    assert(parsed.target && parsed.effect && parsed.area);
    assert(parsed.dismissible && parsed.savingThrows.size() == 2);
    assert(parsed.spellResistanceType == SpellResistanceType::SeeText);

    auto minimal = fixture();
    for (const auto *field : {"subschool", "descriptors", "spellLevelByClass", "spellLevelByDomain", "spellLevelByBloodline",
         "materialComponentDescription", "materialComponentCostInGoldPieces", "focusComponentDescription", "target", "effect", "area", "savingThrows", "dismissible"})
        minimal.erase(field);
    minimal["durationType"] = "INSTANTANEOUS";
    const auto empty = spellDefinitionFromJson(minimal);
    assert(!empty.subschool && !empty.target && !empty.materialComponentCostInGoldPieces);
    assert(empty.descriptors.empty() && empty.spellLevelByDomain.empty() && empty.savingThrows.empty());
    assert(!empty.dismissible);
    minimal["target"] = "   ";
    minimal["subschool"] = nullptr;
    minimal["savingThrows"] = nullptr;
    assert(!spellDefinitionFromJson(minimal).target);

    const auto rejectChange = [](auto change)
    {
        auto invalid = fixture();
        change(invalid);
        rejects([&] { spellDefinitionFromJson(invalid); });
    };
    for (const auto *field : {"school", "subschool", "castingTimeCategory", "rangeType", "durationType", "spellResistanceType"})
        rejectChange([&](auto &j) { j[field] = "UNKNOWN"; });
    rejectChange([](auto &j) { j["school"] = "EVOCATION"; });
    rejectChange([](auto &j) { j["descriptors"] = {"WATER", " WATER "}; });
    rejectChange([](auto &j) { j["descriptors"] = {"UNKNOWN"}; });
    rejectChange([](auto &j) { j["components"] = {"VERBAL", "VERBAL"}; });
    rejectChange([](auto &j) { j["components"] = {"UNKNOWN"}; });
    rejectChange([](auto &j) { j["components"].push_back("MATERIAL"); });
    rejectChange([](auto &j) { j["components"] = Json::array(); });
    rejectChange([](auto &j) { j["descriptors"] = Json::object(); });
    rejectChange([](auto &j) { j["spellLevelByClass"] = Json::array(); });
    rejectChange([](auto &j) { j["spellLevelByClass"]["UNKNOWN"] = 1; });
    rejectChange([](auto &j) { j["spellLevelByClass"][" CLERIC "] = 2; });
    rejectChange([](auto &j) { j["spellLevelByDomain"] = {{"HEALING", 1}}; });
    rejectChange([](auto &j) { j["spellLevelByDomain"]["subdomain"] = {{"UNKNOWN", 1}}; });
    rejectChange([](auto &j) { j["spellLevelByBloodline"] = {{"UNKNOWN", 1}}; });
    rejectChange([](auto &j) { j["savingThrows"] = Json::object(); });
    rejectChange([](auto &j) { j["savingThrows"][0]["savingThrowType"] = "UNKNOWN"; });
    rejectChange([](auto &j) { j["savingThrows"][0]["effect"] = "UNKNOWN"; });
    rejectChange([](auto &j) { j["dismissible"] = false; });
    rejectChange([](auto &j) { j["dismissible"] = "true"; });
    rejectChange([](auto &j) { j["name"] = " "; });
    rejectChange([](auto &j) { j.erase("id"); });
    for (const Json &value : {Json(-1), Json(1.5), Json(true), Json("3"), Json(2147483648ULL)})
    {
        rejectChange([&](auto &j) { j["spellLevelByClass"]["CLERIC"] = value; });
        rejectChange([&](auto &j) { j["materialComponentCostInGoldPieces"] = value; });
    }
    rejects([] { spellDefinitionFromJson(Json::array()); });
}
