#include "golarion/util/game_duration.hpp"

#include <cassert>
#include <cstdint>
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

    assert(GameDuration::fromRounds(3).roundCount() == 3);
    assert(GameDuration::fromMinutes(2).roundCount() == 20);
    assert(GameDuration::fromHours(1).roundCount() == 600);
    assert(throwsInvalidArgument([]
    {
        GameDuration::fromRounds(-1);
    }));
    assert(throwsInvalidArgument([]
    {
        GameDuration::fromHours(std::numeric_limits<std::int64_t>::max());
    }));

    return 0;
}
