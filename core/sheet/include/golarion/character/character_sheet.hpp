#pragma once
#include "golarion/character/pending_choice.hpp"
#include "golarion/character/spell_catalog.hpp"
#include "golarion/character/feat_catalog.hpp"
#include "golarion/character/class_option_definition.hpp"
#include "golarion/character/attack_definition.hpp"
#include "golarion/character/skill_specialization_definition.hpp"
#include "golarion/character/spell_operation_definition.hpp"
#include "golarion/character/enhancements.hpp"

#include "golarion/character/feats.hpp"
#include "golarion/character/classes.hpp"
#include "golarion/character/languages.hpp"
#include "golarion/character/spellcastings.hpp"
#include "golarion/character/granted_spells.hpp"

#include "golarion/character/action.hpp"
#include "golarion/character/ability.hpp"
#include "golarion/character/armor_class.hpp"
#include "golarion/character/attack.hpp"
#include "golarion/character/attack_routine.hpp"
#include "golarion/character/strike.hpp"
#include "golarion/character/base_attack_bonus.hpp"
#include "golarion/character/carrying_capacity.hpp"
#include "golarion/character/character_identity.hpp"
#include "golarion/character/combat_maneuvers.hpp"
#include "golarion/character/condition.hpp"
#include "golarion/character/encumbrance.hpp"
#include "golarion/character/initiative.hpp"
#include "golarion/character/hit_points.hpp"
#include "golarion/character/movement.hpp"
#include "golarion/character/race.hpp"
#include "golarion/character/reminder.hpp"
#include "golarion/character/saving_throws.hpp"
#include "golarion/character/size.hpp"
#include "golarion/character/skills.hpp"
#include "golarion/character/special_defenses.hpp"
#include "golarion/data/character_sheet_save_data.hpp"
#include "golarion/equipment/inventory.hpp"
#include "golarion/view/character_sheet_view.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <array>
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    class CharacterSheet final
    {
    public:
        explicit CharacterSheet(const ClassDefinitionManager &classDefinitions = ClassDefinitionManager::instance());
        static CharacterSheet load(const std::filesystem::path &path, const ClassDefinitionManager &classDefinitions = ClassDefinitionManager::instance());
        void addClassLevel(CharacterClassType type, std::optional<int> hitDieResult = std::nullopt);
        void removeClassLevel(CharacterClassType type);
        void setClassHitDieResult(CharacterClassType type, int classLevel, int result);
        void setFavoredClass(CharacterClassType type);
        void setFavoredClasses(std::set<CharacterClassType> types);
        void selectFavoredClassBonus(CharacterClassType type, int classLevel, std::string optionId);
        void clearFavoredClassBonus(CharacterClassType type, int classLevel);
        void addClassArchetype(CharacterClassType type, const std::string &archetypeId);
        void removeClassArchetype(CharacterClassType type, const std::string &archetypeId);
        int level() const;
        void addEnhancementGroup(EnhancementGroupDefinition data);
        void removeEnhancementGroup(std::string_view groupId);
        void addEnhancement(std::string_view groupId, EnhancementDefinition data);
        void removeEnhancement(std::string_view groupId, std::string_view id);
        void setEnhancementActive(std::string_view groupId, std::string_view id, bool active);
        void selectClassOption(CharacterClassType type, const ClassOptionDefinition &definition);
        void unselectClassOption(CharacterClassType type, const ClassOptionDefinition &definition);

        void setNotes(std::string notes);
        void setName(std::string_view name);
        void setPlayerName(std::string_view playerName);
        void setAlignment(std::optional<Alignment> alignment);
        void setDeity(std::optional<std::string> deity);
        void setHomeland(std::optional<std::string> homeland);
        void setGender(std::optional<std::string> gender);
        void setAge(std::optional<int> age);
        void setHeightCentimeters(std::optional<int> heightCentimeters);
        void setWeightGrams(std::optional<std::int64_t> weightGrams);
        void setHair(std::optional<std::string> hair);
        void setEyes(std::optional<std::string> eyes);
        void setAppearance(std::optional<std::string> appearance);
        void setRace(std::string_view raceDefinitionId);
        void clearRace();
        void addCustomFeatDefinition(FeatDefinition definition);
        void addExtraFeatSlot(std::string_view slotId);
        void removeExtraFeatSlot(std::string_view slotId);
        void selectFeat(std::string_view slotId, std::string_view featId, FeatChoices choices = {});
        void removeFeat(std::string_view slotId);
        void recheckFeatPrerequisites();
        void addLanguage(std::string_view languageId, LanguageOrigin origin);
        void removeLanguage(std::string_view languageId);
        void addSpellToRepertoire(std::string_view castingId, std::string_view spellId);
        void removeSpellFromRepertoire(std::string_view castingId, std::string_view spellId);
        void selectRepertoireSpell(std::string_view castingId, RepertoireSelectionDefinition selection);
        void removeRepertoireSelection(std::string_view castingId, RepertoireSelectionDefinition selection);
        void prepareSpell(std::string_view castingId, const std::vector<SpellSlotId> &slots, const SpellPreparationDefinition &definition);
        void prepareFlexibleSpell(std::string_view castingId, const SpellSlotId &preparation, const SpellPreparationDefinition &definition);
        MetamagicEvaluation consumePreparedSpell(std::string_view castingId, const SpellSlotId &slot, const PreparedSpellConsumptionDefinition &definition = {});
        MetamagicEvaluation consumeSpontaneousSpell(std::string_view castingId, const SpellSlotId &slot, const SpontaneousSpellConsumptionDefinition &definition);
        MetamagicEvaluation consumeFlexibleSpell(std::string_view castingId, const SpellSlotId &slot, const FlexibleSpellConsumptionDefinition &definition);
        void clearSpellSlot(std::string_view castingId, const SpellSlotId &slot);
        void clearFlexiblePreparation(std::string_view castingId, const SpellSlotId &preparation);
        void consumeGrantedSpell(std::string_view id);
        void refillGrantedSpell(std::string_view id);
        void restoreGrantedSpell(std::string_view id, int amount);
        void refillItemUsagePool(std::string_view itemId, std::string_view poolId);
        void restoreItemUsagePool(std::string_view itemId, std::string_view poolId, int amount);
        void selectAlternateRacialFeature(std::string_view featureId);
        void removeAlternateRacialFeature(std::string_view featureId);
        void setRacialChoice(std::string_view elementId, std::string_view choiceId, std::vector<std::string> optionIds);
        void setAbilityBaseValue(AbilityType type, int baseValue);
        void heal(int amount);
        void damage(int amount, DamageLethality lethality);
        void setSkillRanks(SkillType type, int ranks);
        void addSkillSpecialization(SkillType type, const SkillSpecializationDefinition &definition);
        void removeSkillSpecialization(SkillType type, const std::string &specializationId);
        void setSkillSpecializationRanks(SkillType type, const std::string &specializationId, int ranks);
        void createAttack(const AttackDefinition &definition);
        void removeAttack(std::string_view attackId);
        void assignStrikeToAttack(std::string_view attackId, std::string_view slotId, std::string_view strikeGrantId);
        void unassignStrikeFromAttack(std::string_view attackId, std::string_view slotId);
        void addCondition(ConditionEntry entry);
        void removeCondition(std::string_view entryId);
        void addMoney(CoinDenomination denomination, int quantity, std::string_view containerId);
        void addContainer(ContainerDefinition definition);
        void addCustomSpellDefinition(SpellDefinition definition);
        void addCustomItemDefinition(ItemDefinition definition);
        void addItem(ItemInstanceDefinition definition, std::optional<std::string> containerId = std::nullopt);
        void equipItem(std::string_view itemId);
        void unequipItem(std::string_view itemId, std::optional<std::string> containerId = std::nullopt);
        void moveItem(std::string_view itemId, std::optional<std::string> containerId = std::nullopt);
        void removeItem(std::string_view itemId);
        void removeMoney(CoinDenomination denomination, int quantity, std::string_view containerId);
        CharacterSheetView toView();
        CharacterSheetView toView(const StrikeCalculationContext &strikeContext);
        CharacterSheetSaveData toSaveData() const;
        void save(const std::filesystem::path &path) const;

        std::vector<PendingChoice> pendingChoices();

    private:
        static constexpr std::size_t AbilityCount = 6;

        CharacterSheet(const CharacterSheetSaveData &data, const ClassDefinitionManager &classDefinitions);
        AbilityScore &ability(AbilityType type);

        ResourceManager resourceManager_;
        CharacterIdentity identity_;
        ActionManager actionManager_;
        ReminderManager reminderManager_;
        AbilityChecks abilityChecks_;
        BaseAttackBonus baseAttackBonus_;
        std::array<AbilityScore, AbilityCount> abilities_;
        Strikes strikes_;
        AttackRoutines attackRoutines_;
        Attacks attacks_;
        CombatManeuvers combatManeuvers_;
        HitPoints hitPoints_;
        Initiative initiative_;
        ArmorClass armorClass_;
        Skills skills_;
        SavingThrows savingThrows_;
        SpecialDefenses specialDefenses_;
        Movement movement_;
        CarryingCapacity carryingCapacity_;
        Encumbrance encumbrance_;
        SizeManager sizeManager_;
        Languages languages_;
        SpellCatalog spellCatalog_;
        Spellcastings spellcastings_;
        GrantedSpells grantedSpells_;
        FavoredClasses favoredClasses_;
        FeatCatalog featCatalog_;
        Feats feats_;
        ConditionManager conditionManager_;
        ItemCatalog itemCatalog_;
        std::optional<Inventory> inventory_;
        Race race_;
        Classes classes_;
        Enhancements enhancements_;
        bool legacyFirstLevelFeatSlot_ = false;
        std::string notes_;
    };
}
