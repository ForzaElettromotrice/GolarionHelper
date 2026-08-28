#pragma once

#include <string_view>
#include <vector>

namespace golarion
{
    enum class DamageReductionBypassTrait
    {
        Bludgeoning,
        Piercing,
        Slashing,
        Magic,
        Epic,
        Adamantine,
        Silver,
        ColdIron,
        Good,
        Evil,
        Lawful,
        Chaotic
    };

    std::string_view displayName(DamageReductionBypassTrait trait);

    struct DamageReductionBypassAlternative
    {
        std::vector<DamageReductionBypassTrait> allOf;

        bool operator==(const DamageReductionBypassAlternative &) const = default;
    };

    struct DamageReductionBypass
    {
        std::vector<DamageReductionBypassAlternative> anyOf;

        bool operator==(const DamageReductionBypass &) const = default;
    };
}
