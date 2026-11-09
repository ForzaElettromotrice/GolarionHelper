#pragma once

#include "golarion/resource/usage_pool.hpp"
#include <memory>
#include <string>

namespace golarion
{
    enum class GrantedSpellUsage { Limited, AtWill, Constant, SharedPool };

    struct GrantedSpellDefinition
    {
        std::string id;
        std::string source;
        std::string spellId;
        GrantedSpellUsage usage;
        int capacity = 0;
        std::shared_ptr<UsagePool> pool{};
        int cost = 1;
    };

    struct GrantedSpellView
    {
        std::string id;
        std::string source;
        std::string spellId;
        GrantedSpellUsage usage;
        std::optional<int> capacity;
        std::optional<int> remaining;
        int cost;
    };

    class GrantedSpell final
    {
    public:
        explicit GrantedSpell(GrantedSpellDefinition definition);
        GrantedSpell(const GrantedSpell &) = delete;
        GrantedSpell &operator=(const GrantedSpell &) = delete;
        GrantedSpell(GrantedSpell &&) = default;
        GrantedSpell &operator=(GrantedSpell &&) = default;
        void consume();
        void refill();
        void restore(int amount);
        GrantedSpellView toView() const;
    private:
        friend class GrantedSpells;
        GrantedSpellDefinition definition_;
    };
}
