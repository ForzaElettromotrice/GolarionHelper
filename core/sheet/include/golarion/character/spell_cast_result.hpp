#pragma once

#include "golarion/character/metamagic.hpp"

namespace golarion
{
    struct SpellCastResult
    {
        // -1 means unlimited; otherwise this is a positive upper bound.
        int maxTargets = -1;
        std::vector<EffectDefinition> effects;
        std::optional<MetamagicEvaluation> metamagic;
    };

    // Call only after spending the slot/uses. Export failures do not refund them.
    SpellCastResult exportSpellCast(const SpellDefinition &spell, ResourceManager &caster, std::optional<MetamagicEvaluation> metamagic = std::nullopt);
}
