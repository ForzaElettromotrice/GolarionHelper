#include "golarion/character/archetype_definition_json.hpp"

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
    const Json fixture = {
        {"id", "test"}, {"classType", "barbarian"}, {"name", "Archetipo di prova"}, {"description", "Descrizione"},
        {"affectedFeatures", Json::array({{{"featureId", "speed"}, {"type", "replace"}}, {{"featureId", "rage"}, {"type", "alter"}}})},
        {"removedFeatureIds", {"speed"}}, {"removedAcquisitionIds", {"rage.initial"}},
        {"addedFeatures", Json::array({{{"id", "replacement"}, {"name", "Privilegio"}, {"description", "Descrizione"},
            {"progression", Json::array({{{"id", "replacement.initial"}, {"level", 1},
                {"effects", Json::array({{{"id", "notice"}, {"type", "reminder"}, {"message", "Regola manuale"}}})}}})}}})}
    };
    const auto definition = archetypeDefinitionFromJson(fixture);
    assert(definition.classType == CharacterClassType::Barbarian);
    assert(definition.affectedFeatures[0].type == ArchetypeFeatureChangeType::Replace);
    assert(definition.affectedFeatures[1].type == ArchetypeFeatureChangeType::Alter);
    const auto canonical = archetypeDefinitionToJson(definition);
    assert(archetypeDefinitionToJson(archetypeDefinitionFromJson(canonical)) == canonical);
    auto rejectMutation = [&](const auto &mutate)
    {
        auto json = fixture;
        mutate(json);
        rejects([&] { (void)archetypeDefinitionFromJson(json); });
    };
    rejectMutation([](Json &j) { j["id"] = " "; });
    rejectMutation([](Json &j) { j["name"] = " "; });
    rejectMutation([](Json &j) { j["classType"] = "unknown"; });
    rejectMutation([](Json &j) { j["affectedFeatures"][0]["type"] = "unknown"; });
    rejectMutation([](Json &j) { j["affectedFeatures"].push_back(j["affectedFeatures"][0]); });
    rejectMutation([](Json &j) { j["removedFeatureIds"] = {"unknown"}; });
    rejectMutation([](Json &j) { j["removedFeatureIds"] = Json::array(); });
    rejectMutation([](Json &j) { j["removedFeatureIds"].push_back("speed"); });
    rejectMutation([](Json &j) { j["removedAcquisitionIds"].push_back("rage.initial"); });
    rejectMutation([](Json &j) { j["removedAcquisitionIds"] = {" "}; });
    rejectMutation([](Json &j) { j["affectedFeatures"] = Json::object(); });
    rejectMutation([](Json &j) { j["addedFeatures"] = Json::object(); });
    rejectMutation([](Json &j) { j["addedFeatures"].push_back(j["addedFeatures"][0]); });
    rejectMutation([](Json &j) { j["addedFeatures"][0]["progression"][0]["level"] = 1.5; });
    rejectMutation([](Json &j) { j["addedFeatures"][0]["progression"][0]["replaces"] = {"rage.initial"}; });
    rejectMutation([](Json &j) { j["addedFeatures"][0]["progression"][0]["replaces"] = {"replacement.initial"}; });
    rejectMutation([](Json &j) { j["addedFeatures"][0]["progression"][0]["effects"][0]["type"] = "unknown"; });
    auto invalid = definition;
    invalid.affectedFeatures[0].type = static_cast<ArchetypeFeatureChangeType>(-1);
    rejects([&] { (void)archetypeDefinitionToJson(invalid); });
}
