#pragma once

#include "golarion/character/skill.hpp"
#include "golarion/data/skills_save_data.hpp"
#include "golarion/view/skills_view.hpp"

#include <map>
#include <string>

namespace golarion
{
    class CharacterSheet;
    class ResourceManager;

    class Skills final
    {
    public:
        explicit Skills(ResourceManager &resourceManager);

        Skills(const Skills &) = delete;
        Skills &operator=(const Skills &) = delete;
        Skills(Skills &&) = delete;
        Skills &operator=(Skills &&) = delete;

        void setRanks(SkillType type, int ranks);
        void setClassSkill(SkillType type, bool classSkill);
        void setAbilityType(SkillType type, AbilityType abilityType);

        void addSpecialization(SkillType type, const std::string &specializationId, const std::string &specialization);
        void setSpecializationRanks(SkillType type, const std::string &specializationId, int ranks);
        SkillsView toView();
        SkillsSaveData toSaveData() const;

    private:
        friend class CharacterSheet;

        struct SpecializationDefaults
        {
            AbilityType abilityType;
            bool classSkill;
        };

        Skill &skill(SkillType type);
        Skill &specialization(SkillType type, const std::string &specializationId);
        SpecializationDefaults &specializationDefaults(SkillType type);
        std::string removableSpecializationResourceName(SkillType type, const std::string &specializationId) const;
        void removeSpecialization(SkillType type, const std::string &specializationId);
        void load(const SkillsSaveData &data);

        ResourceManager &resourceManager_;
        std::map<SkillType, Skill> skills_;
        std::map<SkillType, std::map<std::string, Skill>> specializations_;
        std::map<SkillType, SpecializationDefaults> specializationDefaults_;
    };
}
