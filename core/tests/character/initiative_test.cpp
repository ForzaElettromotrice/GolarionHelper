#include "golarion/character/ability.hpp"
#include "golarion/character/initiative.hpp"
#include "golarion/data/initiative_save_data.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/initiative_view.hpp"

#include <cassert>

int main()
{
    using namespace golarion;

    ResourceManager manager;
    AbilityScore dexterity(AbilityType::Dexterity, 14);
    AbilityScore charisma(AbilityType::Charisma, 18);
    dexterity.registerResources(manager);
    charisma.registerResources(manager);

    Initiative initiative(manager);
    manager.addModifier("initiative", Modifier(ModifierType::Bonus, "Test", "Bonus all'iniziativa", BonusType::Competence, "4"));

    InitiativeView view = initiative.toView();
    assert(view.abilityType == AbilityType::Dexterity);
    assert(view.abilityModifier == 2);
    assert(view.totalValue == 6);
    assert(view.modifiers.total == 4);

    initiative.setAbilityType(AbilityType::Charisma);
    view = initiative.toView();
    assert(view.abilityType == AbilityType::Charisma);
    assert(view.abilityModifier == 4);
    assert(view.totalValue == 8);

    const InitiativeSaveData data = initiative.toSaveData();
    assert(data.abilityType == AbilityType::Charisma);

    return 0;
}
