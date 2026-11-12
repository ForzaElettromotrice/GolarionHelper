#pragma once
#include "golarion/effect/collection_effect.hpp"
#include "golarion/effect/casting_rule_effect.hpp"

#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/character_class_type.hpp"
#include "golarion/character/granted_spell.hpp"
#include "golarion/character/spellcasting_mode.hpp"
#include "golarion/character/spell_slot_grant.hpp"
#include "golarion/data/languages_data.hpp"
#include "golarion/character/skill.hpp"
#include "golarion/character/attack_routine.hpp"
#include "golarion/character/feat_slot_filter.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/size.hpp"
#include "golarion/resource/modifier.hpp"

#include <optional>
#include <set>
#include <string>
#include <variant>
#include <vector>

namespace golarion
{
    struct ModifierEffectDefinition
    {
        std::string id;
        std::string resource;
        std::string description;
        ModifierType type;
        std::optional<BonusType> bonusType;
        std::string expression;
        std::optional<std::string> condition = std::nullopt;
        SkillRankBudgetPolicy skillRankBudget = SkillRankBudgetPolicy::Included;
    };

    struct ContributionEffectDefinition
    {
        std::string id;
        std::string resource;
        std::string expression;
    };

    struct SizeBaseEffectDefinition
    {
        std::string id;
        SizeCategory category;
    };

    struct MovementGrantEffectDefinition
    {
        std::string id;
        MovementType type;
        std::string baseSpeedExpression;
        std::optional<Maneuverability> maneuverability = std::nullopt;
        bool affectedByArmor;
        bool affectedByLoad;
        bool supportsRunning = false;
    };

    struct CarryingBodyTypeEffectDefinition
    {
        std::string id;
        CarryingBodyType type;
    };

    struct ReminderEffectDefinition
    {
        std::string id;
        std::string message;
    };

    struct LanguageGrantEffectDefinition
    {
        std::string id;
        std::string languageId;
    };

    struct FeatSlotGrantEffectDefinition
    {
        std::string id;
        FeatSlotFilter filter = AnyFeat{};
    };

    struct LanguageOptionsEffectDefinition
    {
        std::string id;
        LanguageOrigin origin;
        std::set<std::string> languageIds;
    };

    struct SkillRankGrantEffectDefinition
    {
        std::string id;
        SkillType skill;
        std::string expression;
        std::optional<std::string> specializationId = std::nullopt;
    };

    struct HandUsagePermissionEffectDefinition
    {
        std::string id;
        std::string strikeGrantId;
        HandUsage handUsage;
        std::vector<Requirement> requirements{};
        std::optional<std::string> routineId = std::nullopt;
        std::optional<std::string> slotId = std::nullopt;
    };

    struct SpellGrantEffectDefinition
    {
        std::string id;
        std::string spellId;
        GrantedSpellUsage usage;
        int capacity = 0;
        std::optional<std::string> poolId = std::nullopt;
        int cost = 1;
    };

    void validateSpellGrantEffectDefinition(const SpellGrantEffectDefinition &effect);

    struct SpellcastingGrantEffectDefinition
    {
        std::string id;
        SpellcastingMode mode;
        CharacterClassType spellListClass;
    };

    enum class SpellSlotPoolType { Casting, Preparation };

    struct SpellSlotGrantEffectDefinition
    {
        std::string id;
        std::string castingId;
        SpellSlotPoolType pool;
        std::map<int, int> dailyCapacityByLevel;
        std::optional<std::set<std::string>> allowedSpellIds = std::nullopt;
    };

    // Favored bonuses may contain ordinary effects, but cannot recursively grant favored options.
    using FavoredClassBenefitEffect = std::variant<
        CastingRuleEffectDefinition, FeatPrerequisiteAlternativeEffectDefinition,
        CollectionEffectDefinition,
        ModifierEffectDefinition,
        ContributionEffectDefinition,
        SizeBaseEffectDefinition,
        MovementGrantEffectDefinition,
        CarryingBodyTypeEffectDefinition,
        ReminderEffectDefinition,
        LanguageGrantEffectDefinition,
        LanguageOptionsEffectDefinition,
        SkillRankGrantEffectDefinition,
        HandUsagePermissionEffectDefinition,
        FeatSlotGrantEffectDefinition,
        SpellGrantEffectDefinition,
        SpellcastingGrantEffectDefinition,
        SpellSlotGrantEffectDefinition>;

    enum class FavoredClassBaseBonus { HitPoint, SkillRank };

    struct FavoredClassGrantEffectDefinition
    {
        std::string id;
        int additionalChoices = 0;
        std::set<CharacterClassType> classes{};
        bool allClasses = false;
    };

    struct FavoredClassBonusEffectDefinition
    {
        std::string id;
        std::string name;
        std::string description;
        std::set<CharacterClassType> classes{};
        std::vector<FavoredClassBenefitEffect> effects{};
        std::set<FavoredClassBaseBonus> replaces{};
        int selectionsPerBenefit = 1;
    };

    using EffectDefinition = std::variant<
        CastingRuleEffectDefinition, FeatPrerequisiteAlternativeEffectDefinition,
        CollectionEffectDefinition,
        ModifierEffectDefinition,
        ContributionEffectDefinition,
        SizeBaseEffectDefinition,
        MovementGrantEffectDefinition,
        CarryingBodyTypeEffectDefinition,
        ReminderEffectDefinition,
        LanguageGrantEffectDefinition,
        LanguageOptionsEffectDefinition,
        SkillRankGrantEffectDefinition,
        HandUsagePermissionEffectDefinition,
        FeatSlotGrantEffectDefinition,
        SpellGrantEffectDefinition,
        SpellcastingGrantEffectDefinition,
        SpellSlotGrantEffectDefinition,
        FavoredClassGrantEffectDefinition,
        FavoredClassBonusEffectDefinition>;

    inline const std::string &effectId(const EffectDefinition &definition)
    {
        return std::visit([](const auto &effect) -> const std::string &
        {
            return effect.id;
        }, definition);
    }
}
