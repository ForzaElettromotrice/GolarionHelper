#pragma once

#include <cstdint>
#include <limits>
#include <stdexcept>

namespace golarion
{
    class GameDuration final
    {
    public:
        static GameDuration fromRounds(std::int64_t rounds)
        {
            return GameDuration(checkedRounds(rounds, 1));
        }

        static GameDuration fromMinutes(std::int64_t minutes)
        {
            return GameDuration(checkedRounds(minutes, 10));
        }

        static GameDuration fromHours(std::int64_t hours)
        {
            return GameDuration(checkedRounds(hours, 600));
        }

        std::int64_t roundCount() const
        {
            return rounds_;
        }

    private:
        explicit GameDuration(std::int64_t rounds) : rounds_(rounds)
        {
        }

        static std::int64_t checkedRounds(std::int64_t value, std::int64_t roundsPerUnit)
        {
            if (value < 0)
            {
                throw std::invalid_argument("game duration must not be negative");
            }
            if (value > std::numeric_limits<std::int64_t>::max() / roundsPerUnit)
            {
                throw std::invalid_argument("game duration is out of range");
            }
            return value * roundsPerUnit;
        }

        std::int64_t rounds_;
    };
}
