#include "golarion/resource/modifier.hpp"
#include "golarion/resource/modifier_set.hpp"
#include "golarion/resource/requirement.hpp"
#include "golarion/view/modifier_set_view.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <cassert>
#include <optional>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace
{
    golarion::Modifier bonus(std::string source, golarion::BonusType bonusType, std::string expression)
    {
        return golarion::Modifier({.type = golarion::ModifierType::Bonus, .source = std::move(source), .description = "Test bonus", .bonusType = bonusType, .expression = std::move(expression)});
    }

    golarion::Modifier penalty(std::string source, std::string expression)
    {
        return golarion::Modifier({.type = golarion::ModifierType::Penalty, .source = std::move(source), .description = "Test penalty", .bonusType = std::nullopt, .expression = std::move(expression)});
    }

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
    ModifierSet emptySet;
    assert(emptySet.calculateTotal(manager) == 0);

    ModifierSet highestOnly;
    highestOnly.addModifier(bonus("Primo incantesimo", BonusType::Enhancement, "2"));
    highestOnly.addModifier(bonus("Secondo incantesimo", BonusType::Enhancement, "4"));
    highestOnly.addModifier(bonus("Discendenza", BonusType::Racial, "2"));
    assert(highestOnly.calculateTotal(manager) == 6);

    ModifierSet stacking;
    stacking.addModifier(bonus("Prima capacità", BonusType::Dodge, "1"));
    stacking.addModifier(bonus("Seconda capacità", BonusType::Dodge, "2"));
    assert(stacking.calculateTotal(manager) == 3);

    ModifierSet circumstance;
    circumstance.addModifier(bonus("Posizione sopraelevata", BonusType::Circumstance, "1"));
    circumstance.addModifier(bonus("Posizione sopraelevata", BonusType::Circumstance, "2"));
    circumstance.addModifier(bonus("Aiutare un altro", BonusType::Circumstance, "3"));
    assert(circumstance.calculateTotal(manager) == 5);

    ModifierSet penalties;
    penalties.addModifier(penalty("Scosso", "2"));
    penalties.addModifier(penalty("Affaticato", "1"));
    assert(penalties.calculateTotal(manager) == -3);

    ModifierSet inherent;
    inherent.addModifier(bonus("Desiderio", BonusType::Inherent, "8"));
    assert(inherent.calculateTotal(manager) == 5);

    ModifierSet conditional;
    conditional.addModifier(bonus("Discendenza", BonusType::Racial, "2"));
    conditional.addModifier(Modifier({.type = ModifierType::Bonus, .source = "Resistenza al veleno", .description = "Bonus contro il veleno", .bonusType = BonusType::Racial, .expression = "4", .condition = "Contro il veleno"}));
    assert(conditional.calculateTotal(manager) == 2);

    manager.registerTarget("strength", []
    {
        return 4;
    });
    ModifierSet expression;
    expression.addModifier(bonus("Capacità", BonusType::Racial, "@strength / 2"));
    assert(expression.calculateTotal(manager) == 2);

    int armorWorn = 1;
    manager.registerTarget("armor.worn", [&armorWorn]
    {
        return armorWorn;
    });
    ModifierSet requirements;
    requirements.addModifier(Modifier({.type = ModifierType::Bonus, .source = "Monaco", .description = "Bonus alla CA", .bonusType = BonusType::Dodge, .expression = "3", .condition = std::nullopt, .requirements = std::vector<Requirement>{Requirement("@armor.worn == 0", "Non applicabile mentre indossi un'armatura")}}));
    assert(requirements.calculateTotal(manager) == 0);
    ModifierSetView inactiveView = requirements.toView(manager);
    assert(inactiveView.total == 0);
    assert(!inactiveView.modifiers[0].active);
    assert(!inactiveView.modifiers[0].requirements[0].satisfied);

    armorWorn = 0;
    assert(requirements.calculateTotal(manager) == 3);
    ModifierSetView activeView = requirements.toView(manager);
    assert(activeView.total == 3);
    assert(activeView.modifiers[0].active);

    ModifierSet negative;
    negative.addModifier(penalty("Scosso", "-2"));
    assert(throwsInvalidArgument([&]
    {
        negative.calculateTotal(manager);
    }));

    ModifierSet snapshotSet;
    snapshotSet.addModifier(bonus("Oggetto permanente", BonusType::Resistance, "3"));
    snapshotSet.addModifier(Modifier({.type = ModifierType::Bonus, .source = "Protezione minore", .description = "Bonus contro il veleno", .bonusType = BonusType::Resistance, .expression = "2", .condition = "Contro il veleno"}));
    snapshotSet.addModifier(Modifier({.type = ModifierType::Bonus, .source = "Protezione maggiore", .description = "Bonus contro la paura", .bonusType = BonusType::Resistance, .expression = "5", .condition = "Contro la paura"}));

    ModifierSetView snapshot = snapshotSet.toView(manager);
    assert(snapshot.total == 3);
    assert(snapshot.modifiers.size() == 3);
    assert(snapshot.conditionalTotals.size() == 2);
    assert(snapshot.conditionalTotals[0].condition == "Contro il veleno");
    assert(snapshot.conditionalTotals[0].value == 0);
    assert(snapshot.conditionalTotals[1].condition == "Contro la paura");
    assert(snapshot.conditionalTotals[1].value == 2);

    manager.registerEnhanceableResource("conditionalCombination");
    manager.addModifier("conditionalCombination", bonus("Oggetto permanente", BonusType::Resistance, "3"));
    manager.addModifier("conditionalCombination", Modifier({.type = ModifierType::Bonus, .source = "Protezione dal veleno", .description = "Bonus contro il veleno", .bonusType = BonusType::Resistance, .expression = "4", .condition = "Contro il veleno"}));
    manager.addModifier("conditionalCombination", Modifier({.type = ModifierType::Bonus, .source = "Protezione dalla paura", .description = "Bonus contro la paura", .bonusType = BonusType::Resistance, .expression = "5", .condition = "Contro la paura"}));
    const ModifierSetView combinedSnapshot = manager.modifierSetView("conditionalCombination", {"Contro il veleno", "Contro la paura"});
    assert(combinedSnapshot.permanentTotal == 3);
    assert(combinedSnapshot.total == 5);

    int evaluations = 0;
    manager.registerTarget("dynamic", [&evaluations]
    {
        return ++evaluations;
    });
    ModifierSet evaluatedOnce;
    evaluatedOnce.addModifier(bonus("Capacità", BonusType::Racial, "@dynamic"));
    ModifierSetView evaluatedOnceView = evaluatedOnce.toView(manager);
    assert(evaluations == 1);
    assert(evaluatedOnceView.total == 1);
    assert(evaluatedOnceView.modifiers[0].resolvedValue == 1);

    return 0;
}
