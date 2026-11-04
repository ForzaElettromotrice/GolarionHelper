#pragma once

#include "golarion/util/game_duration.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    inline constexpr std::string_view TemporaryHitPointsResource = "hp.temporary";

    struct TemporaryHitPointGrant
    {
        std::string id;
        std::string amountExpression;
        std::optional<GameDuration> duration;
    };

    struct TemporaryHitPointsSaveData;
    struct TemporaryHitPointsView;

    class TemporaryHitPoints final
    {
    public:
        void add(std::string id, int amount, std::optional<GameDuration> duration);
        void remove(std::string_view id);
        int absorb(int damage);
        int total() const;
        TemporaryHitPointsView toView() const;
        TemporaryHitPointsSaveData toSaveData() const;
        void load(const TemporaryHitPointsSaveData &data);

    private:
        struct Pool
        {
            std::string id;
            int remaining;
            std::optional<GameDuration> duration;
        };

        static bool expiresBefore(const Pool &left, const Pool &right);
        static bool lastsLonger(const std::optional<GameDuration> &left, const std::optional<GameDuration> &right);

        std::vector<Pool> pools_;
    };
}
