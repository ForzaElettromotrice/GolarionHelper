#include "golarion/character/movement_group.hpp"
#include "golarion/data/movement_group_save_data.hpp"
#include "golarion/view/movement_group_view.hpp"

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

    golarion::MovementAdjustment flyAdjustment(std::string id)
    {
        return golarion::MovementAdjustment(golarion::MovementAdjustmentDefinition{
            .id = std::move(id),
            .source = "Lentezza",
            .description = "Velocità dimezzata",
            .type = golarion::MovementAdjustmentType::SpeedMultiplier,
            .selector = golarion::MovementSelector{.type = golarion::MovementType::Fly, .grantId = std::nullopt},
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

    MovementGroup group({flyGrant("spellFly")}, {flyAdjustment("slow")});
    const MovementGroupView view = group.toView();
    assert(view.grants.size() == 1);
    assert(view.grants[0].id == "spellFly");
    assert(view.adjustments.size() == 1);
    assert(view.adjustments[0].id == "slow");

    const MovementGroupSaveData data = group.toSaveData();
    MovementGroup restored(data);
    assert(restored.toSaveData().grants[0].maneuverability == Maneuverability::Good);
    assert(restored.toSaveData().adjustments[0].expression == "50");

    assert(throwsInvalidArgument([]
    {
        static_cast<void>(MovementGroup({}, {}));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(MovementGroup({flyGrant("same"), flyGrant("same")}, {}));
    }));
    assert(throwsInvalidArgument([]
    {
        static_cast<void>(MovementGroup({}, {flyAdjustment("same"), flyAdjustment("same")}));
    }));

    return 0;
}
