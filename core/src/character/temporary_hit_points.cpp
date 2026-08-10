#include "golarion/character/temporary_hit_points.hpp"

#include "golarion/data/temporary_hit_points_save_data.hpp"
#include "golarion/util/string_utils.hpp"
#include "golarion/view/temporary_hit_points_view.hpp"

#include <algorithm>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <unordered_set>
#include <utility>

namespace golarion
{
    void TemporaryHitPoints::add(std::string id, int amount, std::optional<GameDuration> duration)
    {
        id = normalize(id);
        if (amount <= 0)
        {
            throw std::invalid_argument("temporary hit point amount must be greater than zero");
        }
        if (duration && duration->roundCount() == 0)
        {
            throw std::invalid_argument("temporary hit point duration must be greater than zero");
        }

        auto existing = std::ranges::find(pools_, id, &Pool::id);
        if (existing == pools_.end())
        {
            if (static_cast<long long>(total()) + amount > std::numeric_limits<int>::max())
            {
                throw std::invalid_argument("temporary hit point total is out of range");
            }
            pools_.push_back(Pool{
                .id = std::move(id),
                .remaining = amount,
                .remainingDuration = duration
            });
            return;
        }

        if (amount > existing->remaining || (amount == existing->remaining && lastsLonger(duration, existing->remainingDuration)))
        {
            const long long newTotal = static_cast<long long>(total()) - existing->remaining + amount;
            if (newTotal > std::numeric_limits<int>::max())
            {
                throw std::invalid_argument("temporary hit point total is out of range");
            }
            existing->remaining = amount;
            existing->remainingDuration = duration;
        }
    }

    void TemporaryHitPoints::remove(std::string_view id)
    {
        const std::string normalizedId = normalize(id);
        const auto previousSize = pools_.size();
        std::erase_if(pools_, [&normalizedId](const Pool &pool)
        {
            return pool.id == normalizedId;
        });
        if (pools_.size() == previousSize)
        {
            throw std::invalid_argument("temporary hit point pool is not registered: " + normalizedId);
        }
    }

    int TemporaryHitPoints::absorb(int damage)
    {
        if (damage < 0)
        {
            throw std::invalid_argument("damage amount must not be negative");
        }

        while (damage > 0 && !pools_.empty())
        {
            auto pool = std::ranges::min_element(pools_, expiresBefore);
            const int absorbed = std::min(pool->remaining, damage);
            pool->remaining -= absorbed;
            damage -= absorbed;
            if (pool->remaining == 0)
            {
                pools_.erase(pool);
            }
        }
        return damage;
    }

    void TemporaryHitPoints::advanceTime(GameDuration duration)
    {
        const std::int64_t elapsedRounds = duration.roundCount();
        for (Pool &pool : pools_)
        {
            if (!pool.remainingDuration)
            {
                continue;
            }
            const std::int64_t remainingRounds = std::max<std::int64_t>(0, pool.remainingDuration->roundCount() - elapsedRounds);
            pool.remainingDuration = GameDuration::fromRounds(remainingRounds);
        }

        std::erase_if(pools_, [](const Pool &pool)
        {
            return pool.remainingDuration && pool.remainingDuration->roundCount() == 0;
        });
    }

    int TemporaryHitPoints::total() const
    {
        long long total = 0;
        for (const Pool &pool : pools_)
        {
            total += pool.remaining;
            if (total > std::numeric_limits<int>::max())
            {
                throw std::invalid_argument("temporary hit point total is out of range");
            }
        }
        return static_cast<int>(total);
    }

    TemporaryHitPointsView TemporaryHitPoints::toView() const
    {
        std::vector<Pool> sortedPools = pools_;
        std::ranges::sort(sortedPools, expiresBefore);

        std::vector<TemporaryHitPointPoolView> poolViews;
        poolViews.reserve(sortedPools.size());
        for (const Pool &pool : sortedPools)
        {
            poolViews.push_back(TemporaryHitPointPoolView{
                .id = pool.id,
                .remaining = pool.remaining,
                .remainingDuration = pool.remainingDuration
            });
        }
        return TemporaryHitPointsView{
            .total = total(),
            .pools = std::move(poolViews)
        };
    }

    TemporaryHitPointsSaveData TemporaryHitPoints::toSaveData() const
    {
        std::vector<Pool> sortedPools = pools_;
        std::ranges::sort(sortedPools, {}, &Pool::id);

        std::vector<TemporaryHitPointPoolSaveData> poolData;
        poolData.reserve(sortedPools.size());
        for (const Pool &pool : sortedPools)
        {
            poolData.push_back(TemporaryHitPointPoolSaveData{
                .id = pool.id,
                .remaining = pool.remaining,
                .remainingDuration = pool.remainingDuration
            });
        }
        return TemporaryHitPointsSaveData{.pools = std::move(poolData)};
    }

    void TemporaryHitPoints::load(const TemporaryHitPointsSaveData &data)
    {
        TemporaryHitPoints restored;
        std::unordered_set<std::string> ids;
        for (const TemporaryHitPointPoolSaveData &pool : data.pools)
        {
            const std::string normalizedId = normalize(pool.id);
            if (!ids.insert(normalizedId).second)
            {
                throw std::invalid_argument("temporary hit point pool is duplicated in save data: " + normalizedId);
            }
            restored.add(normalizedId, pool.remaining, pool.remainingDuration);
        }
        *this = std::move(restored);
    }

    bool TemporaryHitPoints::expiresBefore(const Pool &left, const Pool &right)
    {
        if (left.remainingDuration && right.remainingDuration)
        {
            if (left.remainingDuration->roundCount() != right.remainingDuration->roundCount())
            {
                return left.remainingDuration->roundCount() < right.remainingDuration->roundCount();
            }
            return left.id < right.id;
        }
        if (left.remainingDuration)
        {
            return true;
        }
        if (right.remainingDuration)
        {
            return false;
        }
        return left.id < right.id;
    }

    bool TemporaryHitPoints::lastsLonger(const std::optional<GameDuration> &left, const std::optional<GameDuration> &right)
    {
        if (!left)
        {
            return right.has_value();
        }
        if (!right)
        {
            return false;
        }
        return left->roundCount() > right->roundCount();
    }
}
