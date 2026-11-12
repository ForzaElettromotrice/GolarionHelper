#include "golarion/resource/modifier.hpp"
#include "golarion/resource/requirement.hpp"
#include "golarion/view/modifier_view.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <stdexcept>
#include <string>
#include <vector>

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

    ModifierDefinition definition{.type = ModifierType::Bonus, .source = "Fonte", .description = "Bonus", .bonusType = BonusType::Generic, .expression = "3"};
    const Modifier first(definition);
    const Modifier second(definition);
    definition.expression = "9";
    assert(first.expression() == "3" && second.expression() == "3");
    assert(first.id() != second.id());

    Modifier bonus({.type = ModifierType::Bonus, .source = "  Cintura della forza  ", .description = "  Bonus alla Forza  ", .bonusType = BonusType::Enhancement, .expression = "  2 + @level  ", .condition = "  contro il veleno  "});

    assert(bonus.id().size() == 36);
    assert(bonus.type() == ModifierType::Bonus);
    assert(bonus.source() == "Cintura della forza");
    assert(bonus.description() == "Bonus alla Forza");
    assert(bonus.bonusType() == BonusType::Enhancement);
    assert(bonus.expression() == "2 + @level");
    assert(bonus.condition() == "contro il veleno");

    Modifier penalty({.type = ModifierType::Penalty, .source = "Veleno", .description = "Debilitato", .bonusType = std::nullopt, .expression = "2"});
    assert(!penalty.bonusType().has_value());
    assert(!penalty.condition().has_value());
    assert(penalty.id() != bonus.id());

    assert(throwsInvalidArgument([]
    {
        Modifier invalid({.type = ModifierType::Bonus, .source = "Sorgente", .description = "Descrizione", .bonusType = std::nullopt, .expression = "2"});
    }));
    assert(throwsInvalidArgument([]
    {
        Modifier invalid({.type = ModifierType::Penalty, .source = "Sorgente", .description = "Descrizione", .bonusType = BonusType::Luck, .expression = "2"});
    }));
    assert(throwsInvalidArgument([]
    {
        Modifier invalid({.type = ModifierType::Penalty, .source = " ", .description = "Descrizione", .bonusType = std::nullopt, .expression = "2"});
    }));

    assert(displayName(ModifierType::Penalty) == "Penalità");
    assert(displayName(BonusType::Generic) == "Generico");
    assert(displayName(BonusType::Enhancement) == "Potenziamento");
    assert(displayName(StackingRule::HighestOnly) == "Solo il più alto");
    assert(stackingRule(BonusType::Dodge) == StackingRule::Stacks);
    assert(stackingRule(BonusType::Circumstance) == StackingRule::StacksUnlessSameSource);
    assert(stackingRule(BonusType::Generic) == StackingRule::Stacks);
    assert(stackingRule(BonusType::Luck) == StackingRule::HighestOnly);

    ResourceManager manager;
    manager.registerTarget("level", []
    {
        return 3;
    });
    ModifierView view = Modifier({.type = ModifierType::Bonus, .source = "Capacità", .description = "Bonus variabile", .bonusType = BonusType::Racial, .expression = "@level + 1"}).toView(manager);
    assert(view.type == ModifierType::Bonus);
    assert(view.source == "Capacità");
    assert(view.bonusType == BonusType::Racial);
    assert(view.expression == "@level + 1");
    assert(view.resolvedValue == 4);
    assert(view.active);
    assert(view.requirements.empty());

    manager.registerTarget("armor.worn", []
    {
        return 1;
    });
    Modifier restricted({.type = ModifierType::Bonus, .source = "Monaco", .description = "Bonus alla CA", .bonusType = BonusType::Dodge, .expression = "@level", .condition = std::nullopt, .requirements = std::vector<Requirement>{Requirement("@armor.worn == 0", "Non applicabile mentre indossi un'armatura")}});
    const ModifierView restrictedView = restricted.toView(manager);
    assert(!restrictedView.active);
    assert(restrictedView.resolvedValue == 3);
    assert(restrictedView.requirements.size() == 1);
    assert(!restrictedView.requirements[0].satisfied);

    assert(restricted.requirements().size() == 1);
    assert(restricted.requirements()[0].expression() == "@armor.worn == 0");

    return 0;
}
