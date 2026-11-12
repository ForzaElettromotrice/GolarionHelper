#pragma once
#include <map>
#include <optional>
#include <set>
#include <string>

namespace golarion
{
    struct SpellRepertoireQuota
    {
        std::string id, source;
        std::map<int, int> capacityByLevel;
        std::optional<std::set<std::string>> allowedSpellIds = std::nullopt;
    };
    struct SpellRepertoireGrant
    {
        std::string id, source;
        std::set<std::string> spellIds;
    };
    struct SpellRepertoireListGrant
    {
        std::string id, source;
    };
    struct RepertoireSelectionDefinition
    {
        std::string quotaId;
        int spellLevel;
        std::string spellId;
        bool operator==(const RepertoireSelectionDefinition &) const = default;
    };
}
