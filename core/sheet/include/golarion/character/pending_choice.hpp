#pragma once

#include "golarion/character/character_class_type.hpp"
#include "golarion/character/spell_slot_id.hpp"
#include "golarion/data/languages_data.hpp"
#include <cstddef>
#include <string>
#include <variant>
#include <vector>

namespace golarion
{
    enum class PendingChoiceType
    {
        Race, Alignment, RacialOption, ClassOption, FeatSlot, ClassHitDie,
        FavoredClass, FavoredClassBonus, SkillRanks, Language, PreparedSpell, FlexiblePreparation, Repertoire
    };

    struct RaceChoiceReference
    {
        static constexpr auto Type = PendingChoiceType::Race;
        bool operator==(const RaceChoiceReference &) const = default;
    };
    struct AlignmentChoiceReference
    {
        static constexpr auto Type = PendingChoiceType::Alignment;
        bool operator==(const AlignmentChoiceReference &) const = default;
    };
    struct RacialOptionChoiceReference
    {
        static constexpr auto Type = PendingChoiceType::RacialOption;
        std::string raceId, elementId, choiceId;
        bool operator==(const RacialOptionChoiceReference &) const = default;
    };
    struct ClassOptionChoiceReference
    {
        static constexpr auto Type = PendingChoiceType::ClassOption;
        CharacterClassType classType;
        std::string acquisitionId, choiceId;
        bool operator==(const ClassOptionChoiceReference &) const = default;
    };
    struct FeatSlotChoiceReference
    {
        static constexpr auto Type = PendingChoiceType::FeatSlot;
        std::string slotId;
        bool operator==(const FeatSlotChoiceReference &) const = default;
    };
    struct ClassHitDieChoiceReference
    {
        static constexpr auto Type = PendingChoiceType::ClassHitDie;
        CharacterClassType classType;
        int classLevel;
        bool operator==(const ClassHitDieChoiceReference &) const = default;
    };
    struct FavoredClassChoiceReference
    {
        static constexpr auto Type = PendingChoiceType::FavoredClass;
        bool operator==(const FavoredClassChoiceReference &) const = default;
    };
    struct FavoredClassBonusChoiceReference
    {
        static constexpr auto Type = PendingChoiceType::FavoredClassBonus;
        CharacterClassType classType;
        int classLevel;
        bool operator==(const FavoredClassBonusChoiceReference &) const = default;
    };
    struct SkillRanksChoiceReference
    {
        static constexpr auto Type = PendingChoiceType::SkillRanks;
        bool operator==(const SkillRanksChoiceReference &) const = default;
    };
    struct LanguageChoiceReference
    {
        static constexpr auto Type = PendingChoiceType::Language;
        LanguageOrigin origin;
        bool operator==(const LanguageChoiceReference &) const = default;
    };
    struct PreparedSpellChoiceReference
    {
        static constexpr auto Type = PendingChoiceType::PreparedSpell;
        std::string castingId;
        SpellSlotId slot;
        bool operator==(const PreparedSpellChoiceReference &) const = default;
    };
    struct FlexiblePreparationChoiceReference
    {
        static constexpr auto Type = PendingChoiceType::FlexiblePreparation;
        std::string castingId;
        SpellSlotId preparation;
        bool operator==(const FlexiblePreparationChoiceReference &) const = default;
    };

    struct RepertoireChoiceReference
    {
        static constexpr auto Type = PendingChoiceType::Repertoire;
        std::string castingId, quotaId;
        int spellLevel;
        bool operator==(const RepertoireChoiceReference &) const = default;
    };

    using PendingChoiceReference = std::variant<RaceChoiceReference, AlignmentChoiceReference, RacialOptionChoiceReference,
        ClassOptionChoiceReference, FeatSlotChoiceReference, ClassHitDieChoiceReference, FavoredClassChoiceReference,
        FavoredClassBonusChoiceReference, SkillRanksChoiceReference, LanguageChoiceReference, PreparedSpellChoiceReference,
        FlexiblePreparationChoiceReference, RepertoireChoiceReference>;

    // Derived, detached UI snapshot; never persisted and never owns runtime callbacks.
    struct PendingChoice
    {
        PendingChoiceReference choiceReference;
        std::string description;
        std::size_t remaining;
        bool blocksAdvancement;

        PendingChoiceType type() const { return std::visit([](const auto &reference) { return reference.Type; }, choiceReference); }
        bool operator==(const PendingChoice &) const = default;
    };
}
