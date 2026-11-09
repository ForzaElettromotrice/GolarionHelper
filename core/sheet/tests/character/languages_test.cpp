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
    {
        Languages languages(resources, [&] { return ranks; }, [](auto id, auto origin)
        {
            return id != "druidic" && (origin != LanguageOrigin::Intelligence || id == "draconic");
        });
        languages.setIntelligenceBonus(1);
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
        languages.setIntelligenceBonus(0);
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
        rejects([&] { languages.setIntelligenceBonus(-1); });
        rejects([&] { languages.add(" ", LanguageOrigin::Linguistics); });
        rejects([&] { languages.add("test", static_cast<LanguageOrigin>(-1)); });
    }
    assert(reminders.toView().messages.empty());
    // Endpoint and reminders are released with the component.
    Languages replacement(resources, [] { return 0; });

    CharacterSheet sheet;
    sheet.setLanguageIntelligenceBonus(2);
    sheet.setSkillRanks(SkillType::Linguistics, 1);
    sheet.addLanguage("draconic", LanguageOrigin::Intelligence);
    sheet.addLanguage("orc", LanguageOrigin::Linguistics);
    assert(sheet.toView().languages.remainingIntelligence == 1);
    sheet.setAbilityBaseValue(AbilityType::Intelligence, 20);
    assert(sheet.toView().languages.remainingIntelligence == 1);
    const auto path = std::filesystem::temp_directory_path() / "golarion_languages_test.json";
    sheet.save(path);
    auto restored = CharacterSheet::load(path);
    assert(restored.toView().languages.spoken.size() == 2);
    assert(restored.toView().languages.remainingIntelligence == 1);
    assert(restored.toView().languages.remainingLinguistics == 0);
    restored.removeLanguage("orc");
    assert(restored.toView().languages.remainingLinguistics == 1);

    auto json = nlohmann::json::parse(persistence::toJson(sheet.toSaveData()));
    assert(json["formatVersion"] == 29);
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
