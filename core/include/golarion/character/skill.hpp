#pragma once

#include "golarion/character/ability.hpp"

#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    class ResourceManager;
    struct SkillSaveData;
    struct SkillView;

    enum class SkillType
    {
        Acrobatics,
        HandleAnimal,
        Craft,
        EscapeArtist,
        Disguise,
        Ride,
        KnowledgeArcana,
        KnowledgeDungeoneering,
        KnowledgeEngineering,
        KnowledgeGeography,
        KnowledgeHistory,
        KnowledgeLocal,
        KnowledgeNature,
        KnowledgeNobility,
        KnowledgePlanes,
        KnowledgeReligion,
        Diplomacy,
        DisableDevice,
        Stealth,
        Heal,
        Intimidate,
        Perform,
        SenseMotive,
        Linguistics,
        Swim,
        Perception,
        Profession,
        Bluff,
        SleightOfHand,
        Spellcraft,
        Climb,
        Survival,
        UseMagicDevice,
        Appraise,
        Fly
    };

    std::string_view displayName(SkillType type);
    std::string_view resourceName(SkillType type);
    AbilityType defaultAbility(SkillType type);
    bool trainedOnly(SkillType type);
    bool requiresSpecialization(SkillType type);
    bool appliesArmorCheckPenalty(SkillType type);

    class Skill final
    {
    public:
        explicit Skill(SkillType type);
        Skill(SkillType type, const std::string &specializationId, const std::string &specialization);

        void setRanks(int ranks);
        void setClassSkill(bool classSkill);
        void setAbilityType(AbilityType abilityType);
        void registerResources(ResourceManager &resourceManager) const;
        void registerResources(ResourceManager &resourceManager, std::vector<std::string> parentResources) const;

        SkillView toView(ResourceManager &resourceManager) const;
        SkillSaveData toSaveData() const;
        int totalValue(ResourceManager &resourceManager) const;
        bool usable() const;

    private:
        SkillType type_;
        std::optional<std::string> specializationId_;
        std::optional<std::string> specialization_;
        std::string resourceName_;
        AbilityType abilityType_;
        int ranks_;
        bool classSkill_;
    };
}
