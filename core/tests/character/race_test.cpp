#include "golarion/character/ability.hpp"
#include "golarion/character/armor_class.hpp"
#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/combat_maneuvers.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/race.hpp"
#include "golarion/character/reminder.hpp"
#include "golarion/character/size.hpp"
#include "golarion/character/skills.hpp"
#include "golarion/character/strike.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/carrying_capacity_view.hpp"
#include "golarion/view/movement_view.hpp"
#include "golarion/view/size_view.hpp"

#include <array>
#include <cassert>
#include <ranges>
#include <stdexcept>
#include <string>

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
    ReminderManager reminderManager(resourceManager);
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
    assert(view.activeElements.size() == 7);
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
    assert(reminderManager.toView().messages.size() == 4);
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
    assert(view.activeElements.size() == 5);
    assert(std::ranges::find(view.activeElements, "human.abilityScores", &RacialElementView::id) == view.activeElements.end());
    assert(std::ranges::find(view.activeElements, "human.skilled", &RacialElementView::id) == view.activeElements.end());
    assert(std::ranges::find(view.activeElements, "human.bonusFeat", &RacialElementView::id) == view.activeElements.end());
    const RacialElementView &missingDualTalent = elementView(view, "human.dualTalent");
    assert(missingDualTalent.missingChoices.size() == 1);
    assert(missingDualTalent.missingChoices[0].selectionCount == 2);
    assert(resourceManager.modifierTotal("str") == 0);
    assert(reminderManager.toView().messages.size() == 2);
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
    assert(reminderManager.toView().messages.size() == 4);
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

    return 0;
}
