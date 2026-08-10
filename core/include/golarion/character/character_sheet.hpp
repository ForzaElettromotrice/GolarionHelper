#pragma once

#include "golarion/character/ability.hpp"
#include "golarion/character/initiative.hpp"
#include "golarion/character/hit_points.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/movement_group_manager.hpp"
#include "golarion/character/saving_throws.hpp"
#include "golarion/character/skills.hpp"
#include "golarion/data/character_sheet_save_data.hpp"
#include "golarion/view/character_sheet_view.hpp"
#include "golarion/resource/modifier_group_manager.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/resource/contribution_group_manager.hpp"

#include <array>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>

namespace golarion
{
    class CharacterSheet final
    {
    public:
        CharacterSheet();
        static CharacterSheet load(const std::filesystem::path &path);

        void setAbilityBaseValue(AbilityType type, int baseValue);
        void setMaxHitPoints(int value);
        void setCurrentHitPoints(int value);
        void setNonLethalDamage(int value);
        void addTemporaryHitPoints(std::string id, int amount, std::optional<GameDuration> duration);
        void removeTemporaryHitPoints(std::string_view id);
        void advanceTime(GameDuration duration);
        void heal(int amount);
        void damage(int amount, DamageType type);
        void setInitiativeAbilityType(AbilityType abilityType);
        void setSavingThrowBaseValue(SavingThrowType type, int baseValue);
        void setSavingThrowAbilityType(SavingThrowType type, AbilityType abilityType);
        void setSkillRanks(SkillType type, int ranks);
        void setSkillClassSkill(SkillType type, bool classSkill);
        void setSkillAbilityType(SkillType type, AbilityType abilityType);
        void addSkillSpecialization(SkillType type, const std::string &specializationId, const std::string &specialization);
        void removeSkillSpecialization(SkillType type, const std::string &specializationId);
        void setSkillSpecializationRanks(SkillType type, const std::string &specializationId, int ranks);
        void createMovementGroup(const std::string &groupId, MovementGroup group);
        void destroyMovementGroup(std::string_view groupId);
        void setMovementGroupEnabled(std::string_view groupId, bool enabled);
        void createModifierGroup(const std::string &groupId, ModifierGroup group);
        void destroyModifierGroup(std::string_view groupId);
        void setModifierGroupEnabled(std::string_view groupId, bool enabled);
        void createContributionGroup(const std::string &groupId, ContributionGroup group);
        void destroyContributionGroup(std::string_view groupId);
        void setContributionGroupEnabled(std::string_view groupId, bool enabled);
        CharacterSheetView toView();
        CharacterSheetSaveData toSaveData() const;
        void save(const std::filesystem::path &path) const;

    private:
        static constexpr std::size_t AbilityCount = 6;

        explicit CharacterSheet(const CharacterSheetSaveData &data);
        AbilityScore &ability(AbilityType type);
        int level() const;

        ResourceManager resourceManager_;
        ModifierGroupManager modifierGroupManager_;
        ContributionGroupManager contributionGroupManager_;
        std::array<AbilityScore, AbilityCount> abilities_;
        HitPoints hitPoints_;
        Initiative initiative_;
        Skills skills_;
        SavingThrows savingThrows_;
        Movement movement_;
        MovementGroupManager movementGroupManager_;
    };
}
