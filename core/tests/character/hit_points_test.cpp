#include "golarion/character/hit_points.hpp"
#include "golarion/data/hit_points_save_data.hpp"
#include "golarion/resource/contribution.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/hit_points_view.hpp"

#include <cassert>
#include <limits>
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
    HitPoints hitPoints(manager);
    manager.registerTarget("temporaryAmount", []
    {
        return 2;
    });
    hitPoints.setMax(10);
    hitPoints.setCurrent(10);
    manager.addToCollection(TemporaryHitPointsResource, TemporaryHitPointGrant{
        .id = "short",
        .amountExpression = "@temporaryAmount",
        .duration = GameDuration::fromRounds(1)
    });
    hitPoints.setNonLethal(12);
    manager.addContribution("hp.max", Contribution("toughness", "3"));

    assert(throwsInvalidArgument([&]
    {
        manager.targetValue("hp.max");
    }));
    assert(throwsInvalidArgument([&]
    {
        manager.targetValue("hp.current");
    }));
    assert(throwsInvalidArgument([&]
    {
        manager.targetValue("hp.temporary");
    }));

    hitPoints.damage(5, DamageType::NonLethal);
    HitPointsView view = hitPoints.toView();
    assert(view.baseMax == 10);
    assert(view.max == 13);
    assert(view.current == 11);
    assert(view.temporary.total == 0);
    assert(view.temporary.pools.empty());
    assert(view.nonLethal == 13);
    assert(view.maxContributions.total == 3);
    assert(view.maxContributions.contributions.size() == 1);

    hitPoints.heal(4);
    view = hitPoints.toView();
    assert(view.current == 13);
    assert(view.nonLethal == 9);
    assert(view.temporary.total == 0);

    hitPoints.heal(10);
    view = hitPoints.toView();
    assert(view.current == 13);
    assert(view.nonLethal == 0);

    hitPoints.damage(20, DamageType::Lethal);
    assert(hitPoints.toView().current == -7);

    hitPoints.setMax(5);
    assert(hitPoints.toView().max == 8);
    assert(throwsInvalidArgument([&]
    {
        hitPoints.setCurrent(9);
    }));
    assert(throwsInvalidArgument([&]
    {
        hitPoints.setMax(-1);
    }));
    assert(throwsInvalidArgument([&]
    {
        manager.addToCollection(TemporaryHitPointsResource, TemporaryHitPointGrant{
            .id = "invalid",
            .amountExpression = "-1",
            .duration = std::nullopt
        });
    }));
    assert(throwsInvalidArgument([&]
    {
        hitPoints.setNonLethal(-1);
    }));
    assert(throwsInvalidArgument([&]
    {
        hitPoints.heal(-1);
    }));
    assert(throwsInvalidArgument([&]
    {
        hitPoints.damage(-1, DamageType::Lethal);
    }));

    hitPoints.addTemporary("manual", 2, std::nullopt);
    assert(throwsInvalidArgument([&]
    {
        hitPoints.damage(1, static_cast<DamageType>(99));
    }));
    assert(hitPoints.toView().temporary.total == 2);

    hitPoints.setCurrent(hitPoints.toView().max - std::numeric_limits<int>::max());
    assert(throwsInvalidArgument([&]
    {
        hitPoints.damage(3, DamageType::Lethal);
    }));
    assert(hitPoints.toView().temporary.total == 2);
    assert(hitPoints.toView().current == hitPoints.toView().max - std::numeric_limits<int>::max());

    const HitPointsSaveData data = hitPoints.toSaveData();
    assert(data.baseMax == 5);
    assert(data.damageTaken == std::numeric_limits<int>::max());
    assert(data.temporary.pools.size() == 1);
    assert(data.temporary.pools[0].id == "manual");
    assert(data.temporary.pools[0].remaining == 2);
    assert(data.nonLethal == 0);

    assert(displayName(DamageType::Lethal) == "Letale");
    assert(displayName(DamageType::NonLethal) == "Non letale");

    return 0;
}
