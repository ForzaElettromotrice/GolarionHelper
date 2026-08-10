#include "golarion/resource/modifier.hpp"
#include "golarion/data/modifier_save_data.hpp"
#include "golarion/view/modifier_view.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <stdexcept>
#include <string>

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

    Modifier bonus(
        ModifierType::Bonus,
        "  Cintura della forza  ",
        "  Bonus alla Forza  ",
        BonusType::Enhancement,
        "  2 + @level  ",
        "  contro il veleno  "
    );

    assert(bonus.id().size() == 36);
    assert(bonus.type() == ModifierType::Bonus);
    assert(bonus.source() == "Cintura della forza");
    assert(bonus.description() == "Bonus alla Forza");
    assert(bonus.bonusType() == BonusType::Enhancement);
    assert(bonus.expression() == "2 + @level");
    assert(bonus.condition() == "contro il veleno");

    Modifier penalty(ModifierType::Penalty, "Veleno", "Debilitato", std::nullopt, "2");
    assert(!penalty.bonusType().has_value());
    assert(!penalty.condition().has_value());
    assert(penalty.id() != bonus.id());

    ModifierSaveData saveData = bonus.toSaveData();
    assert(saveData.id == bonus.id());
    assert(saveData.type == ModifierType::Bonus);
    assert(saveData.source == "Cintura della forza");
    assert(saveData.description == "Bonus alla Forza");
    assert(saveData.bonusType == BonusType::Enhancement);
    assert(saveData.expression == "2 + @level");
    assert(saveData.condition == "contro il veleno");

    assert(throwsInvalidArgument([]
    {
        Modifier invalid(ModifierType::Bonus, "Sorgente", "Descrizione", std::nullopt, "2");
    }));
    assert(throwsInvalidArgument([]
    {
        Modifier invalid(ModifierType::Penalty, "Sorgente", "Descrizione", BonusType::Luck, "2");
    }));
    assert(throwsInvalidArgument([]
    {
        Modifier invalid(ModifierType::Penalty, " ", "Descrizione", std::nullopt, "2");
    }));

    assert(displayName(ModifierType::Penalty) == "Penalità");
    assert(displayName(BonusType::Enhancement) == "Potenziamento");
    assert(displayName(StackingRule::HighestOnly) == "Solo il più alto");
    assert(stackingRule(BonusType::Dodge) == StackingRule::Stacks);
    assert(stackingRule(BonusType::Circumstance) == StackingRule::StacksUnlessSameSource);
    assert(stackingRule(BonusType::Luck) == StackingRule::HighestOnly);

    ResourceManager manager;
    manager.registerTarget("level", []
    {
        return 3;
    });
    ModifierView view = Modifier(ModifierType::Bonus, "Capacità", "Bonus variabile", BonusType::Racial, "@level + 1").toView(manager);
    assert(view.type == ModifierType::Bonus);
    assert(view.source == "Capacità");
    assert(view.bonusType == BonusType::Racial);
    assert(view.expression == "@level + 1");
    assert(view.resolvedValue == 4);

    return 0;
}
