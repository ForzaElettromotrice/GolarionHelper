#pragma once

#include "golarion/character/ability.hpp"
#include "golarion/character/armor_class.hpp"
#include "golarion/character/attack.hpp"
#include "golarion/character/base_attack_bonus.hpp"
#include "golarion/character/combat_maneuvers.hpp"
#include "golarion/character/initiative.hpp"
#include "golarion/character/hit_points.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/saving_throws.hpp"
#include "golarion/character/skills.hpp"
#include "golarion/data/character_sheet_save_data.hpp"
#include "golarion/view/character_sheet_view.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <array>
#include <filesystem>
#include <string>

namespace golarion
{
    class CharacterSheet final
    {
    public:
        CharacterSheet();
        static CharacterSheet load(const std::filesystem::path &path);

        void setAbilityBaseValue(AbilityType type, int baseValue);
        void advanceTime(GameDuration duration);
        void heal(int amount);
        void damage(int amount, DamageLethality lethality);
        void setSkillRanks(SkillType type, int ranks);
        void addSkillSpecialization(SkillType type, const std::string &specializationId, const std::string &specialization);
        void removeSkillSpecialization(SkillType type, const std::string &specializationId);
        void setSkillSpecializationRanks(SkillType type, const std::string &specializationId, int ranks);
        CharacterSheetView toView();
        CharacterSheetSaveData toSaveData() const;
        void save(const std::filesystem::path &path) const;

    private:
        static constexpr std::size_t AbilityCount = 6;

        explicit CharacterSheet(const CharacterSheetSaveData &data);
        AbilityScore &ability(AbilityType type);
        int level() const;

        ResourceManager resourceManager_;
        BaseAttackBonus baseAttackBonus_;
        std::array<AbilityScore, AbilityCount> abilities_;
        Attacks attacks_;
        CombatManeuvers combatManeuvers_;
        HitPoints hitPoints_;
        Initiative initiative_;
        ArmorClass armorClass_;
        Skills skills_;
        SavingThrows savingThrows_;
        Movement movement_;
    };
}
