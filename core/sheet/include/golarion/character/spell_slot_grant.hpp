#pragma once

#include <map>
#include <optional>
#include <set>
#include <string>

namespace golarion
{
    struct SpellSlotGrant
    {
        std::string id;
        std::string source;
        std::map<int, int> dailyCapacityByLevel;
        // Unset uses the casting list; an explicitly empty set allows no spells.
        std::optional<std::set<std::string>> allowedSpellIds = std::nullopt;
    };

    SpellSlotGrant normalizeSpellSlotGrant(SpellSlotGrant grant);
}
