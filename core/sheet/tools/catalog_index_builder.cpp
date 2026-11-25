#include "golarion/character/archetype_definition_catalog.hpp"
#include "golarion/character/archetype_definition_json.hpp"
#include "golarion/character/class_definition_catalog.hpp"
#include "golarion/character/class_definition_json.hpp"
#include "golarion/character/class_option_catalog.hpp"
#include "golarion/character/class_option_catalog_json.hpp"
#include "golarion/character/race_definition_catalog.hpp"
#include "golarion/character/race_definition_json.hpp"
#include "golarion/character/spell_definition_catalog.hpp"
#include "golarion/character/feat_definition_catalog.hpp"
#include "golarion/character/feat_definition_json.hpp"
#include "golarion/character/spell_definition_json.hpp"
#include "golarion/catalog/definition_path.hpp"
#include "golarion/equipment/item_definition_catalog.hpp"
#include "golarion/equipment/item_definition_json.hpp"
#include "golarion/util/string_utils.hpp"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <set>

namespace
{
    using namespace golarion;
    using Json = nlohmann::json;

    void write(const std::filesystem::path &path, const Json &data)
    {
        std::ofstream file;
        file.exceptions(std::ios::badbit | std::ios::failbit);
        file.open(path, std::ios::binary);
        file << data;
        file.close();
    }

    void collectSets(const Json &node, const std::map<std::string, Json> &sets, std::set<std::string> &needed)
    {
        if (!node.is_structured()) return;
        if (node.contains("optionSet"))
        {
            const auto id = normalize(node.at("optionSet").get<std::string>());
            if (needed.insert(id).second) collectSets(sets.at(id), sets, needed);
        }
        for (const auto &child : node) collectSets(child, sets, needed);
    }

    void checkSource(const std::filesystem::path &folder, const std::string &id, const Json &expected)
    {
        if (id.empty() || id.front() == '.' || id.find_first_not_of("abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789._-") != std::string::npos)
            throw std::invalid_argument("Invalid catalog filename ID: " + id);
        std::ifstream file(folder / (id + ".json"), std::ios::binary);
        if (!file || Json::parse(file) != expected) throw std::invalid_argument("Catalog filename does not match ID: " + id);
    }

    void build(const std::filesystem::path &output)
    {
        catalog::setDirectory(output);
        const auto classes = catalog::loadClassDefinitionCatalog();
        const auto allSets = catalog::loadClassOptionSets();
        std::map<std::string, Json> sets;
        auto allBindings = Json::array();
        for (const auto &set : allSets)
        {
            const auto id = set.at("id").get<std::string>();
            checkSource(output / "class-library/options", id, set);
            sets.emplace(id, set);
        }
        for (const auto &entry : classes)
            for (const auto &binding : entry.at("bindings"))
            {
                if (binding.at("classType") != entry.at("type")) throw std::invalid_argument("Wrong binding owner");
                allBindings.push_back(binding);
            }
        const auto validated = classDefinitionsWithOptionsFromJson(classes, Json::array({Json{{"bindings", allBindings}, {"optionSets", allSets}}}));
        std::map<CharacterClassType, const ClassDefinition *> byType;
        for (const auto &definition : validated) byType.emplace(definition.type, &definition);
        std::map<CharacterClassType, Json> summaries;
        std::set<std::string> ids;
        for (const auto &entry : catalog::loadArchetypeDefinitionCatalog())
        {
            const auto archetype = archetypeDefinitionFromJson(entry);
            const auto &owner = *byType.at(archetype.classType);
            checkSource(output / "class-library/archetypes", archetype.id, entry);
            if (!ids.insert(archetype.id).second || !archetype.id.starts_with(std::string(characterClassId(owner.type)) + ".archetype."))
                throw std::invalid_argument("Invalid or duplicate catalog archetype ID: " + archetype.id);
            for (const auto &impact : archetype.affectedFeatures)
                if (std::none_of(owner.features.begin(), owner.features.end(), [&](const auto &feature){ return feature.id == impact.featureId; }))
                    throw std::invalid_argument("Unknown affected feature: " + impact.featureId);
            summaries[owner.type].push_back({{"id", archetype.id}, {"name", archetype.name}});
        }
        auto index = Json::array();
        std::filesystem::create_directories(output / "class-library/meta");
        for (const auto &entry : classes)
        {
            const auto id = entry.at("type").get<std::string>();
            const auto type = characterClassFromId(id);
            checkSource(output / "class-library/classes", id, entry);
            std::set<std::string> needed;
            collectSets(entry.at("bindings"), sets, needed);
            write(output / "class-library/meta" / (id + ".json"), {{"optionSets", needed},
                {"archetypes", summaries.contains(type) ? summaries.at(type) : Json::array()}});
            index.push_back({{"type", id}, {"name", entry.at("name")}});
        }
        write(output / "class-library/index.json", index);
        auto races = Json::array();
        ids.clear();
        for (const auto &entry : catalog::loadRaceDefinitionCatalog())
        {
            const auto race = raceDefinitionFromJson(entry);
            checkSource(output / "race-library/races", race.id, entry);
            if (!ids.insert(race.id).second) throw std::invalid_argument("Duplicate race ID: " + race.id);
            races.push_back({{"id", race.id}, {"name", race.name}});
        }
        write(output / "race-library/index.json", races);
        auto spells = Json::array();
        ids.clear();
        for (const auto &entry : catalog::loadSpellDefinitionCatalog())
        {
            const auto spell = spellDefinitionFromJson(entry);
            checkSource(output / catalog::definitionPath("spell-library", spell.id).parent_path(), spell.id, entry);
            auto key = spell.id;
            for (char &c : key) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
            if (!ids.insert(key).second) throw std::invalid_argument("Duplicate spell filename: " + spell.id);
            auto levels = Json::object();
            for (const auto &[type, level] : spell.spellLevelByClass) levels[characterClassId(type)] = level;
            spells.push_back({{"id", spell.id}, {"name", spell.name}, {"levels", std::move(levels)}});
        }
        write(output / "spell-library/index.json", spells);
        auto feats = Json::array();
        ids.clear();
        for (const auto &entry : catalog::loadFeatDefinitionCatalog())
        {
            const auto feat = featDefinitionFromJson(entry);
            checkSource(output / catalog::definitionPath("feat-library", feat.id).parent_path(), feat.id, entry);
            auto key = feat.id;
            for (char &c : key) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
            if (!ids.insert(key).second) throw std::invalid_argument("Duplicate feat filename: " + feat.id);
            auto types = Json::array();
            for (const auto &type : entry.at("types")) types.push_back(normalize(type.get<std::string>()));
            feats.push_back({{"id", feat.id}, {"name", feat.name}, {"types", std::move(types)}});
        }
        write(output / "feat-library/index.json", feats);
        auto items = Json::array();
        ids.clear();
        for (const auto &entry : catalog::loadItemDefinitionCatalog())
        {
            const auto item = itemDefinitionFromJson(entry);
            checkSource(output / catalog::definitionPath("item-library", item.id).parent_path(), item.id, entry);
            auto key = item.id;
            for (char &c : key) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
            if (!ids.insert(key).second) throw std::invalid_argument("Duplicate item filename: " + item.id);
            items.push_back({{"id", item.id}, {"name", item.name}});
        }
        write(output / "item-library/index.json", items);
    }
}

int main(int argc, char **argv)
{
    try
    {
        if (argc != 2) throw std::invalid_argument("Expected staged catalog directory");
        build(argv[1]);
        return 0;
    }
    catch (const std::exception &error) { std::cerr << error.what() << '\n'; return 1; }
}
