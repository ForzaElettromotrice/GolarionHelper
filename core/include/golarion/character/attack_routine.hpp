#pragma once

#include "golarion/character/damage.hpp"
#include "golarion/character/strike.hpp"

#include <optional>
#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    inline constexpr std::string_view AttackRoutinesResource = "attackRoutine.grants";
    inline constexpr std::string_view AttackRoutineProgressionGrantsResource = "attackRoutine.progressionGrants";
    inline constexpr std::string_view AttackRoutineBonusAdjustmentsResource = "attackRoutine.attackBonusAdjustments";

    class ResourceManager;
    class Strikes;
    struct AttackRoutinesView;

    enum class RoutineSlotSelectionMode
    {
        ChooseOne,
        AllMatching
    };

    enum class RoutineAttackProgressionType
    {
        Fixed,
        BaseAttackBonusIteratives
    };

    std::string_view displayName(RoutineSlotSelectionMode mode);
    std::string_view displayName(RoutineAttackProgressionType type);

    struct StrikeSelectorDefinition
    {
        std::vector<AttackMode> allowedModes;
        std::vector<AttackTag> requiredTags;
        std::vector<AttackTag> forbiddenTags;
        std::vector<std::string> allowedGrantIds;
    };

    class StrikeSelector final
    {
    public:
        explicit StrikeSelector(StrikeSelectorDefinition definition);

    private:
        friend class RoutineSlot;
        friend class AttackRoutines;

        std::vector<AttackMode> allowedModes_;
        std::vector<AttackTag> requiredTags_;
        std::vector<AttackTag> forbiddenTags_;
        std::vector<std::string> allowedGrantIds_;
    };

    struct RoutineAttackProgressionDefinition
    {
        RoutineAttackProgressionType type;
        std::optional<std::string> countExpression;
        std::string attackBonusAdjustmentExpression;
    };

    class RoutineAttackProgression final
    {
    public:
        explicit RoutineAttackProgression(RoutineAttackProgressionDefinition definition);

    private:
        friend class RoutineSlot;
        friend class AttackRoutines;

        RoutineAttackProgressionType type_;
        std::optional<std::string> countExpression_;
        std::string attackBonusAdjustmentExpression_;
    };

    struct RoutineSlotDefinition
    {
        std::string id;
        std::string name;
        StrikeSelector selector;
        RoutineSlotSelectionMode selectionMode;
        StrikeUsage strikeUsage;
        std::vector<RoutineAttackProgression> progressions;
        std::optional<DamageAbilityRule> damageAbilityRuleOverride;
        std::string baseAttackBonusAdjustmentExpression = "0";
    };

    class RoutineSlot final
    {
    public:
        explicit RoutineSlot(RoutineSlotDefinition definition);

    private:
        friend class AttackRoutine;
        friend class AttackRoutines;

        std::string id_;
        std::string name_;
        StrikeSelector selector_;
        RoutineSlotSelectionMode selectionMode_;
        StrikeUsage strikeUsage_;
        std::vector<RoutineAttackProgression> progressions_;
        std::optional<DamageAbilityRule> damageAbilityRuleOverride_;
        std::string baseAttackBonusAdjustmentExpression_;
    };

    struct AttackRoutineDefinition
    {
        std::string id;
        std::string source;
        std::string name;
        std::vector<RoutineSlot> slots;
    };

    class AttackRoutine final
    {
    public:
        explicit AttackRoutine(AttackRoutineDefinition definition);

    private:
        friend class AttackRoutines;

        std::string id_;
        std::string source_;
        std::string name_;
        std::vector<RoutineSlot> slots_;
    };

    struct RoutineProgressionGrantDefinition
    {
        std::string id;
        std::string source;
        std::string targetRoutineId;
        std::string targetSlotId;
        RoutineAttackProgression progression;
    };

    class RoutineProgressionGrant final
    {
    public:
        explicit RoutineProgressionGrant(RoutineProgressionGrantDefinition definition);

    private:
        friend class AttackRoutines;

        std::string id_;
        std::string source_;
        std::string targetRoutineId_;
        std::string targetSlotId_;
        RoutineAttackProgression progression_;
    };

    struct RoutineAssignmentRequirementDefinition
    {
        std::string slotId;
        std::optional<std::string> strikeGrantId;
        std::optional<AttackTag> requiredTag;
        std::optional<WeaponWeightPurpose> weaponWeightPurpose;
        std::optional<WeaponWeight> weaponWeight;
    };

    class RoutineAssignmentRequirement final
    {
    public:
        explicit RoutineAssignmentRequirement(RoutineAssignmentRequirementDefinition definition);

    private:
        friend class AttackRoutines;

        std::string slotId_;
        std::optional<std::string> strikeGrantId_;
        std::optional<AttackTag> requiredTag_;
        std::optional<WeaponWeightPurpose> weaponWeightPurpose_;
        std::optional<WeaponWeight> weaponWeight_;
    };

    struct RoutineAttackBonusAdjustmentDefinition
    {
        std::string id;
        std::string source;
        std::string targetRoutineId;
        std::optional<std::string> targetSlotId;
        std::string expression;
        std::vector<RoutineAssignmentRequirement> requirements;
    };

    class RoutineAttackBonusAdjustment final
    {
    public:
        explicit RoutineAttackBonusAdjustment(RoutineAttackBonusAdjustmentDefinition definition);

    private:
        friend class AttackRoutines;

        std::string id_;
        std::string source_;
        std::string targetRoutineId_;
        std::optional<std::string> targetSlotId_;
        std::string expression_;
        std::vector<RoutineAssignmentRequirement> requirements_;
    };

    class AttackRoutines final
    {
    public:
        AttackRoutines(ResourceManager &resourceManager, Strikes &strikes);

        AttackRoutinesView toView();

    private:
        void addRoutine(AttackRoutine routine);
        void removeRoutine(std::string_view routineId);
        void addProgressionGrant(RoutineProgressionGrant grant);
        void removeProgressionGrant(std::string_view grantId);
        void addAttackBonusAdjustment(RoutineAttackBonusAdjustment adjustment);
        void removeAttackBonusAdjustment(std::string_view adjustmentId);

        ResourceManager &resourceManager_;
        Strikes &strikes_;
        std::vector<AttackRoutine> routines_;
        std::map<std::string, RoutineProgressionGrant> progressionGrants_;
        std::map<std::string, RoutineAttackBonusAdjustment> attackBonusAdjustments_;
    };
}
