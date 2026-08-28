#include "golarion/character/ability.hpp"
#include "golarion/character/initiative.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/initiative_view.hpp"

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

    ResourceManager manager;
    AbilityChecks abilityChecks(manager);
    AbilityScore dexterity(AbilityType::Dexterity, 14);
    AbilityScore charisma(AbilityType::Charisma, 18);
    dexterity.registerResources(manager);
    charisma.registerResources(manager);

    Initiative initiative(manager);
    manager.addModifier("initiative", Modifier(ModifierType::Bonus, "Test", "Bonus all'iniziativa", BonusType::Competence, "4"));
    manager.addModifier(AbilityCheckRootResource, Modifier(ModifierType::Penalty, "Infermo", "Penalità alle prove di caratteristica", std::nullopt, "2"));
    manager.addModifier(abilityCheckResourceName(AbilityType::Dexterity), Modifier(ModifierType::Penalty, "Test", "Penalità alle prove di Destrezza", std::nullopt, "3"));
    manager.addModifier(abilityCheckResourceName(AbilityType::Charisma), Modifier(ModifierType::Bonus, "Test", "Bonus alle prove di Carisma", BonusType::Insight, "1"));

    InitiativeView view = initiative.toView();
    assert(view.abilityOptions.size() == 1);
    assert(!view.abilityOptions[0].replacementId.has_value());
    assert(view.abilityOptions[0].abilityType == AbilityType::Dexterity);
    assert(view.abilityOptions[0].abilityModifier == 2);
    assert(view.abilityOptions[0].totalValue == 1);
    assert(view.abilityOptions[0].modifiers.total == -1);
    assert(view.modifiers.total == 2);
    assert(manager.enhanceableResourceIsOrInheritsFrom(InitiativeResource, AbilityCheckRootResource));

    manager.addToCollection(InitiativeAbilityReplacementsResource, InitiativeAbilityReplacement(InitiativeAbilityReplacementDefinition{
        .id = "nobleScion",
        .source = "Rampollo nobile",
        .abilityType = AbilityType::Charisma
    }));
    view = initiative.toView();
    assert(view.abilityOptions.size() == 2);
    assert(view.abilityOptions[1].replacementId == "nobleScion");
    assert(view.abilityOptions[1].source == "Rampollo nobile");
    assert(view.abilityOptions[1].abilityType == AbilityType::Charisma);
    assert(view.abilityOptions[1].abilityModifier == 4);
    assert(view.abilityOptions[1].totalValue == 7);
    assert(view.abilityOptions[1].modifiers.total == 3);
    assert(throwsInvalidArgument([&manager]
    {
        manager.addToCollection(InitiativeAbilityReplacementsResource, InitiativeAbilityReplacement(InitiativeAbilityReplacementDefinition{
            .id = "nobleScion",
            .source = "Duplicato",
            .abilityType = AbilityType::Wisdom
        }));
    }));
    manager.removeFromCollection(InitiativeAbilityReplacementsResource, "nobleScion");
    assert(initiative.toView().abilityOptions.size() == 1);
    assert(throwsInvalidArgument([&manager]
    {
        manager.removeFromCollection(InitiativeAbilityReplacementsResource, "nobleScion");
    }));

    return 0;
}
