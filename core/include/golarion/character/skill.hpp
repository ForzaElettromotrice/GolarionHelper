#pragma once

#include "golarion/character/ability.hpp"

#include <map>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    class ResourceManager;
    class Skills;
    struct SkillSaveData;
    struct SkillView;

    inline constexpr std::string_view SkillAbilityReplacementsResource = "skill.abilityReplacements";
    inline constexpr std::string_view SkillClassSkillGrantsResource = "skill.classSkillGrants";
    inline constexpr std::string_view ArmorCheckPenaltiesResource = "skill.armorCheckPenalties";

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
    std::string skillCheckResourceName(AbilityType abilityType);
    AbilityType defaultAbility(SkillType type);
    bool trainedOnly(SkillType type);
    bool requiresSpecialization(SkillType type);
    bool appliesArmorCheckPenalty(SkillType type);

    struct ArmorCheckPenaltyDefinition
    {
        std::string id;
        std::string source;
        std::string expression;
    };

    class ArmorCheckPenalty final
    {
    public:
        explicit ArmorCheckPenalty(ArmorCheckPenaltyDefinition definition);

    private:
        friend class Skills;

        std::string id_;
        std::string source_;
        std::string expression_;
    };

    struct SkillAbilityReplacementDefinition
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        AbilityType abilityType;
    };

    class SkillAbilityReplacement final
    {
    public:
        explicit SkillAbilityReplacement(SkillAbilityReplacementDefinition definition);

    private:
        friend class Skill;
        friend class Skills;

        std::string id_;
        std::string source_;
        std::string targetResourceName_;
        AbilityType abilityType_;
    };

    struct SkillClassSkillGrantDefinition
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
    };

    class SkillClassSkillGrant final
    {
    public:
        explicit SkillClassSkillGrant(SkillClassSkillGrantDefinition definition);

    private:
        friend class Skill;
        friend class Skills;

        std::string id_;
        std::string source_;
        std::string targetResourceName_;
    };

    class Skill final
    {
    public:
        explicit Skill(SkillType type);
        Skill(SkillType type, const std::string &specializationId, const std::string &specialization);

        void setRanks(int ranks);
        void registerResources(ResourceManager &resourceManager) const;
        void registerResources(ResourceManager &resourceManager, std::vector<std::string> parentResources) const;

        SkillView toView(ResourceManager &resourceManager, const std::map<std::string, SkillAbilityReplacement> &abilityReplacements = {}, const std::map<std::string, SkillClassSkillGrant> &classSkillGrants = {}, int armorCheckPenalty = 0) const;
        SkillSaveData toSaveData() const;
        int totalValue(ResourceManager &resourceManager, int armorCheckPenalty = 0) const;
        bool usable() const;

    private:
        SkillType type_;
        std::optional<std::string> specializationId_;
        std::optional<std::string> specialization_;
        std::string resourceName_;
        int ranks_;
    };
}
