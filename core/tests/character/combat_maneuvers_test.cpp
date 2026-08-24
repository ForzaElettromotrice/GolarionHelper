#include "golarion/character/ability.hpp"
#include "golarion/character/strike.hpp"
#include "golarion/character/base_attack_bonus.hpp"
#include "golarion/character/combat_maneuvers.hpp"
#include "golarion/resource/contribution.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/combat_maneuvers_view.hpp"

#include <array>
#include <cassert>
#include <stdexcept>

namespace
{
    template<typename Function>
    bool throwsInvalidArgument(Function function)
    {
        try
        {
            function();
            return false;
        }
        catch (const std::invalid_argument &)
        {
            return true;
        }
    }
}

int main()
{
    using namespace golarion;

    ResourceManager resourceManager;
    BaseAttackBonus baseAttackBonus(resourceManager);
    std::array abilities{
        AbilityScore(AbilityType::Strength, 16),
        AbilityScore(AbilityType::Dexterity, 14),
        AbilityScore(AbilityType::Constitution),
        AbilityScore(AbilityType::Intelligence),
        AbilityScore(AbilityType::Wisdom),
        AbilityScore(AbilityType::Charisma)
    };
    for (const AbilityScore &ability : abilities)
    {
        ability.registerResources(resourceManager);
    }
    Strikes attacks(resourceManager);
    CombatManeuvers combatManeuvers(resourceManager);

    resourceManager.addContribution(BaseAttackBonusResource, Contribution("fighter", "5"));
    resourceManager.addModifier("attack.all", Modifier(ModifierType::Bonus, "Competenza", "Bonus a tutti gli attacchi", BonusType::Generic, "1"));
    resourceManager.addModifier(CombatManeuverBonusAllResource, Modifier(ModifierType::Bonus, "Addestramento", "Bonus a tutte le manovre", BonusType::Generic, "2"));
    resourceManager.addModifier(CombatManeuverBonusAllResource, Modifier(ModifierType::Bonus, "Taglia", "Modificatore di taglia al BMC", BonusType::Size, "1"));
    resourceManager.addModifier(combatManeuverBonusResourceName(CombatManeuverType::Trip), Modifier(ModifierType::Bonus, "Sbilanciare Migliorato", "Bonus a Sbilanciare", BonusType::Generic, "3"));
    resourceManager.addModifier(CombatManeuverDefenseAllResource, Modifier(ModifierType::Bonus, "Difesa", "Bonus alla DMC", BonusType::Generic, "2"));
    resourceManager.addModifier(CombatManeuverDefenseAllResource, Modifier(ModifierType::Bonus, "Taglia", "Modificatore di taglia alla DMC", BonusType::Size, "1"));
    resourceManager.addModifier(combatManeuverDefenseResourceName(CombatManeuverType::Trip), Modifier(ModifierType::Bonus, "Stabilità", "Bonus contro Sbilanciare", BonusType::Generic, "3"));

    CombatManeuversView view = combatManeuvers.toView();
    assert(view.baseAttackBonus == 5);
    assert(view.maneuvers.size() == 10);
    assert(view.maneuvers[0].type == CombatManeuverType::BullRush);
    assert(view.maneuvers[0].bonus.abilityOptions[0].totalValue == 12);
    assert(view.maneuvers[0].defense.dexterityModifier == 2);
    assert(view.maneuvers[0].defense.appliedDexterityModifier == 2);
    assert(!view.maneuvers[0].defense.dexterityBonusSuppressed);
    assert(view.maneuvers[0].defense.dexteritySuppressions.empty());
    assert(view.maneuvers[0].defense.totalValue == 23);
    assert(view.maneuvers[9].type == CombatManeuverType::Trip);
    assert(view.maneuvers[9].bonus.modifiers.total == 7);
    assert(view.maneuvers[9].bonus.abilityOptions[0].totalValue == 15);
    assert(view.maneuvers[9].defense.totalValue == 26);

    resourceManager.addToCollection(CombatManeuverAbilityReplacementsResource, CombatManeuverAbilityReplacement(CombatManeuverAbilityReplacementDefinition{
        .id = "agileManeuvers",
        .source = "Manovre Agili",
        .targetResourceName = std::string(CombatManeuverBonusAllResource),
        .abilityType = AbilityType::Dexterity
    }));
    view = combatManeuvers.toView();
    assert(view.maneuvers[0].bonus.abilityOptions.size() == 2);
    assert(view.maneuvers[0].bonus.abilityOptions[1].replacementId == "agileManeuvers");
    assert(view.maneuvers[0].bonus.abilityOptions[1].abilityType == AbilityType::Dexterity);
    assert(view.maneuvers[0].bonus.abilityOptions[1].totalValue == 11);
    assert(view.maneuvers[9].bonus.abilityOptions[1].totalValue == 14);

    resourceManager.addToCollection(CombatManeuverAbilityReplacementsResource, CombatManeuverAbilityReplacement(CombatManeuverAbilityReplacementDefinition{
        .id = "tripTraining",
        .source = "Addestramento a Sbilanciare",
        .targetResourceName = combatManeuverBonusResourceName(CombatManeuverType::Trip),
        .abilityType = AbilityType::Wisdom
    }));
    view = combatManeuvers.toView();
    assert(view.maneuvers[0].bonus.abilityOptions.size() == 2);
    assert(view.maneuvers[9].bonus.abilityOptions.size() == 3);

    resourceManager.addToCollection(CombatManeuverDefenseDexteritySuppressionsResource, CombatManeuverDefenseDexteritySuppression(CombatManeuverDefenseDexteritySuppressionDefinition{
        .id = " flatFooted ",
        .source = " Impreparato "
    }));
    view = combatManeuvers.toView();
    assert(view.maneuvers[0].defense.dexterityModifier == 2);
    assert(view.maneuvers[0].defense.appliedDexterityModifier == 0);
    assert(view.maneuvers[0].defense.dexterityBonusSuppressed);
    assert(view.maneuvers[0].defense.dexteritySuppressions.size() == 1);
    assert(view.maneuvers[0].defense.dexteritySuppressions[0].id == "flatFooted");
    assert(view.maneuvers[0].defense.dexteritySuppressions[0].source == "Impreparato");
    assert(view.maneuvers[0].defense.totalValue == 21);
    assert(view.maneuvers[9].defense.totalValue == 24);
    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(CombatManeuverDefenseDexteritySuppressionsResource, CombatManeuverDefenseDexteritySuppression(CombatManeuverDefenseDexteritySuppressionDefinition{
            .id = "flatFooted",
            .source = "Duplicata"
        }));
    }));
    resourceManager.removeFromCollection(CombatManeuverDefenseDexteritySuppressionsResource, "flatFooted");
    assert(!combatManeuvers.toView().maneuvers[0].defense.dexterityBonusSuppressed);
    assert(combatManeuvers.toView().maneuvers[0].defense.totalValue == 23);
    assert(throwsInvalidArgument([&]
    {
        resourceManager.removeFromCollection(CombatManeuverDefenseDexteritySuppressionsResource, "flatFooted");
    }));
    assert(throwsInvalidArgument([&]
    {
        resourceManager.addToCollection(CombatManeuverAbilityReplacementsResource, CombatManeuverAbilityReplacement(CombatManeuverAbilityReplacementDefinition{
            .id = "invalid",
            .source = "Errore",
            .targetResourceName = "attack.all",
            .abilityType = AbilityType::Dexterity
        }));
    }));

    resourceManager.removeFromCollection(CombatManeuverAbilityReplacementsResource, "agileManeuvers");
    assert(combatManeuvers.toView().maneuvers[0].bonus.abilityOptions.size() == 1);
    assert(displayName(CombatManeuverType::Grapple) == "Lottare");

    return 0;
}
