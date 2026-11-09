#include "golarion/resource/usage_pool.hpp"
#include <algorithm>
#include <stdexcept>

namespace golarion
{
    UsagePool::UsagePool(int capacity, std::optional<int> remaining) : capacity_(capacity), remaining_(remaining.value_or(capacity))
    {
        if (capacity <= 0 || remaining_ < 0 || remaining_ > capacity) throw std::invalid_argument("invalid usage pool capacity or remaining uses");
    }
    void UsagePool::consume(int amount)
    {
        if (amount <= 0 || amount > remaining_) throw std::invalid_argument("invalid or unavailable usage cost");
        remaining_ -= amount;
    }
    void UsagePool::refill() noexcept { remaining_ = capacity_; }
    void UsagePool::restore(int amount)
    {
        if (amount <= 0) throw std::invalid_argument("restored uses must be positive");
        remaining_ += std::min(amount, capacity_ - remaining_);
    }
}
