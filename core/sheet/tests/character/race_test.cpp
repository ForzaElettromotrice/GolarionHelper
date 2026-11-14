#include "golarion/character/ability.hpp"
#include "golarion/character/feats.hpp"
#include "golarion/character/favored_classes.hpp"
#include "golarion/character/special_defenses.hpp"
#include "golarion/character/initiative.hpp"
#include "golarion/character/languages.hpp"
#include "golarion/character/armor_class.hpp"
#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/combat_maneuvers.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/race.hpp"
#include "golarion/character/reminder.hpp"
#include "golarion/character/saving_throws.hpp"
#include "golarion/character/size.hpp"
#include "golarion/character/skills.hpp"
#include "golarion/character/strike.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/carrying_capacity_view.hpp"
#include "golarion/view/movement_view.hpp"
#include "golarion/view/size_view.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <ranges>
#include <stdexcept>
#include <string>
#include <string_view>

namespace
{
    template<typename Exception, typename Function>
    bool throws(Function function)
    {
        try
        {
            function();
            return false;
        }
        catch (const Exception &)
        {
            return true;
        }
    }

    const golarion::RacialElementView &elementView(const golarion::RaceView &view, std::string_view elementId)
    {
        const auto element = std::ranges::find(view.activeElements, elementId, &golarion::RacialElementView::id);
        if (element == view.activeElements.end())
        {
            throw std::logic_error("expected racial element is missing from the View");
        }
        return *element;
    }
}

int main()
{
    using namespace golarion;

    ResourceManager resourceManager;
    int level = 1;
    resourceManager.registerTarget("level", [&] { return level; });
    AbilityChecks abilityChecks(resourceManager);
    std::array abilities{
        AbilityScore(AbilityType::Strength),
        AbilityScore(AbilityType::Dexterity),
        AbilityScore(AbilityType::Constitution),
        AbilityScore(AbilityType::Intelligence),
        AbilityScore(AbilityType::Wisdom),
        AbilityScore(AbilityType::Charisma)
    };
    for (const AbilityScore &ability : abilities)
    {
        ability.registerResources(resourceManager);
    }

    Strikes strikes(resourceManager);
    CombatManeuvers combatManeuvers(resourceManager);
    ArmorClass armorClass(resourceManager);
    Skills skills(resourceManager);
    SavingThrows savingThrows(resourceManager);
    ReminderManager reminderManager(resourceManager);
    Feats feats(resourceManager);
    FavoredClasses favoredClasses(resourceManager);
    SpecialDefenses specialDefenses(resourceManager);
    Initiative initiative(resourceManager);
    Languages languages(resourceManager, [] { return 0; });
    Movement movement(resourceManager);
    CarryingCapacity carryingCapacity(resourceManager);
    SizeManager sizeManager(resourceManager);

    static_cast<void>(abilityChecks);
    static_cast<void>(strikes);
    static_cast<void>(combatManeuvers);
    static_cast<void>(armorClass);
    static_cast<void>(skills);

    Race race(resourceManager);
    RaceView view = race.toView();
    assert(!view.race.has_value());
    assert(view.activeElements.empty());
    assert(throws<std::logic_error>([&race]
    {
        race.selectAlternateFeature("human.dualTalent");
    }));
    assert(throws<std::logic_error>([&race]
    {
        race.setChoice("human.abilityScores", "ability", {"strength"});
    }));

    race.setRace("human");
    view = race.toView();
    assert(view.race.has_value());
    assert(view.race->id == "human");
    assert(view.race->name == "Umano");
    assert(view.activeElements.size() == 8);
    const RacialElementView &abilityScores = elementView(view, "human.abilityScores");
    assert(abilityScores.choices.empty());
    assert(abilityScores.missingChoices.size() == 1);
    assert(abilityScores.missingChoices[0].id == "ability");
    assert(abilityScores.missingChoices[0].selectionCount == 1);
    assert(abilityScores.missingChoices[0].options.size() == 6);
    assert(abilityScores.missingChoices[0].options[0].name == "Forza");
    assert(sizeManager.toView().base.has_value());
    assert(sizeManager.toView().effectiveCategory == SizeCategory::Medium);
    assert(movement.toView().grants.size() == 1);
    assert(movement.toView().grants[0].effectiveUnits == 6);
    assert(carryingCapacity.toView().bodyType == CarryingBodyType::Biped);
    assert(reminderManager.toView().messages.size() == 1);
    assert(resourceManager.contributionTotal(MaxSkillRanksResource) == 1);
    assert(feats.toView().slots.size() == 1);
    assert(languages.toView().spoken.front().id == "common");
    level = 5;
    assert(resourceManager.contributionTotal(MaxSkillRanksResource) == 5);
    assert(resourceManager.modifierTotal("str") == 0);

    assert(throws<std::invalid_argument>([&race]
    {
        race.setChoice("human.abilityScores", "ability", {});
    }));
    assert(throws<std::invalid_argument>([&race]
    {
        race.setChoice("human.abilityScores", "ability", {"missing"});
    }));
    race.setChoice("human.abilityScores", "ability", {"strength"});
    assert(resourceManager.modifierTotal("str") == 2);
    view = race.toView();
    assert(elementView(view, "human.abilityScores").missingChoices.empty());
    assert(elementView(view, "human.abilityScores").choices.size() == 1);
    assert(elementView(view, "human.abilityScores").choices[0].selectedOptions[0].id == "strength");

    race.setRace("human");
    assert(resourceManager.modifierTotal("str") == 2);

    race.selectAlternateFeature("human.dualTalent");
    assert(throws<std::invalid_argument>([&race]
    {
        race.selectAlternateFeature("human.dualTalent");
    }));
    view = race.toView();
    assert(view.activeElements.size() == 6);
    assert(std::ranges::find(view.activeElements, "human.abilityScores", &RacialElementView::id) == view.activeElements.end());
    assert(std::ranges::find(view.activeElements, "human.skilled", &RacialElementView::id) == view.activeElements.end());
    assert(std::ranges::find(view.activeElements, "human.bonusFeat", &RacialElementView::id) == view.activeElements.end());
    const RacialElementView &missingDualTalent = elementView(view, "human.dualTalent");
    assert(missingDualTalent.missingChoices.size() == 1);
    assert(missingDualTalent.missingChoices[0].selectionCount == 2);
    assert(resourceManager.modifierTotal("str") == 0);
    assert(reminderManager.toView().messages.size() == 1);
    assert(resourceManager.contributionTotal(MaxSkillRanksResource) == 0);
    assert(feats.toView().slots.empty());
    assert(throws<std::invalid_argument>([&race]
    {
        race.setChoice("human.abilityScores", "ability", {"dexterity"});
    }));
    assert(throws<std::invalid_argument>([&race]
    {
        race.setChoice("human.dualTalent", "abilities", {"strength", "strength"});
    }));

    race.setChoice("human.dualTalent", "abilities", {"constitution", "strength"});
    assert(resourceManager.modifierTotal("str") == 2);
    assert(resourceManager.modifierTotal("con") == 2);
    view = race.toView();
    const RacialElementView &dualTalent = elementView(view, "human.dualTalent");
    assert(dualTalent.missingChoices.empty());
    assert(dualTalent.choices.size() == 1);
    assert(dualTalent.choices[0].selectedOptions.size() == 2);
    assert(dualTalent.choices[0].selectedOptions[0].id == "strength");
    assert(dualTalent.choices[0].selectedOptions[1].id == "constitution");

    race.removeAlternateFeature("human.dualTalent");
    assert(resourceManager.modifierTotal("str") == 2);
    assert(resourceManager.modifierTotal("con") == 0);
    assert(reminderManager.toView().messages.size() == 1);
    assert(resourceManager.contributionTotal(MaxSkillRanksResource) == 5);
    assert(elementView(race.toView(), "human.abilityScores").missingChoices.empty());
    assert(throws<std::invalid_argument>([&race]
    {
        race.removeAlternateFeature("human.dualTalent");
    }));

    race.setChoice("human.abilityScores", "ability", {"dexterity"});
    assert(resourceManager.modifierTotal("str") == 0);
    assert(resourceManager.modifierTotal("dex") == 2);

    race.selectAlternateFeature("human.dualTalent");
    assert(resourceManager.modifierTotal("str") == 2);
    assert(resourceManager.modifierTotal("dex") == 0);
    assert(resourceManager.modifierTotal("con") == 2);
    assert(elementView(race.toView(), "human.dualTalent").missingChoices.empty());

    const RaceSaveData saveData = race.toSaveData();
    assert(saveData.raceDefinitionId == "human");
    assert((saveData.alternateFeatureIds == std::vector<std::string>{"human.dualTalent"}));
    assert(saveData.choices.size() == 2);
    assert(saveData.choices[0].elementId == "human.abilityScores");
    assert(saveData.choices[0].choiceId == "ability");
    assert((saveData.choices[0].optionIds == std::vector<std::string>{"dexterity"}));
    assert(saveData.choices[1].elementId == "human.dualTalent");
    assert(saveData.choices[1].choiceId == "abilities");
    assert((saveData.choices[1].optionIds == std::vector<std::string>{"strength", "constitution"}));

    race.clearRace();
    view = race.toView();
    assert(!view.race.has_value());
    assert(view.activeElements.empty());
    assert(resourceManager.modifierTotal("str") == 0);
    assert(resourceManager.modifierTotal("con") == 0);
    assert(!sizeManager.toView().base.has_value());
    assert(movement.toView().grants.empty());
    assert(!carryingCapacity.toView().bodyTypeBase.has_value());
    assert(reminderManager.toView().messages.empty());

    race.load(saveData);
    assert(race.toView().race->id == "human");
    assert(resourceManager.modifierTotal("str") == 2);
    assert(resourceManager.modifierTotal("dex") == 0);
    assert(resourceManager.modifierTotal("con") == 2);
    race.removeAlternateFeature("human.dualTalent");
    assert(resourceManager.modifierTotal("str") == 0);
    assert(resourceManager.modifierTotal("dex") == 2);
    assert(resourceManager.modifierTotal("con") == 0);
    race.clearRace();
    assert(!race.toSaveData().raceDefinitionId.has_value());

    assert(throws<std::invalid_argument>([&race]
    {
        race.setRace("missing");
    }));

    struct CoreRaceExpectation
    {
        std::string_view id;
        std::array<int, 6> abilityScores;
        SizeCategory size;
        int speedUnits;
        bool slowedByLoad;
    };
    const std::array coreRaces{
        CoreRaceExpectation{"elf", {10, 12, 8, 12, 10, 10}, SizeCategory::Medium, 6, true},
        CoreRaceExpectation{"gnome", {8, 10, 12, 10, 10, 12}, SizeCategory::Small, 4, true},
        CoreRaceExpectation{"halfling", {8, 12, 10, 10, 10, 12}, SizeCategory::Small, 4, true},
        CoreRaceExpectation{"halfElf", {10, 10, 10, 10, 10, 10}, SizeCategory::Medium, 6, true},
        CoreRaceExpectation{"halfOrc", {10, 10, 10, 10, 10, 10}, SizeCategory::Medium, 6, true},
        CoreRaceExpectation{"dwarf", {10, 10, 12, 10, 12, 8}, SizeCategory::Medium, 4, false}
    };
    const std::array<std::string_view, 6> abilityTargets{"str", "dex", "con", "int", "wis", "cha"};
    const auto assertContextTotal = [&resourceManager](std::string_view resource, const std::string &condition, int expected)
    {
        assert(resourceManager.modifierSetView(resource, {condition}).total == expected);
    };

    for (const CoreRaceExpectation &expected : coreRaces)
    {
        race.setRace(expected.id);
        assert(race.toView().race->id == expected.id);
        for (std::size_t index = 0; index < abilityTargets.size(); ++index)
        {
            assert(resourceManager.targetValue(abilityTargets[index]) == expected.abilityScores[index]);
        }
        assert(sizeManager.toView().effectiveCategory == expected.size);
        const MovementView movementView = movement.toView();
        assert(movementView.grants.size() == 1);
        assert(movementView.grants[0].effectiveUnits == expected.speedUnits);
        assert(movementView.grants[0].affectedByArmor == expected.slowedByLoad);
        assert(movementView.grants[0].affectedByLoad == expected.slowedByLoad);
        assert(movementView.grants[0].run.supported);
        const int sizeBonus = expected.size == SizeCategory::Small ? 1 : 0;
        assert(resourceManager.modifierTotal("attack.melee") == sizeBonus);
        assert(resourceManager.modifierTotal("armorClass.normal") == sizeBonus);
        assert(resourceManager.modifierTotal("skill.stealth") == 4 * sizeBonus);
        assert(resourceManager.modifierTotal("combatManeuver.defense.bullRush") == -sizeBonus);
        assert(resourceManager.modifierTotal("savingThrow.all") == (expected.id == "halfling" ? 1 : 0));
        assert(resourceManager.modifierTotal("skill.perception") == (expected.id == "halfOrc" || expected.id == "dwarf" ? 0 : 2));

        if (expected.id == "elf" || expected.id == "halfElf")
        {
            assertContextTotal("savingThrow.fortitude", "Contro incantesimi ed effetti di ammaliamento", 2);
        }
        if (expected.id == "elf")
        {
            assert(resourceManager.modifierTotal("skill.spellcraft") == 0);
            assertContextTotal("skill.spellcraft", "Per identificare le proprietà degli oggetti magici", 2);
        }
        if (expected.id == "gnome" || expected.id == "dwarf")
        {
            const std::string giantCondition = "Contro creature del sottotipo gigante, se si conserva il bonus di Destrezza alla CA";
            assertContextTotal("armorClass.normal", giantCondition, sizeBonus + 4);
            assertContextTotal("armorClass.touch", giantCondition, sizeBonus + 4);
            assertContextTotal("armorClass.flatFooted", giantCondition, sizeBonus);
            const std::string hatredCondition = expected.id == "gnome"
                ? "Contro umanoidi del sottotipo rettile o goblinoide"
                : "Contro umanoidi del sottotipo orco o goblinoide";
            assertContextTotal("attack.melee", hatredCondition, sizeBonus + 1);
            assertContextTotal("attack.ranged", hatredCondition, sizeBonus + 1);
        }
        if (expected.id == "gnome")
        {
            assertContextTotal("savingThrow.will", "Contro incantesimi ed effetti di illusione", 2);
            assert(resourceManager.modifierTotal("skill.craft.alchemy") == 0);
        }
        if (expected.id == "halfling")
        {
            assert(resourceManager.modifierTotal("skill.acrobatics") == 2);
            assert(resourceManager.modifierTotal("skill.climb") == 2);
            assertContextTotal("savingThrow.will", "Contro effetti di paura", 3);
        }
        if (expected.id == "halfElf" || expected.id == "halfOrc")
        {
            assert(elementView(race.toView(), std::string(expected.id) + ".abilityScores").missingChoices.size() == 1);
            race.setChoice(std::string(expected.id) + ".abilityScores", "ability", {"strength"});
            assert(resourceManager.targetValue("str") == 12);
        }
        if (expected.id == "halfOrc")
        {
            assert(resourceManager.modifierTotal("skill.intimidate") == 2);
        }
        if (expected.id == "dwarf")
        {
            const ModifierSetView hardy = resourceManager.modifierSetView("savingThrow.all", {"Contro veleni, incantesimi o capacità magiche"});
            assert(hardy.total == 2);
            assert(hardy.modifiers.size() == 1);
            assertContextTotal("combatManeuver.defense.bullRush", "Quando si è saldi sul terreno", 4);
            assertContextTotal("combatManeuver.defense.trip", "Quando si è saldi sul terreno", 4);
            assertContextTotal("combatManeuver.defense.grapple", "Quando si è saldi sul terreno", 0);
            assertContextTotal("skill.appraise", "Per valutare beni non magici contenenti metalli preziosi o gemme", 2);
            assertContextTotal("skill.perception", "Per individuare opere insolite in muratura", 2);
        }

        const RaceSaveData coreSave = race.toSaveData();
        race.clearRace();
        for (std::string_view target : abilityTargets)
        {
            assert(resourceManager.targetValue(target) == 10);
        }
        for (std::string_view resource : {"attack.melee", "attack.ranged", "armorClass.normal", "skill.stealth", "skill.perception", "skill.intimidate", "skill.acrobatics", "skill.climb", "savingThrow.all"})
        {
            assert(resourceManager.modifierSetView(resource).modifiers.empty());
        }
        assert(movement.toView().grants.empty());
        languages.refreshReminders();
        assert(reminderManager.toView().messages.empty());
        assert(!sizeManager.toView().base.has_value());
        race.load(coreSave);
        assert(race.toView().race->id == expected.id);
        assert(resourceManager.targetValue("str") == (expected.id == "halfElf" || expected.id == "halfOrc" ? 12 : expected.abilityScores[0]));
    }

    return 0;
}
