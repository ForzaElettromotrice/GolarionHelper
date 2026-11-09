#pragma once

#include "golarion/character/character_class_type.hpp"

#include <variant>

namespace golarion
{
    // Closed rule vocabularies ported from the Java model on master (8f81a09).

    enum class SpellSchool
    {
        Abjuration,
        Conjuration,
        Divination,
        Enchantment,
        Evocation,
        Illusion,
        Necromancy,
        Transmutation,
        Universal
    };

    enum class SpellSubschool
    {
        Calling,
        Charm,
        Compulsion,
        Creation,
        Figment,
        Glamer,
        Healing,
        Pattern,
        Phantasm,
        Polymorph,
        Scrying,
        Shadow,
        Summoning,
        Teleportation
    };

    enum class SpellDescriptor
    {
        Acid,
        Air,
        Chaotic,
        Cold,
        Curse,
        Darkness,
        Death,
        Disease,
        Earth,
        Electricity,
        Emotion,
        Evil,
        Fear,
        Fire,
        Force,
        Good,
        LanguageDependent,
        Lawful,
        Light,
        MindAffecting,
        Pain,
        Poison,
        Shadow,
        Sonic,
        Water
    };

    enum class SpellCastingTimeCategory
    {
        SwiftAction,
        ImmediateAction,
        StandardAction,
        FullRoundAction,
        Round,
        Minute,
        Hour,
        Day,
        Special
    };

    enum class SpellComponentType
    {
        Verbal,
        Somatic,
        Material,
        Focus,
        DivineFocus,
        MaterialOrDivineFocus,
        FocusOrDivineFocus
    };

    enum class SpellRangeType
    {
        Personal,
        Touch,
        Close,
        Medium,
        Long,
        Unlimited,
        Other
    };

    enum class SpellDurationType
    {
        Instantaneous,
        Permanent,
        Concentration,
        Round,
        Minute,
        Hour,
        Day,
        Other
    };

    enum class SpellResistanceType
    {
        Yes,
        No,
        SeeText
    };

    enum class SpellSavingThrowEffect
    {
        Negates,
        Partial,
        Half,
        SeeText,
        Harmless,
        Object,
        Disbelief
    };

    enum class DomainType
    {
        Air,
        Animal,
        Artifice,
        Chaos,
        Charm,
        Community,
        Darkness,
        Death,
        Destruction,
        Earth,
        Evil,
        Fire,
        Glory,
        Good,
        Healing,
        Knowledge,
        Law,
        Liberation,
        Luck,
        Madness,
        Magic,
        Nobility,
        Plant,
        Protection,
        Reptile,
        Repose,
        Rune,
        Strength,
        Sun,
        Travel,
        Trickery,
        War,
        Water,
        Weather,
        Void
    };

    enum class SubdomainType
    {
        Agathion,
        Alienation,
        Ancestors,
        Arcane,
        Archon,
        Ash,
        Assassination,
        Azata,
        Blood,
        Catastrophe,
        Caves,
        Clouds,
        Commerce,
        Construct,
        Curse,
        DarkTapestry,
        Daemon,
        Day,
        Decay,
        Defense,
        Demon,
        Devil,
        Divine,
        Exploration,
        Family,
        Fate,
        Feather,
        Ferocity,
        Fur,
        Growth,
        Healing,
        Heroism,
        Home,
        Honor,
        Ice,
        Inevitables,
        Interdictions,
        Isolation,
        Language,
        Leadership,
        Liberation,
        Light,
        Loss,
        Loyalty,
        Love,
        Lust,
        Martyrdom,
        Memory,
        Metal,
        Moon,
        Night,
        Nightmare,
        Oceans,
        Protean,
        Purity,
        Rage,
        Resolution,
        Resurrection,
        Revolution,
        Rivers,
        Seasons,
        Smoke,
        Souls,
        Stars,
        Storms,
        Subterfuge,
        Tactics,
        Theft,
        Thought,
        Undeath,
        Whimsy,
        Wind
    };

    enum class BloodlineType
    {
        Aberrant,
        Abyssal,
        Accursed,
        Aquatic,
        Arcane,
        Celestial,
        Destined,
        Draconic,
        Earth,
        Elemental,
        Fey,
        Impossible,
        Infernal,
        Marid,
        Oni,
        Phoenix,
        Pestilence,
        Protean,
        Psychic,
        Serpentine,
        Shadow,
        Starsoul,
        Stormborn,
        Undead,
        Vermin
    };


    using DomainSpellListType = std::variant<DomainType, SubdomainType>;
}
