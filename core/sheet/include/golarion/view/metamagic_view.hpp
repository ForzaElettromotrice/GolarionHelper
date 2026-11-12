#pragma once

#include "golarion/character/metamagic.hpp"

namespace golarion
{
    struct MetamagicUsagePoolView
    {
        std::string id;
        int remaining;
    };

    struct MetamagicUsageCostView
    {
        std::string poolId;
        int uses;
        std::optional<int> remaining;
    };

    struct MetamagicGrantView
    {
        std::string id;
        std::string source;
        MetamagicPhase phase;
        MetamagicType type;
        std::optional<DamageType> elementalEnergy;
        MetamagicSlotCostPolicy slotCost;
        std::optional<MetamagicRodGrade> rodGrade;
        CastingMetamagicScope castingScope;
        std::optional<MetamagicUsageCostView> usageCost;
        bool hasApplicabilityRestriction;
    };

    struct MetamagicMixingPermissionView
    {
        std::string id;
        std::string source;
        std::optional<MetamagicUsageCostView> usageCost;
    };
}
