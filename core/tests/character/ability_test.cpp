#include "golarion/character/ability.hpp"
#include "golarion/data/ability_save_data.hpp"
#include "golarion/view/ability_view.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"

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
    AbilityScore strength(AbilityType::Strength);
    strength.registerResources(manager);
    manager.addModifier("str", Modifier(ModifierType::Bonus, "Cintura", "Bonus alla Forza", BonusType::Enhancement, "4"));

    assert(strength.baseValue() == 10);
    assert(strength.totalValue(manager) == 14);
    assert(strength.modifier(manager) == 2);
    assert(manager.targetValue("str") == 14);
    assert(manager.targetValue("strMod") == 2);

    AbilityView view = strength.toView(manager);
    assert(view.type == AbilityType::Strength);
    assert(view.baseValue == 10);
    assert(view.totalValue == 14);
    assert(view.modifier == 2);
    assert(view.modifiers.total == 4);
    assert(view.modifiers.modifiers.size() == 1);

    AbilitySaveData saveData = strength.toSaveData();
    assert(saveData.type == AbilityType::Strength);
    assert(saveData.baseValue == 10);

    AbilityScore lowStrength(AbilityType::Strength, 9);
    ResourceManager lowStrengthManager;
    lowStrength.registerResources(lowStrengthManager);
    assert(lowStrength.modifier(lowStrengthManager) == -1);

    assert(throwsInvalidArgument([]
    {
        AbilityScore invalid(AbilityType::Strength, 0);
    }));

    assert(displayName(AbilityType::Strength) == "Forza");
    assert(resourceName(AbilityType::Charisma) == "cha");

    return 0;
}
