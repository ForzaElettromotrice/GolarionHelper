#include "golarion/character/movement.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/movement_view.hpp"

#include <algorithm>
#include <cassert>
#include <stdexcept>
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

    ResourceManager manager;
    Movement movement(manager);
    manager.addToCollection(MovementGrantsResource, MovementGrant(MovementGrantDefinition{
        .id = "naturalWings",
        .source = "Ali naturali",
        .type = MovementType::Fly,
        .baseSpeedExpression = "8",
        .maneuverability = Maneuverability::Poor,
        .affectedByArmor = true,
        .affectedByLoad = true,
        .supportsRunning = true
    }));
    manager.addToCollection(MovementGrantsResource, MovementGrant(MovementGrantDefinition{
        .id = "spellFly",
        .source = "Volare",
        .type = MovementType::Fly,
        .baseSpeedExpression = "12",
        .maneuverability = Maneuverability::Good,
        .affectedByArmor = true,
        .affectedByLoad = true,
        .supportsRunning = true
    }));

    manager.addModifier("speed.fly", Modifier(ModifierType::Bonus, "Velocità", "Bonus al volo", BonusType::Enhancement, "2"));
    Modifier wingsModifier(ModifierType::Bonus, "Ali sviluppate", "Bonus alle ali", BonusType::Racial, "2");
    const std::string wingsModifierId = wingsModifier.id();
    manager.addModifier("speed.fly.naturalWings", std::move(wingsModifier));
    manager.addToCollection(MovementAdjustmentsResource, MovementAdjustment(MovementAdjustmentDefinition{
        .id = "exhausted",
        .source = "Esausto",
        .description = "Velocità dimezzata",
        .type = MovementAdjustmentType::SpeedMultiplier,
        .selector = MovementSelector{.type = std::nullopt, .grantId = std::nullopt},
        .expression = "50",
        .condition = std::nullopt
    }));
    manager.addToCollection(MovementAdjustmentsResource, MovementAdjustment(MovementAdjustmentDefinition{
        .id = "trainedWings",
        .source = "Addestramento aereo",
        .description = "Manovrabilità migliorata",
        .type = MovementAdjustmentType::ManeuverabilityChange,
        .selector = MovementSelector{.type = MovementType::Fly, .grantId = "naturalWings"},
        .expression = "1",
        .condition = std::nullopt
    }));
    manager.addToCollection(MovementAdjustmentsResource, MovementAdjustment(MovementAdjustmentDefinition{
        .id = "situationalBlock",
        .source = "Vento forte",
        .description = "Volo non utilizzabile",
        .type = MovementAdjustmentType::Block,
        .selector = MovementSelector{.type = MovementType::Fly, .grantId = std::nullopt},
        .expression = std::nullopt,
        .condition = "Durante una tempesta"
    }));

    MovementView view = movement.toView();
    assert(view.grants.size() == 2);
    assert(view.grants[0].id == "naturalWings");
    assert(view.grants[0].baseUnits == 8);
    assert(view.grants[0].modifiedBaseUnits == 12);
    assert(view.grants[0].effectiveUnits == 6);
    assert(view.grants[0].maneuverability == Maneuverability::Average);
    assert(view.grants[0].usable);
    assert(view.grants[0].run.supported);
    assert(view.grants[0].run.usable);
    assert(view.grants[0].run.baseMultiplier == 4);
    assert(view.grants[0].run.effectiveMultiplier == 4);
    assert(view.grants[0].run.distanceUnits == 24);
    assert(view.grants[0].modifiers.total == 4);
    assert(view.grants[0].adjustments.size() == 3);
    assert(view.grants[1].id == "spellFly");
    assert(view.grants[1].modifiedBaseUnits == 14);
    assert(view.grants[1].effectiveUnits == 7);
    assert(view.grants[1].maneuverability == Maneuverability::Good);
    assert(view.grants[1].run.effectiveMultiplier == 4);
    assert(view.grants[1].run.distanceUnits == 28);
    assert(view.grants[1].adjustments.size() == 2);

    assert(throwsInvalidArgument([&]
    {
        manager.removeFromCollection(MovementGrantsResource, "naturalWings");
    }));
    manager.removeFromCollection(MovementAdjustmentsResource, "trainedWings");
    assert(throwsInvalidArgument([&]
    {
        manager.removeFromCollection(MovementGrantsResource, "naturalWings");
    }));
    manager.removeModifier("speed.fly.naturalWings", wingsModifierId);
    manager.removeFromCollection(MovementGrantsResource, "naturalWings");
    view = movement.toView();
    assert(view.grants.size() == 1);

    manager.addToCollection(MovementAdjustmentsResource, MovementAdjustment(MovementAdjustmentDefinition{
        .id = "loadReduction",
        .source = "Carico",
        .description = "Velocità ridotta dal carico",
        .type = MovementAdjustmentType::ReducedByArmorOrLoad,
        .selector = MovementSelector{.type = std::nullopt, .grantId = std::nullopt, .affectedByArmorOnly = false, .affectedByLoadOnly = true},
        .expression = std::nullopt,
        .condition = std::nullopt
    }));
    manager.addToCollection(MovementAdjustmentsResource, MovementAdjustment(MovementAdjustmentDefinition{
        .id = "armorReduction",
        .source = "Armatura",
        .description = "Velocità ridotta dall'armatura",
        .type = MovementAdjustmentType::ReducedByArmorOrLoad,
        .selector = MovementSelector{.type = std::nullopt, .grantId = std::nullopt, .affectedByArmorOnly = true, .affectedByLoadOnly = false},
        .expression = std::nullopt,
        .condition = std::nullopt
    }));
    view = movement.toView();
    assert(view.grants[0].effectiveUnits == 5);
    manager.removeFromCollection(MovementAdjustmentsResource, "loadReduction");
    manager.removeFromCollection(MovementAdjustmentsResource, "armorReduction");

    manager.addToCollection(RunAdjustmentsResource, RunAdjustment(RunAdjustmentDefinition{
        .id = "feat.run",
        .source = "Correre",
        .description = "Moltiplicatore di corsa aumentato di 1",
        .type = RunAdjustmentType::Increase,
        .stackingGroup = "feat.run",
        .selector = MovementSelector{},
        .expression = "1",
        .condition = std::nullopt
    }));
    view = movement.toView();
    assert(view.grants[0].run.effectiveMultiplier == 5);
    assert(view.grants[0].run.distanceUnits == 35);

    manager.addToCollection(RunAdjustmentsResource, RunAdjustment(RunAdjustmentDefinition{
        .id = "load.heavy",
        .source = "Carico pesante",
        .description = "Moltiplicatore di corsa ridotto di 1",
        .type = RunAdjustmentType::Penalty,
        .stackingGroup = std::string(HeavyArmorOrLoadRunPenaltyGroup),
        .selector = MovementSelector{},
        .expression = "1",
        .condition = std::nullopt
    }));
    manager.addToCollection(RunAdjustmentsResource, RunAdjustment(RunAdjustmentDefinition{
        .id = "armor.heavy",
        .source = "Armatura pesante",
        .description = "Moltiplicatore di corsa ridotto di 1",
        .type = RunAdjustmentType::Penalty,
        .stackingGroup = std::string(HeavyArmorOrLoadRunPenaltyGroup),
        .selector = MovementSelector{},
        .expression = "1",
        .condition = std::nullopt
    }));
    view = movement.toView();
    assert(view.grants[0].run.effectiveMultiplier == 4);
    assert(view.grants[0].run.distanceUnits == 28);
    assert(view.grants[0].run.adjustments.size() == 3);
    assert(std::ranges::count_if(view.grants[0].run.adjustments, [](const RunAdjustmentView &adjustment)
    {
        return adjustment.type == RunAdjustmentType::Penalty && adjustment.applied;
    }) == 1);

    manager.addToCollection(RunAdjustmentsResource, RunAdjustment(RunAdjustmentDefinition{
        .id = "feat.run.mythic",
        .source = "Correre (Mitico)",
        .description = "Moltiplicatore di corsa aumentato di 2",
        .type = RunAdjustmentType::Increase,
        .stackingGroup = "feat.run.mythic",
        .selector = MovementSelector{},
        .expression = "2",
        .condition = std::nullopt
    }));
    view = movement.toView();
    assert(view.grants[0].run.effectiveMultiplier == 6);
    assert(view.grants[0].run.distanceUnits == 42);

    manager.addToCollection(RunAdjustmentsResource, RunAdjustment(RunAdjustmentDefinition{
        .id = "overloaded",
        .source = "Sovraccarico",
        .description = "Non può correre",
        .type = RunAdjustmentType::Block,
        .stackingGroup = "overloaded",
        .selector = MovementSelector{},
        .expression = std::nullopt,
        .condition = std::nullopt
    }));
    view = movement.toView();
    assert(!view.grants[0].run.usable);
    assert(!view.grants[0].run.distanceUnits.has_value());
    assert(view.grants[0].run.notUsableReasons == std::vector<std::string>{"Non può correre"});
    manager.removeFromCollection(RunAdjustmentsResource, "overloaded");
    manager.removeFromCollection(RunAdjustmentsResource, "feat.run.mythic");
    manager.removeFromCollection(RunAdjustmentsResource, "armor.heavy");
    manager.removeFromCollection(RunAdjustmentsResource, "load.heavy");
    manager.removeFromCollection(RunAdjustmentsResource, "feat.run");

    manager.addToCollection(MovementGrantsResource, MovementGrant(MovementGrantDefinition{
        .id = "teleportStep",
        .source = "Passo magico",
        .type = MovementType::Land,
        .baseSpeedExpression = "6",
        .maneuverability = std::nullopt,
        .affectedByArmor = false,
        .affectedByLoad = false,
        .supportsRunning = false
    }));
    view = movement.toView();
    const auto teleportStep = std::ranges::find(view.grants, "teleportStep", &MovementGrantView::id);
    assert(teleportStep != view.grants.end());
    assert(!teleportStep->run.supported);
    assert(!teleportStep->run.usable);
    assert(!teleportStep->run.effectiveMultiplier.has_value());
    assert(!teleportStep->run.distanceUnits.has_value());
    assert(throwsInvalidArgument([&]
    {
        manager.addToCollection(RunAdjustmentsResource, RunAdjustment(RunAdjustmentDefinition{
            .id = "invalidExactRun",
            .source = "Errore",
            .description = "Aggiustamento non valido",
            .type = RunAdjustmentType::Increase,
            .stackingGroup = "invalid",
            .selector = MovementSelector{.type = MovementType::Land, .grantId = "teleportStep"},
            .expression = "1",
            .condition = std::nullopt
        }));
    }));
    manager.removeFromCollection(MovementGrantsResource, "teleportStep");

    assert(throwsInvalidArgument([]
    {
        static_cast<void>(RunAdjustment(RunAdjustmentDefinition{
            .id = "invalidBlock",
            .source = "Errore",
            .description = "Blocco non valido",
            .type = RunAdjustmentType::Block,
            .stackingGroup = "invalid",
            .selector = MovementSelector{},
            .expression = "1",
            .condition = std::nullopt
        }));
    }));
    assert(throwsInvalidArgument([&]
    {
        manager.removeFromCollection(RunAdjustmentsResource, "missing");
    }));

    assert(throwsInvalidArgument([&]
    {
        manager.addToCollection(MovementGrantsResource, MovementGrant(MovementGrantDefinition{
            .id = "invalid",
            .source = "Errore",
            .type = MovementType::Land,
            .baseSpeedExpression = "6",
            .maneuverability = Maneuverability::Good,
            .affectedByArmor = true,
            .affectedByLoad = true
        }));
    }));
    assert(flyCheckModifier(Maneuverability::Perfect) == 8);
    assert(displayName(MovementType::Burrow) == "Scavare");
    assert(displayName(MovementAdjustmentType::ReducedByArmorOrLoad) == "Velocità ridotta da armatura o carico");
    assert(displayName(RunAdjustmentType::Increase) == "Incremento del moltiplicatore di corsa");

    return 0;
}
