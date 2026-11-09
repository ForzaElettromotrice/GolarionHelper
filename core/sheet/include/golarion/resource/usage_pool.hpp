#pragma once

#include <optional>

namespace golarion
{
    class UsagePool final
    {
    public:
        explicit UsagePool(int capacity, std::optional<int> remaining = std::nullopt);
        UsagePool(const UsagePool &) = delete;
        UsagePool &operator=(const UsagePool &) = delete;
        int capacity() const noexcept { return capacity_; }
        int remaining() const noexcept { return remaining_; }
        void consume(int amount = 1);
        void refill() noexcept;
        void restore(int amount);
    private:
        int capacity_;
        int remaining_;
    };
}
