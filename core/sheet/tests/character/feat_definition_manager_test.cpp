#include "golarion/character/feat_definition_json.hpp"
#include "golarion/character/feat_definition_manager.hpp"

#include <nlohmann/json.hpp>

#include <cassert>
#include <stdexcept>
#include <variant>

namespace
{
    using Json = nlohmann::json;

    template<typename Function>
    void expectInvalid(Function function)
    {
        bool rejected = false;
        try
        {
            function();
        }
        catch (const std::invalid_argument &)
        {
            rejected = true;
        }
        assert(rejected);
    }

    Json fixture()
    {
        return Json::parse(R"({
            "id": " test ",
            "name": "Talento di prova",
            "description": "Descrizione di prova.",
            "types": ["combat", "teamwork"],
            "requirements": [
                {"description": "Requisito numerico", "expression": "@bab >= 1"},
                {"description": "Verifica al tavolo"}
            ],
            "effects": [{"id": "note", "type": "reminder", "message": "Promemoria generale."}],
            "choices": [{
                "id": "choice",
                "prompt": "Scegli un'opzione",
                "selectionCount": 1,
                "options": [{
                    "id": "option",
                    "name": "Opzione",
                    "requirements": [{"description": "Verifica specifica"}],
                    "effects": [{"id": "note", "type": "reminder", "message": "Promemoria specifico."}]
                }]
            }]
        })");
    }
}

int main()
{
    using namespace golarion;
    auto &manager = FeatDefinitionManager::instance();
    const auto &feat = manager.get(" improvedInitiative ");
    assert(&feat == &manager.get("improvedInitiative"));
    assert(feat.name == "Iniziativa Migliorata");
    assert(feat.requirements.empty() && feat.choices.empty());
    const auto &bonus = std::get<ModifierEffectDefinition>(feat.effects.at(0));
    assert(bonus.resource == "initiative" && bonus.expression == "4");
    assert(bonus.type == ModifierType::Bonus && bonus.bonusType == BonusType::Generic);
    expectInvalid([&] { manager.get("missing"); });
    expectInvalid([&] { manager.get(" "); });
    assert(&feat == &manager.get("improvedInitiative"));

    const auto parsed = featDefinitionFromJson(fixture());
    assert(parsed.repeatability == FeatRepeatability::Once);
    auto repeatable = fixture();
    repeatable["repeatability"] = "differentChoices";
    repeatable["requirements"] = Json::array({{{"description", "Talento precedente"}, {"feat", {{"id", "previous"}, {"matchingChoices", {{"weapon", "choice"}}}}}}});
    const auto dependent = featDefinitionFromJson(repeatable);
    assert(dependent.repeatability == FeatRepeatability::DifferentChoices);
    assert(dependent.requirements[0].feat->id == "previous");
    assert(dependent.requirements[0].feat->matchingChoices.at("weapon") == "choice");
    repeatable["repeatability"] = "unlimited";
    assert(featDefinitionFromJson(repeatable).repeatability == FeatRepeatability::Unlimited);
    assert(parsed.id == "test");
    assert(parsed.types.size() == 2);
    assert(parsed.types[0] == FeatType::Combat);
    assert(parsed.types[1] == FeatType::Teamwork);
    assert(parsed.requirements[0].expression == "@bab >= 1");
    assert(!parsed.requirements[1].expression.has_value());
    const auto &option = parsed.choices.at(0).options.at(0);
    assert(!option.requirements[0].expression.has_value());
    assert(std::get<ReminderEffectDefinition>(option.effects[0]).message == "Promemoria specifico.");

    auto minimal = fixture();
    minimal.erase("requirements");
    minimal.erase("effects");
    minimal.erase("choices");
    const auto empty = featDefinitionFromJson(minimal);
    assert(empty.requirements.empty() && empty.effects.empty() && empty.choices.empty());

    const auto rejectChange = [](auto change)
    {
        auto invalid = fixture();
        change(invalid);
        expectInvalid([&] { featDefinitionFromJson(invalid); });
    };
    rejectChange([](Json &j) { j["types"].push_back(" combat "); });
    rejectChange([](Json &j) { j["types"] = {"custom"}; });
    rejectChange([](Json &j) { j["choices"].push_back(j["choices"][0]); });
    rejectChange([](Json &j) { j["choices"][0]["options"].push_back(j["choices"][0]["options"][0]); });
    rejectChange([](Json &j) { j["effects"].push_back(j["effects"][0]); });
    rejectChange([](Json &j) {
        auto &effects = j["choices"][0]["options"][0]["effects"];
        effects.push_back(effects[0]);
    });
    for (const Json &count : {Json(0), Json(-1), Json(2), Json(1.5), Json(true), Json("1"), Json(18446744073709551615ULL)})
    {
        rejectChange([&](Json &j) { j["choices"][0]["selectionCount"] = count; });
    }
    for (const auto *field : {"types", "requirements", "effects", "choices"})
    {
        rejectChange([&](Json &j) { j[field] = Json::object(); });
    }
    rejectChange([](Json &j) { j["choices"][0]["options"] = Json::object(); });
    rejectChange([](Json &j) { j["requirements"][0]["expression"] = " "; });
    rejectChange([](Json &j) { j["requirements"][0]["expression"] = 42; });
    rejectChange([](Json &j) { j["choices"][0]["options"][0]["requirements"] = nullptr; });
    rejectChange([](Json &j) { j["effects"][0]["type"] = "unknown"; });
    rejectChange([](Json &j) { j["id"] = " "; });
    rejectChange([](Json &j) { j["repeatability"] = "unknown"; });
    rejectChange([](Json &j) { j["repeatability"] = "differentChoices"; j.erase("choices"); });
    rejectChange([](Json &j) { j["requirements"][0]["feat"] = {{"id", "test"}}; });
    rejectChange([](Json &j) { j.erase("name"); });
    expectInvalid([] { featDefinitionFromJson(Json::array()); });
}
