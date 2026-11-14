#include "golarion/character/class_definition_json.hpp"

#include <nlohmann/json.hpp>

#include <cassert>
#include <functional>
#include <limits>
#include <stdexcept>

namespace
{
    using Json = nlohmann::json;

    void rejects(const std::function<void()> &operation)
    {
        bool rejected = false;
        try { operation(); }
        catch (const std::invalid_argument &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    const Json fixture = Json::parse(R"json({
        "type": "wizard", "name": "Mago", "description": "Classe di prova.",
        "hitDie": 6, "skillRanksPerLevel": 2,
        "classSkills": ["skill.spellcraft", "skill.knowledge.arcana"],
        "progression": [
            {"level": 1, "baseAttackBonus": 0, "baseSavingThrows": {"fortitude": 0, "reflex": 0, "will": 2}},
            {"level": 2, "baseAttackBonus": 1, "baseSavingThrows": {"fortitude": 0, "reflex": 0, "will": 3}}
        ],
        "features": [{"id": "school", "name": "Scuola", "description": "Scegli una scuola.", "progression": [
            {"id": "school.initial", "level": 1,
             "effects": [{"id": "notice", "type": "reminder", "message": "Regola iniziale."}],
             "choices": [{"id": "school", "prompt": "Scegli una scuola", "selectionCount": 1, "blocksAdvancement": false,
                 "options": [{"id": "abjuration", "name": "Abiurazione", "description": "Difesa magica.", "progression": [
                     {"id": "school.abjuration", "level": 2, "effects": [
                         {"id": "notice", "type": "reminder", "message": "Regola della scuola."}
                     ]}
                 ]}]
             }]},
            {"id": "school.upgrade", "level": 2, "replaces": ["school.initial"]}
        ]}]
    })json");

    const auto definition = classDefinitionFromJson(fixture);
    assert(definition.type == CharacterClassType::Wizard);
    assert(definition.hitDie == ClassHitDie::D6);
    assert(definition.classSkills.contains(SkillType::KnowledgeArcana));
    assert(definition.progression[1].baseSavingThrows.at(SavingThrowType::Will) == 3);
    assert(!definition.features[0].progression[0].choices[0].blocksAdvancement);
    assert(definition.features[0].progression[0].choices[0].options[0].progression[0].level == 2);
    assert(definition.features[0].progression[1].replaces[0] == "school.initial");
    const auto encoded = classDefinitionToJson(definition);
    assert(classDefinitionToJson(classDefinitionFromJson(encoded)) == encoded);
    auto defaults = fixture;
    defaults["features"][0]["progression"][0]["choices"][0].erase("blocksAdvancement");
    assert(classDefinitionFromJson(defaults).features[0].progression[0].choices[0].blocksAdvancement);

    const auto rejectMutation = [&](const std::function<void(Json &)> &mutate)
    {
        auto input = fixture;
        mutate(input);
        rejects([&] { (void)classDefinitionFromJson(input); });
    };
    rejectMutation([](Json &j) { j["type"] = "unknown"; });
    rejectMutation([](Json &j) { j["hitDie"] = 4; });
    rejectMutation([](Json &j) { j["hitDie"] = 6.5; });
    rejectMutation([](Json &j) { j["skillRanksPerLevel"] = -1; });
    rejectMutation([](Json &j) { j["skillRanksPerLevel"] = std::numeric_limits<unsigned long long>::max(); });
    rejectMutation([](Json &j) { j["classSkills"] = {"skill.unknown"}; });
    rejectMutation([](Json &j) { j["classSkills"].push_back("skill.spellcraft"); });
    rejectMutation([](Json &j) { j["progression"][0]["level"] = 0; });
    rejectMutation([](Json &j) { j["progression"][0]["baseAttackBonus"] = -1; });
    rejectMutation([](Json &j) { j["progression"].push_back(j["progression"][0]); });
    rejectMutation([](Json &j) { j["progression"][0]["baseSavingThrows"].erase("will"); });
    rejectMutation([](Json &j) { j["progression"][0]["baseSavingThrows"]["unknown"] = 0; });
    rejectMutation([](Json &j) { j["progression"][0]["baseSavingThrows"]["will"] = -1; });
    rejectMutation([](Json &j) { j["features"].push_back(j["features"][0]); });
    rejectMutation([](Json &j) { j["features"][0]["id"] = " "; });
    rejectMutation([](Json &j) { j["features"][0]["progression"][1]["id"] = "school.initial"; });
    rejectMutation([](Json &j) { j["features"][0]["progression"][0]["effects"].push_back(j["features"][0]["progression"][0]["effects"][0]); });
    rejectMutation([](Json &j) { j["features"][0]["progression"][0]["effects"][0]["type"] = "unknown"; });
    rejectMutation([](Json &j) { j["features"][0]["progression"][0]["choices"].push_back(j["features"][0]["progression"][0]["choices"][0]); });
    rejectMutation([](Json &j) {
        auto &options = j["features"][0]["progression"][0]["choices"][0]["options"];
        options[0]["progression"] = Json::array();
        options.push_back(options[0]);
    });
    rejectMutation([](Json &j) { j["features"][0]["progression"][0]["choices"][0]["selectionCount"] = 0; });
    rejectMutation([](Json &j) { j["features"][0]["progression"][0]["choices"][0]["selectionCount"] = 2; });
    rejectMutation([](Json &j) { j["features"][0]["progression"][0]["choices"][0]["blocksAdvancement"] = 1; });
    rejectMutation([](Json &j) { j["features"][0]["progression"][1]["replaces"] = {"unknown"}; });
    rejectMutation([](Json &j) { j["features"][0]["progression"][1]["replaces"] = {"school.upgrade"}; });
    rejectMutation([](Json &j) { j["features"][0]["progression"][0]["replaces"] = {"school.upgrade"}; });
    rejectMutation([](Json &j) { j["features"][0]["progression"][0]["choices"][0]["options"][0]["progression"][0]["replaces"] = {"school.initial"}; });

    auto invalid = definition;
    invalid.hitDie = static_cast<ClassHitDie>(7);
    rejects([&] { (void)classDefinitionToJson(invalid); });
    invalid = definition;
    invalid.type = static_cast<CharacterClassType>(-1);
    rejects([&] { (void)classDefinitionToJson(invalid); });
    invalid = definition;
    invalid.progression[0].baseSavingThrows.erase(SavingThrowType::Will);
    rejects([&] { (void)classDefinitionToJson(invalid); });
}
