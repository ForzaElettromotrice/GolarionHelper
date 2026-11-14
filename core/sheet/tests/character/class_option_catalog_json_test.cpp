#include "golarion/character/class_option_catalog_json.hpp"
#include "golarion/character/class_definition_json.hpp"

#include <nlohmann/json.hpp>

#include <cassert>
#include <functional>
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
    const Json classes = Json::parse(R"json([{
        "type":"wizard","name":"Mago","description":"Classe di prova","hitDie":6,"skillRanksPerLevel":2,
        "classSkills":[],"progression":[],"features":[{"id":"school","name":"Scuola","description":"Regola di prova",
            "progression":[{"id":"school.first","level":1},{"id":"school.second","level":2}]}]
    }])json");
    const Json catalogs = Json::parse(R"json([{
        "optionSets":[
            {"id":"powers","options":[{"id":"power","name":"Potere","description":"Regola di prova", "progression":[
                {"id":"initial","level":1,"effects":[{"id":"notice","type":"reminder","message":"Nota"}]},
                {"id":"upgrade","level":2,"replaces":["initial"]}
            ]}]},
            {"id":"schools","options":[{"id":"school","name":"Scuola","description":"Regola di prova", "progression":[
                {"id":"initial","level":1,"choices":[{"id":"power","prompt":"Scegli","selectionCount":1,"optionSet":"powers"}]}
            ]}]}
        ],
        "bindings":[{"id":"wizard.schools","classType":"wizard","featureId":"school",
            "acquisitionIds":["school.first","school.second"],
            "choice":{"id":"school","prompt":"Scegli la scuola","selectionCount":1,"blocksAdvancement":true,"optionSet":"schools"}}]
    }])json");
    const auto result = classDefinitionsWithOptionsFromJson(classes, catalogs);
    assert(result.size() == 1);
    const auto &stages = result.front().features.front().progression;
    assert(stages.size() == 2 && stages[0].choices.size() == 1 && stages[1].choices.size() == 1);
    const auto &first = stages[0].choices[0].options[0].progression[0];
    const auto &second = stages[1].choices[0].options[0].progression[0];
    assert(first.id != second.id && first.id.starts_with("school.first.choice.school.option.school."));
    const auto &nested = first.choices[0].options[0].progression;
    assert(nested[0].id != first.id && nested[1].replaces == std::vector<std::string>{nested[0].id});
    assert(classDefinitionToJson(classDefinitionFromJson(classDefinitionToJson(result.front()))) == classDefinitionToJson(result.front()));
    assert(classes[0]["features"][0]["progression"][0].find("choices") == classes[0]["features"][0]["progression"][0].end());
    const auto unchanged = classDefinitionsWithOptionsFromJson(classes, Json::array());
    assert(unchanged[0].features[0].progression[0].choices.empty());
    auto rejectMutation = [&](const auto &mutate)
    {
        auto data = catalogs;
        mutate(data);
        rejects([&] { (void)classDefinitionsWithOptionsFromJson(classes, data); });
    };
    rejectMutation([](Json &j) { j[0]["optionSets"][1]["id"] = "powers"; });
    rejectMutation([](Json &j) { j[0]["optionSets"][0]["options"] = Json::array(); });
    rejectMutation([](Json &j) { j[0]["optionSets"][0]["options"][0]["minimumLevel"] = 3; });
    rejectMutation([](Json &j) { j[0]["optionSets"][0]["options"][0]["progression"][0]["level"] = 1.5; });
    rejectMutation([](Json &j) { j[0]["optionSets"][0]["options"][0]["progression"][0]["effects"][0]["type"] = "unknown"; });
    rejectMutation([](Json &j) { j[0]["optionSets"][0]["options"][0]["progression"][1]["replaces"] = {"school.first"}; });
    rejectMutation([](Json &j) { j[0]["optionSets"][0]["options"][0]["progression"][1]["replaces"] = {"upgrade"}; });
    rejectMutation([](Json &j) { j[0]["optionSets"][1]["options"][0]["progression"][0]["choices"][0]["optionSet"] = "schools"; });
    rejectMutation([](Json &j) { j[0]["bindings"][0]["choice"]["optionSet"] = "unknown"; });
    rejectMutation([](Json &j) { j[0]["bindings"][0]["choice"]["options"] = Json::array(); });
    rejectMutation([](Json &j) { j[0]["bindings"][0]["choice"]["selectionCount"] = 0; });
    rejectMutation([](Json &j) { j[0]["bindings"][0]["choice"]["blocksAdvancement"] = "true"; });
    rejectMutation([](Json &j) { j[0]["bindings"][0]["classType"] = "unknown"; });
    rejectMutation([](Json &j) { j[0]["bindings"][0]["classType"] = "bard"; });
    rejectMutation([](Json &j) { j[0]["bindings"][0]["featureId"] = "unknown"; });
    rejectMutation([](Json &j) { j[0]["bindings"][0]["acquisitionIds"] = {"unknown"}; });
    rejectMutation([](Json &j) { j[0]["bindings"][0]["acquisitionIds"] = {"school.first", "school.first"}; });
    rejectMutation([](Json &j) { j[0]["bindings"][0]["acquisitionIds"] = Json::array(); });
    rejectMutation([](Json &j) { j[0]["bindings"].push_back(j[0]["bindings"][0]); });
    rejectMutation([](Json &j) { j[0]["bindings"][0]["archetypeId"] = "unsupported"; });
    auto normalized = classes;
    normalized[0]["features"][0]["id"] = " school ";
    normalized[0]["features"][0]["progression"][0]["id"] = " school.first ";
    assert(!classDefinitionsWithOptionsFromJson(normalized, catalogs)[0].features[0].progression[0].choices.empty());
    auto recurring = classes;
    recurring[0]["features"][0]["progression"][1]["replaces"] = {"school.first"};
    auto retained = catalogs;
    retained[0]["bindings"][0]["retainEarlierAcquisitions"] = true;
    assert(classDefinitionsWithOptionsFromJson(recurring, catalogs)[0].features[0].progression[1].replaces.size() == 1);
    assert(classDefinitionsWithOptionsFromJson(recurring, retained)[0].features[0].progression[1].replaces.empty());
    recurring[0]["features"][0]["progression"][0]["effects"] = Json::array({{{"id", "automatic"}, {"type", "contribution"}, {"resource", "baseAttackBonus"}, {"expression", "1"}}});
    rejects([&] { (void)classDefinitionsWithOptionsFromJson(recurring, retained); });
}
