#include "golarion/catalog/catalog_reader.hpp"
#include "golarion/character/class_definition_manager.hpp"
#include "golarion/character/race_definition_manager.hpp"
#include "golarion/character/feat_definition_manager.hpp"
#include "golarion/character/spell_definition_manager.hpp"
#include "golarion/equipment/item_definition_manager.hpp"

#include <chrono>
#include <stdexcept>

namespace fs = std::filesystem;
using namespace golarion;

static void check(bool ok)
{
    if (!ok) throw std::runtime_error("Unexpected lazy catalog state");
}

static void requiresFile(auto &&read)
{
    try { read(); }
    catch (const catalog::CatalogReadError &) { return; }
    throw std::runtime_error("Expected a missing catalog file");
}

int main(int argc, char **argv)
{
    if (argc != 3) return 1;
    const fs::path source = fs::path(argv[1]) / "class-library";
    const auto sandbox = fs::path(argv[2]) / std::to_string(std::chrono::steady_clock::now().time_since_epoch().count());
    const auto library = sandbox / "class-library";
    fs::create_directories(library / "classes");
    fs::create_directories(library / "archetypes");
    catalog::setDirectory(sandbox);
    fs::copy_file(source / "index.json", library / "index.json");
    const auto &manager = ClassDefinitionManager::instance();
    check(manager.listClasses().size() == 44);
    const auto type = CharacterClassType::Wizard;
    requiresFile([&] { manager.get(type); });
    fs::copy_file(source / "classes/wizard.json", library / "classes/wizard.json");
    fs::copy(source / "meta", library / "meta", fs::copy_options::recursive);
    fs::copy(source / "options", library / "options", fs::copy_options::recursive);
    const auto options = manager.listArchetypes(type);
    check(!options.empty());
    const auto &wizard = manager.get(type);
    fs::remove(library / "classes/wizard.json");
    check(&manager.get(type) == &wizard);
    const auto id = options.front().id;
    requiresFile([&] { manager.getArchetype(id); });
    const fs::path file = fs::path("archetypes") / (id + ".json");
    fs::copy_file(source / file, library / file);
    const auto &archetype = manager.getArchetype(id);
    check(archetype.classType == type && archetype.name == options.front().name);
    fs::remove(library / file);
    check(&manager.getArchetype(id) == &archetype);
    requiresFile([&] { manager.get(CharacterClassType::Bard); });
    const auto raceSource = fs::path(argv[1]) / "race-library";
    const auto raceLibrary = sandbox / "race-library";
    fs::create_directories(raceLibrary / "races");
    fs::copy_file(raceSource / "index.json", raceLibrary / "index.json");
    auto &races = RaceDefinitionManager::instance();
    check(races.listRaces().size() == 63);
    requiresFile([&] { races.get("dwarf"); });
    fs::copy_file(raceSource / "races/dwarf.json", raceLibrary / "races/dwarf.json");
    const auto &dwarf = races.get("dwarf");
    fs::remove(raceLibrary / "races/dwarf.json");
    check(&races.get("dwarf") == &dwarf);
    check(races.alternateFeaturesView("dwarf").size() == dwarf.alternateFeatures.size());
    requiresFile([&] { races.get("human"); });
    const auto spellSource = fs::path(argv[1]) / "spell-library";
    const auto spellLibrary = sandbox / "spell-library";
    fs::create_directories(spellLibrary / "f");
    fs::copy_file(spellSource / "index.json", spellLibrary / "index.json");
    auto &spells = SpellDefinitionManager::instance();
    check(spells.contains("fireball"));
    bool hasClericSpell = false;
    for (const auto &entry : spells.catalogEntries())
        hasClericSpell |= entry.levels.contains(CharacterClassType::Cleric);
    check(hasClericSpell);
    requiresFile([&] { spells.get("fireball"); });
    fs::copy_file(spellSource / "f/fireball.json", spellLibrary / "f/fireball.json");
    const auto &fireball = spells.get("fireball");
    fs::remove(spellLibrary / "f/fireball.json");
    check(&spells.get("fireball") == &fireball);
    const auto featSource = fs::path(argv[1]) / "feat-library";
    const auto featLibrary = sandbox / "feat-library";
    fs::create_directories(featLibrary / "i");
    fs::copy_file(featSource / "index.json", featLibrary / "index.json");
    auto &feats = FeatDefinitionManager::instance();
    check(feats.contains("improvedInitiative") && !feats.contains("notRegistered"));
    bool found = false;
    for (const auto &entry : feats.catalogEntries())
        if (entry.id == "improvedInitiative")
            found = entry.name == "Iniziativa Migliorata" && entry.types == std::vector{FeatType::Combat};
    check(found);
    requiresFile([&] { feats.get("improvedInitiative"); });
    fs::copy_file(featSource / "i/improvedInitiative.json", featLibrary / "i/improvedInitiative.json");
    const auto &feat = feats.get("improvedInitiative");
    fs::remove(featLibrary / "i/improvedInitiative.json");
    check(&feats.get(" improvedInitiative ") == &feat);
    const auto itemSource = fs::path(argv[1]) / "item-library";
    const auto itemLibrary = sandbox / "item-library";
    fs::create_directories(itemLibrary / "g");
    fs::copy_file(itemSource / "index.json", itemLibrary / "index.json");
    auto &items = ItemDefinitionManager::instance();
    check(items.contains("goldCoin") && !items.catalogEntries().empty());
    requiresFile([&] { items.get("goldCoin"); });
    fs::copy_file(itemSource / "g/goldCoin.json", itemLibrary / "g/goldCoin.json");
    const auto &coin = items.get("goldCoin");
    fs::remove(itemLibrary / "g/goldCoin.json");
    check(&items.get(" goldCoin ") == &coin);
    fs::remove_all(sandbox);
}
