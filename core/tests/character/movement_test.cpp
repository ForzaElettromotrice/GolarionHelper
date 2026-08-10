#include "golarion/character/movement.hpp"
#include "golarion/data/movement_save_data.hpp"
#include "golarion/resource/modifier.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/movement_view.hpp"

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
    Movement movement(manager);
    manager.addToCollection(MovementGrantsResource, MovementGrant(MovementGrantDefinition{
        .id = "naturalWings",
        .source = "Ali naturali",
        .type = MovementType::Fly,
        .baseSpeedExpression = "8",
        .maneuverability = Maneuverability::Poor,
        .affectedByArmor = true,
        .affectedByLoad = true
    }));
    manager.addToCollection(MovementGrantsResource, MovementGrant(MovementGrantDefinition{
        .id = "spellFly",
        .source = "Volare",
        .type = MovementType::Fly,
        .baseSpeedExpression = "12",
        .maneuverability = Maneuverability::Good,
        .affectedByArmor = true,
        .affectedByLoad = true
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
    assert(view.grants[0].modifiers.total == 4);
    assert(view.grants[0].adjustments.size() == 3);
    assert(view.grants[1].id == "spellFly");
    assert(view.grants[1].modifiedBaseUnits == 14);
    assert(view.grants[1].effectiveUnits == 7);
    assert(view.grants[1].maneuverability == Maneuverability::Good);
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

    return 0;
}
