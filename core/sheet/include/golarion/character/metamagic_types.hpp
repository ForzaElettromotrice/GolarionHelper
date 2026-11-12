#pragma once

namespace golarion
{
    enum class MetamagicType
    {
        Empower, Enlarge, Extend, Heighten, Maximize, Quicken, Silent, Still, Widen,
        Elemental, Reach, Intensified, Selective
    };
    enum class MetamagicPhase { Preparation, Casting };
    enum class ElementalDamageMode { Replace, Split };
    enum class MetamagicRodGrade { Lesser, Normal, Greater };
    enum class MetamagicSlotCostPolicy { Standard, Waived };
    enum class CastingMetamagicScope { AnyCast, SpontaneousConversionOnly };
}
