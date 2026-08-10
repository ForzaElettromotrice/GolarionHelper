#include "golarion/character/ability.hpp"
#include "golarion/character/saving_throw.hpp"
#include "golarion/data/saving_throw_save_data.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/saving_throw_view.hpp"

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
    AbilityScore constitution(AbilityType::Constitution, 14);
    AbilityScore charisma(AbilityType::Charisma, 18);
    constitution.registerResources(manager);
    charisma.registerResources(manager);
    manager.registerEnhanceableResource("savingThrow.all");

    SavingThrow fortitude(SavingThrowType::Fortitude);
    fortitude.registerResources(manager, {"savingThrow.all"});
    fortitude.setBaseValue(2);
    manager.addModifier("savingThrow.fortitude", Modifier(ModifierType::Bonus, "Mantello", "Bonus alla Tempra", BonusType::Resistance, "2"));
    assert(fortitude.totalValue(manager) == 6);

    manager.addModifier("savingThrow.fortitude", Modifier(ModifierType::Bonus, "Sostituzione", "Carisma contro la magia", BonusType::Circumstance, "-@conMod + @chaMod", "Contro la magia"));
    SavingThrowView view = fortitude.toView(manager);
    assert(view.type == SavingThrowType::Fortitude);
    assert(view.baseValue == 2);
    assert(view.abilityType == AbilityType::Constitution);
    assert(view.abilityModifier == 2);
    assert(view.totalValue == 6);
    assert(view.modifiers.conditionalTotals.size() == 1);
    assert(view.modifiers.conditionalTotals[0].value == 2);

    fortitude.setAbilityType(AbilityType::Charisma);
    assert(fortitude.totalValue(manager) == 8);

    const SavingThrowSaveData data = fortitude.toSaveData();
    assert(data.type == SavingThrowType::Fortitude);
    assert(data.baseValue == 2);
    assert(data.abilityType == AbilityType::Charisma);

    assert(throwsInvalidArgument([&]
    {
        fortitude.setBaseValue(-1);
    }));
    assert(displayName(SavingThrowType::Reflex) == "Riflessi");
    assert(resourceName(SavingThrowType::Will) == "savingThrow.will");
    assert(defaultAbility(SavingThrowType::Will) == AbilityType::Wisdom);

    return 0;
}
