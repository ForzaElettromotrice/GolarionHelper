#include "golarion/character/temporary_hit_points.hpp"
#include "golarion/data/temporary_hit_points_save_data.hpp"
#include "golarion/view/temporary_hit_points_view.hpp"

#include <cassert>
#include <limits>
#include <optional>
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

    TemporaryHitPoints temporary;
    temporary.add("short", 5, GameDuration::fromRounds(2));
    temporary.add("long", 10, GameDuration::fromRounds(5));
    temporary.add("manual", 3, std::nullopt);
    assert(temporary.total() == 18);

    temporary.add("short", 4, GameDuration::fromRounds(10));
    temporary.add("short", 6, GameDuration::fromRounds(1));
    temporary.add("short", 6, GameDuration::fromRounds(3));
    TemporaryHitPointsView view = temporary.toView();
    assert(view.total == 19);
    assert(view.pools[0].id == "short");
    assert(view.pools[0].remaining == 6);
    assert(view.pools[0].remainingDuration->roundCount() == 3);
    assert(view.pools[1].id == "long");
    assert(view.pools[2].id == "manual");

    assert(temporary.absorb(7) == 0);
    view = temporary.toView();
    assert(view.total == 12);
    assert(view.pools[0].id == "long");
    assert(view.pools[0].remaining == 9);
    assert(view.pools[1].id == "manual");

    temporary.advanceTime(GameDuration::fromRounds(5));
    view = temporary.toView();
    assert(view.total == 3);
    assert(view.pools.size() == 1);
    assert(view.pools[0].id == "manual");

    const TemporaryHitPointsSaveData data = temporary.toSaveData();
    TemporaryHitPoints restored;
    restored.load(data);
    assert(restored.total() == 3);
    restored.remove(" manual ");
    assert(restored.total() == 0);

    assert(throwsInvalidArgument([&]
    {
        restored.remove("missing");
    }));
    assert(throwsInvalidArgument([&]
    {
        restored.absorb(-1);
    }));
    assert(throwsInvalidArgument([&]
    {
        restored.add("zero", 1, GameDuration::fromRounds(0));
    }));

    const TemporaryHitPointsSaveData duplicateData{
        .pools = {
            TemporaryHitPointPoolSaveData{.id = "same", .remaining = 1, .remainingDuration = std::nullopt},
            TemporaryHitPointPoolSaveData{.id = " same ", .remaining = 2, .remainingDuration = std::nullopt}
        }
    };
    assert(throwsInvalidArgument([&]
    {
        restored.load(duplicateData);
    }));

    TemporaryHitPoints overflow;
    overflow.add("first", std::numeric_limits<int>::max(), std::nullopt);
    assert(throwsInvalidArgument([&]
    {
        overflow.add("second", 1, std::nullopt);
    }));
    assert(overflow.total() == std::numeric_limits<int>::max());

    return 0;
}
