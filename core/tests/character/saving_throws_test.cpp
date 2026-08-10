#include "golarion/character/ability.hpp"
#include "golarion/character/saving_throws.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <array>
#include <cassert>

int main()
{
    using namespace golarion;

    ResourceManager manager;
    std::array abilities{
        AbilityScore(AbilityType::Strength, 10),
        AbilityScore(AbilityType::Dexterity, 14),
        AbilityScore(AbilityType::Constitution, 12),
        AbilityScore(AbilityType::Intelligence, 10),
        AbilityScore(AbilityType::Wisdom, 8),
        AbilityScore(AbilityType::Charisma, 16)
    };
    for (const AbilityScore &ability : abilities)
    {
        ability.registerResources(manager);
    }

    SavingThrows savingThrows(manager);
    savingThrows.setBaseValue(SavingThrowType::Reflex, 1);
    manager.addModifier("savingThrow.all", Modifier(ModifierType::Bonus, "Mantello", "Bonus ai tiri salvezza", BonusType::Resistance, "1"));

    SavingThrowsView view = savingThrows.toView();
    assert(view.savingThrows.size() == 3);
    assert(view.savingThrows[0].totalValue == 2);
    assert(view.savingThrows[1].totalValue == 4);
    assert(view.savingThrows[2].totalValue == 0);

    savingThrows.setAbilityType(SavingThrowType::Reflex, AbilityType::Charisma);
    view = savingThrows.toView();
    assert(view.savingThrows[1].totalValue == 5);

    const SavingThrowsSaveData data = savingThrows.toSaveData();
    assert(data.savingThrows.size() == 3);
    assert(data.savingThrows[1].type == SavingThrowType::Reflex);
    assert(data.savingThrows[1].baseValue == 1);
    assert(data.savingThrows[1].abilityType == AbilityType::Charisma);

    return 0;
}
