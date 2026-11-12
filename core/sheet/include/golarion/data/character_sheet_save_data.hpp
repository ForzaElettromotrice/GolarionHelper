#pragma once
#include "golarion/character/feat_definition.hpp"
#include "golarion/character/spell_definition.hpp"
#include "golarion/equipment/item.hpp"
#include "golarion/data/enhancements_data.hpp"

#include "golarion/data/ability_save_data.hpp"
#include "golarion/data/classes_data.hpp"
#include "golarion/data/attacks_data.hpp"
#include "golarion/data/feats_data.hpp"
#include "golarion/data/languages_data.hpp"
#include "golarion/data/spellcastings_data.hpp"
#include "golarion/data/granted_spells_data.hpp"
#include "golarion/data/character_identity_save_data.hpp"
#include "golarion/data/condition_manager_save_data.hpp"
#include "golarion/data/hit_points_save_data.hpp"
#include "golarion/data/inventory_save_data.hpp"
#include "golarion/data/race_save_data.hpp"
#include "golarion/data/skills_save_data.hpp"

#include <string>
#include <vector>

namespace golarion
{
    struct CharacterSheetSaveData
    {
        int formatVersion;
        CharacterIdentitySaveData identity;
        RaceSaveData race;
        std::vector<AbilitySaveData> abilities;
        HitPointsSaveData hitPoints;
        SkillsSaveData skills;
        AttacksData attacks;
        ConditionManagerSaveData conditions;
        InventorySaveData inventory;
        FeatsData feats;
        LanguagesData languages;
        SpellcastingsData spellcastings;
        GrantedSpellsData grantedSpells;
        ClassesData classes;
        // Preserve an old level-one feat until a real class is assigned.
        bool legacyFirstLevelFeatSlot = false;
        EnhancementsData enhancements;
        std::vector<ItemDefinition> customItemDefinitions;
        std::vector<SpellDefinition> customSpellDefinitions;
        std::vector<FeatDefinition> customFeatDefinitions;
        std::string notes;
    };
}
