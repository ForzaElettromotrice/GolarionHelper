#include "golarion/character/character_sheet.hpp"
#include "golarion/character/languages.hpp"
#include "golarion/character/reminder.hpp"
#include "golarion/effect/effect_compiler.hpp"
#include "golarion/effect/effect_json.hpp"
#include "golarion/persistence/json.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <nlohmann/json.hpp>

#include <algorithm>
#include <cassert>
#include <filesystem>
#include <stdexcept>

namespace
{
    template<typename Function> void rejects(Function function)
    {
        bool rejected = false;
        try { function(); }
        catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    ResourceManager resources;
    ReminderManager reminders(resources);
    int ranks = 1;
    int permanentIntelligenceModifier = 1;
    resources.registerTarget(PermanentIntelligenceModifierTarget, [&] { return permanentIntelligenceModifier; });
    {
        Languages languages(resources, [&] { return ranks; }, [](auto id, auto origin)
        {
            return id != "druidic" && (origin != LanguageOrigin::Intelligence || id == "draconic");
        });
        languages.refreshReminders();
        rejects([&] { languages.add("draconic", LanguageOrigin::Intelligence); });
        auto optionsDefinition = effectDefinitionFromJson(nlohmann::json{
            {"id", "options"}, {"type", "languageOptions"}, {"origin", "intelligence"}, {"languageIds", {"draconic", "orc"}}
        });
        auto optionsCleanup = compileEffect(optionsDefinition, {"race.options", "Razza"})(resources);
        resources.addToCollection(LanguageOptionsResource, LanguageOptionsGrant{"skill.options", "Linguistica", LanguageOrigin::Linguistics, {"orc", "elf", "druidic"}});
        assert(languages.toView().options.size() == 2);
        assert(languages.toView().options[0].languageIds == std::vector<std::string>{"draconic"});
        assert(reminders.toView().messages.size() == 2);
        languages.refreshReminders();
        assert(reminders.toView().messages.size() == 2);
        resources.addToCollection(SpokenLanguagesResource, LanguageGrant{"race.common", "Razza", "common"});
        resources.addToCollection(SpokenLanguagesResource, LanguageGrant{"class.common", "Classe", "common"});
        assert(languages.toView().spoken.size() == 1);
        assert(languages.toView().spoken[0].grantedSources.size() == 2);
        assert(languages.toView().remainingIntelligence == 1);
        rejects([&] { languages.add("common", LanguageOrigin::Intelligence); });
        rejects([&] { languages.remove("common"); });
        resources.removeFromCollection(SpokenLanguagesResource, "race.common");
        assert(languages.toView().spoken.size() == 1);
        resources.removeFromCollection(SpokenLanguagesResource, "class.common");
        assert(languages.toView().spoken.empty());
        rejects([&] { languages.add("orc", LanguageOrigin::Intelligence); });
        languages.add(" draconic ", LanguageOrigin::Intelligence);
        resources.addToCollection(LanguageOptionsResource, LanguageOptionsGrant{"other.options", "Altra fonte", LanguageOrigin::Intelligence, {"draconic"}});
        optionsCleanup();
        assert(languages.toView().options[0].languageIds == std::vector<std::string>{"draconic"});
        resources.removeFromCollection(LanguageOptionsResource, "other.options");
        assert(languages.toData().learned.size() == 1);
        assert(reminders.toView().messages.size() == 2); // Unspent Linguistics and invalid retained choice.
        optionsCleanup = compileEffect(optionsDefinition, {"race.options", "Razza"})(resources);
        rejects([&] { languages.add("draconic", LanguageOrigin::Linguistics); });
        rejects([&] { languages.add("druidic", LanguageOrigin::Linguistics); });
        languages.add("orc", LanguageOrigin::Linguistics);
        assert(reminders.toView().messages.empty());
        rejects([&] { languages.add("elf", LanguageOrigin::Linguistics); });
        ranks = 2;
        languages.refreshReminders();
        assert(reminders.toView().messages.size() == 1);
        assert(languages.toView().remainingLinguistics == 1);

        const auto definition = effectDefinitionFromJson(nlohmann::json{
            {"id", "language"}, {"type", "languageGrant"}, {"languageId", "orc"}
        });
        auto cleanup = compileEffect(definition, {"effect.language", "Fonte"})(resources);
        assert(languages.toView().spoken.size() == 2);
        languages.remove("orc");
        assert(languages.toView().spoken.size() == 2);
        assert(languages.toView().remainingLinguistics == 2);
        cleanup();
        assert(languages.toView().spoken.size() == 1);

        languages.add("orc", LanguageOrigin::Linguistics);
        const auto saved = languages.toData();
        assert(saved.learned.size() == 2);
        permanentIntelligenceModifier = 0;
        ranks = 0;
        languages.refreshReminders();
        assert(languages.toView().spoken.size() == 2);
        assert(languages.toView().remainingLinguistics == 0);
        assert(reminders.toView().messages.size() == 2);
        languages.load(saved);
        assert(languages.toView().spoken.size() == 2);
        assert(languages.toView().remainingIntelligence == 0);
        auto duplicate = saved;
        duplicate.learned.push_back(saved.learned[0]);
        rejects([&] { languages.load(duplicate); });
        assert(languages.toData().learned.size() == 2);
        permanentIntelligenceModifier = -2;
        assert(languages.toView().remainingIntelligence == 0);
        rejects([&] { languages.add(" ", LanguageOrigin::Linguistics); });
        rejects([&] { languages.add("test", static_cast<LanguageOrigin>(-1)); });
        rejects([&] { resources.addToCollection(LanguageOptionsResource, LanguageOptionsGrant{"bad", "Fonte", static_cast<LanguageOrigin>(-1), {"orc"}}); });
        rejects([&] { resources.addToCollection(LanguageOptionsResource, LanguageOptionsGrant{"bad", "Fonte", LanguageOrigin::Intelligence, {"orc", " orc "}}); });
        for (const auto invalid : {
            nlohmann::json{{"origin", "unknown"}, {"languageIds", {"orc"}}},
            nlohmann::json{{"origin", "intelligence"}, {"languageIds", {"orc", " orc "}}},
            nlohmann::json{{"origin", "linguistics"}, {"languageIds", nullptr}}})
        {
            auto input = invalid;
            input["id"] = "options";
            input["type"] = "languageOptions";
            rejects([&] { effectDefinitionFromJson(input); });
        }
    }
    assert(reminders.toView().messages.empty());
    {
        Languages languages(resources, [] { return 2; });
        permanentIntelligenceModifier = 2;
        languages.refreshReminders();
        const auto apply = compileEffect(LanguageOptionsEffectDefinition{"options", LanguageOrigin::Intelligence, {"draconic"}}, {"first", "Prima fonte"});
        auto firstCleanup = apply(resources);
        auto secondCleanup = compileEffect(LanguageOptionsEffectDefinition{"options", LanguageOrigin::Intelligence, {"orc"}}, {"second", "Seconda fonte"})(resources);
        rejects([&] { apply(resources); });
        rejects([&] { languages.add("draconic", LanguageOrigin::Linguistics); });
        languages.add("draconic", LanguageOrigin::Intelligence);
        languages.add("orc", LanguageOrigin::Intelligence);
        assert(languages.toView().remainingIntelligence == 0);
        const auto saved = languages.toData();
        firstCleanup();
        assert(languages.toView().options[0].languageIds == std::vector<std::string>{"orc"});
        languages.remove("draconic");
        rejects([&] { languages.add("draconic", LanguageOrigin::Intelligence); });
        secondCleanup();
        languages.load(saved);
        assert(languages.toView().spoken.size() == 2 && languages.toView().options.empty());
        assert(reminders.toView().messages.size() == 3);
        firstCleanup = apply(resources);
        assert(reminders.toView().messages.size() == 2);
        firstCleanup();
    }
    assert(reminders.toView().messages.empty());
    // Endpoint and reminders are released with the component.
    Languages replacement(resources, [] { return 0; });

    CharacterSheet sheet;
    sheet.setAbilityBaseValue(AbilityType::Intelligence, 14);
    sheet.setSkillRanks(SkillType::Linguistics, 1);
    rejects([&] { sheet.addLanguage("draconic", LanguageOrigin::Intelligence); });
    // Persist historical selections even while their catalog sources are unavailable.
    auto savedSheet = sheet.toSaveData();
    savedSheet.languages.learned = {{"draconic", LanguageOrigin::Intelligence}, {"orc", LanguageOrigin::Linguistics}};
    sheet.setAbilityBaseValue(AbilityType::Intelligence, 20);
    assert(sheet.toView().languages.remainingIntelligence == 5);
    const auto path = std::filesystem::temp_directory_path() / "golarion_languages_test.json";
    persistence::saveToFile(savedSheet, path);
    auto restored = CharacterSheet::load(path);
    assert(restored.toView().languages.spoken.size() == 2);
    assert(restored.toView().languages.remainingIntelligence == 1);
    assert(restored.toView().languages.remainingLinguistics == 0);
    restored.removeLanguage("orc");
    assert(restored.toView().languages.remainingLinguistics == 1);

    auto json = nlohmann::json::parse(persistence::toJson(savedSheet));
    assert(json["formatVersion"] == 42);
    assert(!json["languages"].contains("intelligenceBonus"));
    auto oldSave = json;
    oldSave["formatVersion"] = 34;
    oldSave["languages"]["intelligenceBonus"] = 99;
    persistence::saveToFile(persistence::fromJson(oldSave.dump()), path);
    auto migrated = CharacterSheet::load(path);
    assert(migrated.toView().languages.spoken.size() == 2);
    assert(migrated.toView().languages.remainingIntelligence == 1);
    migrated.setAbilityBaseValue(AbilityType::Intelligence, 8);
    assert(migrated.toView().languages.spoken.size() == 2);
    assert(migrated.toView().languages.remainingIntelligence == 0);
    auto invalid = json;
    invalid["languages"]["learned"][0]["origin"] = "unknown";
    rejects([&] { persistence::fromJson(invalid.dump()); });
    invalid = json;
    invalid["languages"]["intelligenceBonus"] = 1.5;
    rejects([&] { persistence::fromJson(invalid.dump()); });
    json["formatVersion"] = 25;
    json.erase("languages");
    persistence::saveToFile(persistence::fromJson(json.dump()), path);
    auto legacy = CharacterSheet::load(path);
    assert(legacy.toView().languages.spoken.empty());
    assert(legacy.toView().languages.remainingLinguistics == 1);
    std::filesystem::remove(path);
}
