#include "golarion/character/movement.hpp"
#include "golarion/character/movement_group_manager.hpp"
#include "golarion/data/movement_group_manager_save_data.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/movement_group_manager_view.hpp"
#include "golarion/view/movement_view.hpp"

#include <cassert>
#include <stdexcept>
#include <vector>

namespace
{
    golarion::MovementGrant flyGrant(std::string id)
    {
        return golarion::MovementGrant(golarion::MovementGrantDefinition{
            .id = std::move(id),
            .source = "Volare",
            .type = golarion::MovementType::Fly,
            .baseSpeedExpression = "12",
            .maneuverability = golarion::Maneuverability::Good,
            .affectedByArmor = true,
            .affectedByLoad = true
        });
    }

    golarion::MovementAdjustment exactAdjustment(std::string id, std::string grantId)
    {
        return golarion::MovementAdjustment(golarion::MovementAdjustmentDefinition{
            .id = std::move(id),
            .source = "Effetto",
            .description = "Velocità dimezzata",
            .type = golarion::MovementAdjustmentType::SpeedMultiplier,
            .selector = golarion::MovementSelector{.type = golarion::MovementType::Fly, .grantId = std::move(grantId)},
            .expression = "50",
            .condition = std::nullopt
        });
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

    ResourceManager resourceManager;
    Movement movement(resourceManager);
    MovementGroupManager manager(resourceManager);

    manager.addGroup("user.flight", MovementGroup({flyGrant("spellFly")}, {exactAdjustment("slowFly", "spellFly")}), false);
    assert(movement.toView().grants.empty());
    manager.setGroupEnabled("user.flight", true);
    MovementView movementView = movement.toView();
    assert(movementView.grants.size() == 1);
    assert(movementView.grants[0].effectiveUnits == 6);

    MovementGroupManagerView managerView = manager.toView();
    assert(managerView.groups.size() == 1);
    assert(managerView.groups[0].id == "user.flight");
    assert(managerView.groups[0].enabled);
    assert(managerView.groups[0].group.grants.size() == 1);
    assert(managerView.groups[0].group.adjustments.size() == 1);

    manager.setGroupEnabled("user.flight", false);
    assert(movement.toView().grants.empty());
    manager.setGroupEnabled("user.flight", true);

    manager.addGroup("dependent", MovementGroup({flyGrant("wings")}, {}), true);
    resourceManager.addToCollection(MovementAdjustmentsResource, exactAdjustment("external", "wings"));
    assert(throwsInvalidArgument([&]
    {
        manager.setGroupEnabled("dependent", false);
    }));
    assert(manager.toView().groups[0].enabled);
    resourceManager.removeFromCollection(MovementAdjustmentsResource, "external");
    manager.setGroupEnabled("dependent", false);

    MovementGroup invalidGroup({flyGrant("temporary")}, {exactAdjustment("missingTarget", "missing")});
    assert(throwsInvalidArgument([&]
    {
        manager.addGroup("invalid", std::move(invalidGroup), true);
    }));
    assert(movement.toView().grants.size() == 1);
    assert(movement.toView().grants[0].id == "spellFly");

    manager.addGroup("disabled", MovementGroup({}, {exactAdjustment("later", "spellFly")}), false);
    const MovementGroupManagerSaveData data = manager.toSaveData();
    assert(data.groups.size() == 3);
    assert(data.groups[0].id == "dependent");
    assert(!data.groups[0].enabled);
    assert(data.groups[1].id == "disabled");
    assert(!data.groups[1].enabled);
    assert(data.groups[2].id == "user.flight");
    assert(data.groups[2].enabled);

    manager.removeGroup("disabled");
    manager.removeGroup("dependent");
    manager.removeGroup("user.flight");
    assert(movement.toView().grants.empty());

    return 0;
}
