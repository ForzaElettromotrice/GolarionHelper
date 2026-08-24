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
        if (duration.has_value() && duration->roundCount() == 0)
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
                .duration = duration
            });
            return;
        }

        if (amount > existing->remaining || (amount == existing->remaining && lastsLonger(duration, existing->duration)))
        {
            const long long newTotal = static_cast<long long>(total()) - existing->remaining + amount;
            if (newTotal > std::numeric_limits<int>::max())
            {
                throw std::invalid_argument("temporary hit point total is out of range");
            }
            existing->remaining = amount;
            existing->duration = duration;
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
                .duration = pool.duration
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
                .duration = pool.duration
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
            restored.add(normalizedId, pool.remaining, pool.duration);
        }
        *this = std::move(restored);
    }

    bool TemporaryHitPoints::expiresBefore(const Pool &left, const Pool &right)
    {
        if (left.duration.has_value() && right.duration.has_value())
        {
            if (left.duration->roundCount() != right.duration->roundCount())
            {
                return left.duration->roundCount() < right.duration->roundCount();
            }
            return left.id < right.id;
        }
        if (left.duration.has_value())
        {
            return true;
        }
        if (right.duration.has_value())
        {
            return false;
        }
        return left.id < right.id;
    }

    bool TemporaryHitPoints::lastsLonger(const std::optional<GameDuration> &left, const std::optional<GameDuration> &right)
    {
        if (!left.has_value())
        {
            return right.has_value();
        }
        if (!right.has_value())
        {
            return false;
        }
        return left->roundCount() > right->roundCount();
    }
}
