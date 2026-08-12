#include "golarion/character/ability.hpp"
#include "golarion/character/saving_throws.hpp"
#include "golarion/resource/contribution.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"

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
    manager.addContribution(baseResourceName(SavingThrowType::Reflex), Contribution("rogue.reflex", "1"));
    manager.addModifier("savingThrow.all", Modifier(ModifierType::Bonus, "Mantello", "Bonus ai tiri salvezza", BonusType::Resistance, "1"));

    SavingThrowsView view = savingThrows.toView();
    assert(view.savingThrows.size() == 3);
    assert(view.savingThrows[0].abilityOptions[0].totalValue == 2);
    assert(view.savingThrows[1].abilityOptions[0].totalValue == 4);
    assert(view.savingThrows[2].abilityOptions[0].totalValue == 0);

    manager.addToCollection(SavingThrowAbilityReplacementsResource, SavingThrowAbilityReplacement(SavingThrowAbilityReplacementDefinition{
        .id = "divineGrace.reflex",
        .source = "Grazia divina",
        .savingThrowType = SavingThrowType::Reflex,
        .abilityType = AbilityType::Charisma
    }));
    view = savingThrows.toView();
    assert(view.savingThrows[0].abilityOptions.size() == 1);
    assert(view.savingThrows[1].abilityOptions.size() == 2);
    assert(view.savingThrows[1].abilityOptions[1].replacementId == "divineGrace.reflex");
    assert(view.savingThrows[1].abilityOptions[1].source == "Grazia divina");
    assert(view.savingThrows[1].abilityOptions[1].abilityType == AbilityType::Charisma);
    assert(view.savingThrows[1].abilityOptions[1].totalValue == 5);
    assert(throwsInvalidArgument([&manager]
    {
        manager.addToCollection(SavingThrowAbilityReplacementsResource, SavingThrowAbilityReplacement(SavingThrowAbilityReplacementDefinition{
            .id = "divineGrace.reflex",
            .source = "Duplicato",
            .savingThrowType = SavingThrowType::Will,
            .abilityType = AbilityType::Charisma
        }));
    }));
    manager.removeFromCollection(SavingThrowAbilityReplacementsResource, "divineGrace.reflex");
    assert(savingThrows.toView().savingThrows[1].abilityOptions.size() == 1);

    return 0;
}
