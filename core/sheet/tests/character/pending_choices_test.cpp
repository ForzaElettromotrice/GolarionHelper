#include "golarion/character/character_sheet.hpp"
#include "golarion/character/race_definition_json.hpp"
#include "golarion/character/race_definition_catalog.hpp"
#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/character/flexible_spellcasting.hpp"
#include "golarion/character/spontaneous_spellcasting.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/persistence/json.hpp"
#include "../fixtures/class_definition.hpp"
#include <nlohmann/json.hpp>
#include <algorithm>
#include <cassert>
#include <filesystem>

namespace
{
    using namespace golarion;
    template<class Reference> const PendingChoice *find(const std::vector<PendingChoice> &choices, const Reference &reference)
    {
        const auto found = std::find_if(choices.begin(), choices.end(), [&](const auto &choice)
        {
            const auto *target = std::get_if<Reference>(&choice.choiceReference);
            return target && *target == reference;
        });
        return found == choices.end() ? nullptr : &*found;
    }
    template<class Reference> void check(const std::vector<PendingChoice> &choices, const Reference &reference, std::size_t remaining, bool blocks)
    {
        const auto *entry = find(choices, reference);
        assert(entry && entry->remaining == remaining && entry->blocksAdvancement == blocks);
        assert(entry->type() == Reference::Type && !entry->description.empty());
    }
    template<class F> void rejects(F action)
    {
        bool rejected = false;
        try { action(); } catch (const std::exception &) { rejected = true; }
        assert(rejected);
    }
}

int main()
{
    using namespace golarion;
    auto fighter = test::classDefinition(CharacterClassType::Fighter);
    fighter.features = {{"path", "Percorso", "Descrizione", {
        {.id = "start", .level = 1, .choices = {
            {"pick", "Scegli due opzioni", 2, {
                {"a", "A", "A", {{.id = "nested", .level = 1, .choices = {{"child", "Scelta interna", 1, {{"x", "X", "X"}}}}}}},
                {"b", "B", "B"}, {"c", "C", "C"}
            }, false},
            {"required", "Scelta obbligatoria", 1, {{"x", "X", "X"}}}
        }},
        {.id = "upgrade", .level = 2, .replaces = {"start"}},
        {.id = "future", .level = 3, .choices = {{"later", "Scelta futura", 1, {{"x", "X", "X"}}}}}
    }}};
    const ClassDefinitionManager catalog({fighter});
    CharacterSheet sheet(catalog);
    auto pending = sheet.pendingChoices();
    assert(pending.size() == 3);
    check(pending, RaceChoiceReference{}, 1, true);
    check(pending, AlignmentChoiceReference{}, 1, true);
    check(pending, FavoredClassChoiceReference{}, 1, true);
    sheet.setAlignment(Alignment::Neutral);
    sheet.setFavoredClass(CharacterClassType::Fighter);
    sheet.setRace("test");
    pending = sheet.pendingChoices();
    check(pending, RacialOptionChoiceReference{"test", "feature", "required"}, 2, true);
    check(pending, RacialOptionChoiceReference{"test", "feature", "optional"}, 1, false);
    assert(!sheet.toView().race.activeElements.front().missingChoices.back().blocksAdvancement);
    sheet.setRacialChoice("feature", "required", {"a", "b"});
    assert(!find(sheet.pendingChoices(), RacialOptionChoiceReference{"test", "feature", "required"}));
    sheet.selectAlternateRacialFeature("replacement");
    assert(sheet.pendingChoices().empty());
    sheet.removeAlternateRacialFeature("replacement");
    assert(!find(sheet.pendingChoices(), RacialOptionChoiceReference{"test", "feature", "required"}));
    sheet.setRacialChoice("feature", "optional", {"a"});

    // Reporting is read-only; nonblocking missing choices allow advancement.
    sheet.addClassLevel(CharacterClassType::Fighter);
    pending = sheet.pendingChoices();
    check(pending, ClassHitDieChoiceReference{CharacterClassType::Fighter, 1}, 1, false);
    check(pending, ClassOptionChoiceReference{CharacterClassType::Fighter, "start", "pick"}, 2, false);
    check(pending, ClassOptionChoiceReference{CharacterClassType::Fighter, "start", "required"}, 1, true);
    assert(!find(pending, ClassOptionChoiceReference{CharacterClassType::Fighter, "nested", "child"}));
    assert(!find(pending, ClassOptionChoiceReference{CharacterClassType::Fighter, "future", "later"}));
    check(pending, FavoredClassBonusChoiceReference{CharacterClassType::Fighter, 1}, 1, false);
    check(pending, FeatSlotChoiceReference{"base.feat.level1"}, 1, true);
    check(pending, SkillRanksChoiceReference{}, 2, false);
    sheet.selectClassOption(CharacterClassType::Fighter, {"start", "pick", "a"});
    check(sheet.pendingChoices(), ClassOptionChoiceReference{CharacterClassType::Fighter, "start", "pick"}, 1, false);
    check(sheet.pendingChoices(), ClassOptionChoiceReference{CharacterClassType::Fighter, "nested", "child"}, 1, true);
    sheet.selectClassOption(CharacterClassType::Fighter, {"start", "pick", "b"});
    assert(!find(sheet.pendingChoices(), ClassOptionChoiceReference{CharacterClassType::Fighter, "start", "pick"}));
    sheet.setClassHitDieResult(CharacterClassType::Fighter, 1, 8);
    sheet.selectFavoredClassBonus(CharacterClassType::Fighter, 1, "base.hitPoint");
    sheet.selectFeat("base.feat.level1", "improvedInitiative");
    sheet.addExtraFeatSlot("extra");
    check(sheet.pendingChoices(), FeatSlotChoiceReference{"extra"}, 1, false);
    assert(!find(sheet.pendingChoices(), ClassHitDieChoiceReference{CharacterClassType::Fighter, 1}));
    assert(!find(sheet.pendingChoices(), FavoredClassBonusChoiceReference{CharacterClassType::Fighter, 1}));
    sheet.setSkillRanks(SkillType::Climb, 2);
    assert(!find(sheet.pendingChoices(), SkillRanksChoiceReference{}));
    sheet.setSkillRanks(SkillType::Climb, 5);
    assert(!find(sheet.pendingChoices(), SkillRanksChoiceReference{})); // Overspending is not a missing choice.
    sheet.setAbilityBaseValue(AbilityType::Intelligence, 14);
    sheet.setSkillRanks(SkillType::Linguistics, 2);
    check(sheet.pendingChoices(), LanguageChoiceReference{LanguageOrigin::Intelligence}, 2, false);
    check(sheet.pendingChoices(), LanguageChoiceReference{LanguageOrigin::Linguistics}, 2, false);
    const auto snapshot = sheet.pendingChoices();
    const auto before = persistence::toJson(sheet.toSaveData());
    assert(sheet.pendingChoices() == snapshot && persistence::toJson(sheet.toSaveData()) == before);
    const auto path = std::filesystem::temp_directory_path() / "golarion-pending-choices-test.json";
    sheet.save(path);
    auto restored = CharacterSheet::load(path, catalog);
    assert(restored.pendingChoices() == snapshot);
    std::filesystem::remove(path);
    rejects([&] { sheet.addClassLevel(CharacterClassType::Fighter, 4); });
    sheet.selectClassOption(CharacterClassType::Fighter, {"start", "required", "x"});
    sheet.selectClassOption(CharacterClassType::Fighter, {"nested", "child", "x"});
    sheet.addClassLevel(CharacterClassType::Fighter, 4);
    assert(!find(sheet.pendingChoices(), ClassOptionChoiceReference{CharacterClassType::Fighter, "start", "required"}));
    assert(!find(sheet.pendingChoices(), ClassOptionChoiceReference{CharacterClassType::Fighter, "nested", "child"}));
    sheet.removeClassLevel(CharacterClassType::Fighter);
    assert(!find(sheet.pendingChoices(), ClassOptionChoiceReference{CharacterClassType::Fighter, "nested", "child"}));
    assert(snapshot == restored.pendingChoices());

    auto raceJson = nlohmann::json::parse(embedded::RaceDefinitionCatalogJson).front();
    assert(raceDefinitionFromJson(raceJson).standardFeatures.front().choices.front().blocksAdvancement);
    raceJson["standardFeatures"][0]["choices"][0]["blocksAdvancement"] = "yes";
    rejects([&] { raceDefinitionFromJson(raceJson); });

    ResourceManager resources;
    FavoredClasses favored(resources);
    favored.setClasses({CharacterClassType::Fighter});
    resources.addToCollection(FavoredClasses::GrantsResource, FavoredClassGrant{"second", "Seconda", 1});
    check(favored.pendingChoices(), FavoredClassChoiceReference{}, 1, true);
    favored.setClasses({CharacterClassType::Fighter, CharacterClassType::Wizard});
    assert(favored.pendingChoices().empty());
    resources.removeFromCollection(FavoredClasses::GrantsResource, "second");
    assert(favored.pendingChoices().empty()); // Excess choices are validation issues, not pending selections.

    Feats feats(resources);
    resources.addToCollection(FeatSlotsResource, FeatSlot{.id="source", .source="Fonte", .filter=FeatTypeFilter{FeatType::Combat}});
    check(feats.pendingChoices(), FeatSlotChoiceReference{"source"}, 1, true);
    resources.removeFromCollection(FeatSlotsResource, "source");
    assert(feats.pendingChoices().empty());

    Spellcastings castings(resources);
    for (const auto &[id, mode] : {std::pair{"p", SpellcastingMode::Prepared}, {"s", SpellcastingMode::Spontaneous}, {"f", SpellcastingMode::Flexible}})
    {
        resources.addToCollection(Spellcastings::Resource, SpellcastingGrant{id, "Incantatore", mode, CharacterClassType::Wizard});
        resources.addToCollection(spellSlotsResource(id), SpellSlotGrant{"slots", "Slot", {{3, 1}}});
        resources.addToCollection(repertoireQuotasResource(id), SpellRepertoireQuota{"known", "Class", {{3, 1}}});
        castings.get(id).selectRepertoireSpell({"known", 3, "fireball"});
    }
    resources.addToCollection(preparationSlotsResource("f"), SpellSlotGrant{"preparations", "Preparazioni", {{3, 1}}});
    auto &prepared = dynamic_cast<PreparedSpellcasting &>(castings.get("p"));
    auto &flexible = dynamic_cast<FlexibleSpellcasting &>(castings.get("f"));
    pending = castings.pendingChoices();
    assert(pending.size() == 2); // Spontaneous casting slots do not require a selection.
    check(pending, PreparedSpellChoiceReference{"p", {"slots", 3, 0}}, 1, false);
    check(pending, FlexiblePreparationChoiceReference{"f", {"preparations", 3, 0}}, 1, false);
    prepared.prepare(SpellSlotId{"slots", 3, 0}, {"fireball"});
    flexible.prepare({"preparations", 3, 0}, {"fireball"});
    assert(castings.pendingChoices().empty());
    prepared.consume({"slots", 3, 0});
    assert(castings.pendingChoices().empty()); // Consumed is not unprepared.
    prepared.clear({"slots", 3, 0});
    assert(castings.pendingChoices().size() == 1);

    // Rank completion does not depend on unrelated skill-check/armor targets.
    ResourceManager isolatedResources;
    Skills isolatedSkills(isolatedResources);
    assert(isolatedSkills.pendingChoices().empty());
}
