#pragma once
#include "golarion/data/spell_repertoire_data.hpp"

namespace golarion
{
    struct RepertoireSelectionView
    {
        RepertoireSelectionDefinition selection;
        bool active;
    };
    struct RepertoireQuotaView
    {
        SpellRepertoireQuota grant;
        std::map<int, int> remainingByLevel;
    };
    struct SpellRepertoireView
    {
        bool manualAdditionAllowed;
        std::vector<std::string> manualSpellIds;
        std::vector<std::string> unassignedSpellIds;
        std::vector<RepertoireSelectionView> selections;
        std::vector<RepertoireQuotaView> quotas;
        std::vector<SpellRepertoireGrant> grants;
        std::vector<SpellRepertoireListGrant> listGrants;
    };
}
