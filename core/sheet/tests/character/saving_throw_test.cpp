#include "golarion/character/ability.hpp"
#include "golarion/character/saving_throw.hpp"
#include "golarion/resource/contribution.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/saving_throw_view.hpp"

#include <cassert>

int main()
{
    using namespace golarion;

    ResourceManager manager;
    AbilityScore constitution(AbilityType::Constitution, 14);
    AbilityScore charisma(AbilityType::Charisma, 18);
    constitution.registerResources(manager);
    charisma.registerResources(manager);
    manager.registerEnhanceableResource("savingThrow.all");

    SavingThrow fortitude(SavingThrowType::Fortitude);
    fortitude.registerResources(manager, {"savingThrow.all"});
    manager.addContribution(baseResourceName(SavingThrowType::Fortitude), Contribution("fighter.fortitude", "2"));
    manager.addModifier("savingThrow.fortitude", Modifier(ModifierType::Bonus, "Mantello", "Bonus alla Tempra", BonusType::Resistance, "2"));
    assert(fortitude.totalValue(manager) == 6);

    manager.addModifier("savingThrow.fortitude", Modifier(ModifierType::Bonus, "Sostituzione", "Carisma contro la magia", BonusType::Circumstance, "-@conMod + @chaMod", "Contro la magia"));
    SavingThrowView view = fortitude.toView(manager);
    assert(view.type == SavingThrowType::Fortitude);
    assert(view.baseValue == 2);
    assert(view.baseContributions.total == 2);
    assert(view.baseContributions.contributions.size() == 1);
    assert(view.abilityOptions.size() == 1);
    assert(!view.abilityOptions[0].replacementId.has_value());
    assert(view.abilityOptions[0].abilityType == AbilityType::Constitution);
    assert(view.abilityOptions[0].abilityModifier == 2);
    assert(view.abilityOptions[0].totalValue == 6);
    assert(view.modifiers.conditionalTotals.size() == 1);
    assert(view.modifiers.conditionalTotals[0].value == 2);

    assert(displayName(SavingThrowType::Reflex) == "Riflessi");
    assert(resourceName(SavingThrowType::Will) == "savingThrow.will");
    assert(baseResourceName(SavingThrowType::Will) == "savingThrow.will.base");
    assert(defaultAbility(SavingThrowType::Will) == AbilityType::Wisdom);

    return 0;
}
