#include <iterator>
#include "golarion/character/character_sheet.hpp"

#include "conditions/condition_factories.hpp"
#include "golarion/persistence/json.hpp"
#include "golarion/character/prepared_spellcasting.hpp"
#include "golarion/character/spontaneous_spellcasting.hpp"
#include "golarion/character/flexible_spellcasting.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
    template<class Casting> Casting &castingAs(golarion::Spellcastings &castings, std::string_view id)
    {
        auto *casting = dynamic_cast<Casting *>(&castings.get(id));
        if (!casting) throw std::invalid_argument("operation is not supported by this spellcasting mode");
        return *casting;
    }
    std::size_t abilityIndex(golarion::AbilityType type)
    {
        switch (type)
        {
            case golarion::AbilityType::Strength:
                return 0;
            case golarion::AbilityType::Dexterity:
                return 1;
            case golarion::AbilityType::Constitution:
                return 2;
            case golarion::AbilityType::Intelligence:
                return 3;
            case golarion::AbilityType::Wisdom:
                return 4;
            case golarion::AbilityType::Charisma:
                return 5;
        }

        throw std::invalid_argument("unknown ability type");
    }
}

namespace golarion
{
    void CharacterSheet::addEnhancementGroup(EnhancementGroupDefinition data)
    {
        enhancements_.addGroup(std::move(data));
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::removeEnhancementGroup(std::string_view groupId)
    {
        enhancements_.removeGroup(groupId);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::addEnhancement(std::string_view groupId, EnhancementDefinition data)
    {
        enhancements_.add(groupId, std::move(data));
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::removeEnhancement(std::string_view groupId, std::string_view id)
    {
        enhancements_.remove(groupId, id);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::setEnhancementActive(std::string_view groupId, std::string_view id, bool active)
    {
        enhancements_.setActive(groupId, id, active);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::addContainer(ContainerDefinition definition)
    {
        inventory_->addContainer(std::move(definition));
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::addCustomSpellDefinition(SpellDefinition definition)
    {
        spellCatalog_.add(std::move(definition));
    }

    void CharacterSheet::addCustomItemDefinition(ItemDefinition definition)
    {
        itemCatalog_.add(std::move(definition));
    }

    void CharacterSheet::addItem(ItemInstanceDefinition definition, std::optional<std::string> containerId)
    {
        inventory_->addItem(std::move(definition), std::move(containerId));
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::equipItem(std::string_view itemId)
    {
        inventory_->equip(itemId);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::unequipItem(std::string_view itemId, std::optional<std::string> containerId)
    {
        inventory_->unequip(itemId, std::move(containerId));
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::moveItem(std::string_view itemId, std::optional<std::string> containerId)
    {
        inventory_->moveItem(itemId, std::move(containerId));
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::removeItem(std::string_view itemId)
    {
        inventory_->removeItem(itemId);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::addSpellToRepertoire(std::string_view castingId, std::string_view spellId)
    {
        spellcastings_.get(castingId).addToRepertoire(spellId);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::removeSpellFromRepertoire(std::string_view castingId, std::string_view spellId)
    {
        spellcastings_.get(castingId).removeFromRepertoire(spellId);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::selectRepertoireSpell(std::string_view castingId, RepertoireSelectionDefinition selection)
    {
        spellcastings_.get(castingId).selectRepertoireSpell(std::move(selection));
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::removeRepertoireSelection(std::string_view castingId, RepertoireSelectionDefinition selection)
    {
        spellcastings_.get(castingId).removeRepertoireSelection(std::move(selection));
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::prepareSpell(std::string_view castingId, const std::vector<SpellSlotId> &slots, const SpellPreparationDefinition &definition)
    {
        castingAs<PreparedSpellcasting>(spellcastings_, castingId).prepare(slots, definition);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::prepareFlexibleSpell(std::string_view castingId, const SpellSlotId &preparation, const SpellPreparationDefinition &definition)
    {
        castingAs<FlexibleSpellcasting>(spellcastings_, castingId).prepare(preparation, definition);
        feats_.recheckPrerequisites();
    }

    MetamagicEvaluation CharacterSheet::consumePreparedSpell(std::string_view castingId, const SpellSlotId &slot, const PreparedSpellConsumptionDefinition &definition)
    {
        auto result = castingAs<PreparedSpellcasting>(spellcastings_, castingId).consume(slot, definition);
        feats_.recheckPrerequisites();
        return result;
    }

    MetamagicEvaluation CharacterSheet::consumeSpontaneousSpell(std::string_view castingId, const SpellSlotId &slot, const SpontaneousSpellConsumptionDefinition &definition)
    {
        auto result = castingAs<SpontaneousSpellcasting>(spellcastings_, castingId).consume(slot, definition);
        feats_.recheckPrerequisites();
        return result;
    }

    MetamagicEvaluation CharacterSheet::consumeFlexibleSpell(std::string_view castingId, const SpellSlotId &slot, const FlexibleSpellConsumptionDefinition &definition)
    {
        auto result = castingAs<FlexibleSpellcasting>(spellcastings_, castingId).consume(slot, definition);
        feats_.recheckPrerequisites();
        return result;
    }

    void CharacterSheet::clearSpellSlot(std::string_view castingId, const SpellSlotId &slot)
    {
        auto &casting = spellcastings_.get(castingId);
        if (auto *prepared = dynamic_cast<PreparedSpellcasting *>(&casting)) prepared->clear(slot);
        else if (auto *spontaneous = dynamic_cast<SpontaneousSpellcasting *>(&casting)) spontaneous->clear(slot);
        else if (auto *flexible = dynamic_cast<FlexibleSpellcasting *>(&casting)) flexible->clearSlot(slot);
        else throw std::invalid_argument("unknown spellcasting mode");
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::clearFlexiblePreparation(std::string_view castingId, const SpellSlotId &preparation)
    {
        castingAs<FlexibleSpellcasting>(spellcastings_, castingId).clearPreparation(preparation);
        feats_.recheckPrerequisites();
    }

    CharacterSheet::CharacterSheet(const ClassDefinitionManager &classDefinitions)
        : actionManager_(resourceManager_),
          reminderManager_(resourceManager_),
          abilityChecks_(resourceManager_),
          baseAttackBonus_(resourceManager_),
          abilities_{
              AbilityScore(AbilityType::Strength),
              AbilityScore(AbilityType::Dexterity),
              AbilityScore(AbilityType::Constitution),
              AbilityScore(AbilityType::Intelligence),
              AbilityScore(AbilityType::Wisdom),
              AbilityScore(AbilityType::Charisma)
          },
          strikes_(resourceManager_),
          attackRoutines_(resourceManager_, actionManager_, strikes_),
          attacks_(attackRoutines_, strikes_),
          combatManeuvers_(resourceManager_),
          hitPoints_(resourceManager_),
          initiative_(resourceManager_),
          armorClass_(resourceManager_),
          skills_(resourceManager_),
          savingThrows_(resourceManager_),
          specialDefenses_(resourceManager_),
          movement_(resourceManager_),
          carryingCapacity_(resourceManager_),
          encumbrance_(resourceManager_, carryingCapacity_),
          sizeManager_(resourceManager_),
          languages_(resourceManager_, [this] { return skills_.skill(SkillType::Linguistics).toSaveData().ranks; }),
          spellcastings_(resourceManager_, &spellCatalog_),
          grantedSpells_(resourceManager_, &spellCatalog_),
          favoredClasses_(resourceManager_),
          feats_(resourceManager_, &featCatalog_),
          conditionManager_(resourceManager_),
          race_(resourceManager_),
          classes_(resourceManager_, favoredClasses_, classDefinitions),
          enhancements_(resourceManager_)
    {
        for (AbilityScore &abilityScore : abilities_)
        {
            abilityScore.registerResources(resourceManager_);
        }
        resourceManager_.registerTarget("level", [this]
        {
            return level();
        });
        registerCanonicalConditions(conditionManager_);
        feats_.setLevel(level());
        hitPoints_.connectConditionEntries();
        inventory_.emplace(resourceManager_, itemCatalog_);
    }

    CharacterSheet::CharacterSheet(const CharacterSheetSaveData &data, const ClassDefinitionManager &classDefinitions) : CharacterSheet(classDefinitions)
    {
        if (data.formatVersion != 8 && data.formatVersion != 10 && data.formatVersion != 11 && data.formatVersion != 12 && data.formatVersion != 13 && data.formatVersion != 14 && data.formatVersion != 15 && data.formatVersion != 16 && data.formatVersion != 17 && data.formatVersion != 18 && data.formatVersion != 19 && data.formatVersion != 20 && data.formatVersion != 21 && data.formatVersion != 22 && data.formatVersion != 23 && data.formatVersion != 24 && data.formatVersion != 25 && data.formatVersion != 26 && data.formatVersion != 27 && data.formatVersion != 28 && data.formatVersion != 29 && data.formatVersion != 30 && data.formatVersion != 31 && data.formatVersion != 32 && data.formatVersion != 33 && data.formatVersion != 34 && data.formatVersion != 35 && data.formatVersion != 36 && data.formatVersion != 37 && data.formatVersion != 38 && data.formatVersion != 39 && data.formatVersion != 40 && data.formatVersion != 41)
        {
            throw std::invalid_argument("unsupported character sheet format version: " + std::to_string(data.formatVersion));
        }
        if (data.abilities.size() != AbilityCount)
        {
            throw std::invalid_argument("character sheet save must contain exactly six abilities");
        }

        std::array<bool, AbilityCount> loadedAbilities{};
        for (const AbilitySaveData &abilityData : data.abilities)
        {
            const std::size_t index = abilityIndex(abilityData.type);
            if (loadedAbilities[index])
            {
                throw std::invalid_argument("ability is duplicated in character sheet save");
            }

            abilities_[index].setBaseValue(abilityData.baseValue);
            loadedAbilities[index] = true;
        }

        if (data.formatVersion >= 39)
            for (const auto &definition : data.customFeatDefinitions) featCatalog_.add(definition);
        if (data.formatVersion >= 38)
            for (const auto &definition : data.customSpellDefinitions) spellCatalog_.add(definition);
        notes_ = data.formatVersion >= 41 ? data.notes : std::string{};
        identity_ = CharacterIdentity(data.identity);
        if (data.formatVersion >= 30) classes_.load(data.classes);
        legacyFirstLevelFeatSlot_ = data.formatVersion >= 30 ? data.legacyFirstLevelFeatSlot : std::ranges::any_of(data.feats.selections, [](const auto &selection) { return selection.slotId == "base.feat.level1"; });
        if (legacyFirstLevelFeatSlot_ && level() != 0) throw std::invalid_argument("legacy feat slot cannot coexist with class levels");
        feats_.setLevel(legacyFirstLevelFeatSlot_ ? 1 : level());
        race_.load(data.race);
        skills_.load(data.skills);
        attacks_.load(data.attacks);
        conditionManager_.load(data.conditions);
        if (data.formatVersion >= 37)
            for (const auto &definition : data.customItemDefinitions) itemCatalog_.add(definition);
        inventory_->load(data.inventory);
        classes_.loadFavoredClasses(data.classes.favoredClasses);
        if (data.formatVersion >= 36)
            for (const auto &group : data.enhancements.groups)
            {
                EnhancementGroupDefinition definition{group.id, group.name, {}};
                for (const auto &entry : group.enhancements)
                    definition.enhancements.push_back({entry.definition, entry.active});
                enhancements_.addGroup(std::move(definition));
            }
        feats_.load(data.feats);
        hitPoints_.load(data.hitPoints);
        languages_.load(data.languages);
        hitPoints_.reconcileConditionEntries();
        feats_.recheckPrerequisites();
        if (data.formatVersion >= 27) spellcastings_.load(data.spellcastings);
        if (data.formatVersion >= 29) grantedSpells_.load(data.grantedSpells);
    }

    CharacterSheet CharacterSheet::load(const std::filesystem::path &path, const ClassDefinitionManager &classDefinitions)
    {
        return CharacterSheet(persistence::loadFromFile(path), classDefinitions);
    }

    void CharacterSheet::setNotes(std::string notes)
    {
        notes_ = std::move(notes);
    }

    void CharacterSheet::setName(std::string_view name)
    {
        identity_.setName(name);
    }

    void CharacterSheet::consumeGrantedSpell(std::string_view id) { grantedSpells_.consume(id); }
    void CharacterSheet::refillGrantedSpell(std::string_view id) { grantedSpells_.refill(id); }
    void CharacterSheet::restoreGrantedSpell(std::string_view id, int amount) { grantedSpells_.restore(id, amount); }
    void CharacterSheet::refillItemUsagePool(std::string_view itemId, std::string_view poolId) { inventory_->refillUsagePool(itemId, poolId); }
    void CharacterSheet::restoreItemUsagePool(std::string_view itemId, std::string_view poolId, int amount) { inventory_->restoreUsagePool(itemId, poolId, amount); }

    void CharacterSheet::setPlayerName(std::string_view playerName)
    {
        identity_.setPlayerName(playerName);
    }

    void CharacterSheet::setAlignment(std::optional<Alignment> alignment)
    {
        identity_.setAlignment(alignment);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::setDeity(std::optional<std::string> deity)
    {
        identity_.setDeity(std::move(deity));
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::setHomeland(std::optional<std::string> homeland)
    {
        identity_.setHomeland(std::move(homeland));
    }

    void CharacterSheet::setGender(std::optional<std::string> gender)
    {
        identity_.setGender(std::move(gender));
    }

    void CharacterSheet::setAge(std::optional<int> age)
    {
        identity_.setAge(age);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::setHeightCentimeters(std::optional<int> heightCentimeters)
    {
        identity_.setHeightCentimeters(heightCentimeters);
    }

    void CharacterSheet::setWeightGrams(std::optional<std::int64_t> weightGrams)
    {
        identity_.setWeightGrams(weightGrams);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::setHair(std::optional<std::string> hair)
    {
        identity_.setHair(std::move(hair));
    }

    void CharacterSheet::setEyes(std::optional<std::string> eyes)
    {
        identity_.setEyes(std::move(eyes));
    }

    void CharacterSheet::setAppearance(std::optional<std::string> appearance)
    {
        identity_.setAppearance(std::move(appearance));
    }

    void CharacterSheet::setRace(std::string_view raceDefinitionId)
    {
        race_.setRace(raceDefinitionId);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::clearRace()
    {
        race_.clearRace();
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::addCustomFeatDefinition(FeatDefinition definition)
    {
        featCatalog_.add(std::move(definition));
    }

    void CharacterSheet::addExtraFeatSlot(std::string_view slotId)
    {
        feats_.addExtraSlot(slotId);
    }

    void CharacterSheet::removeExtraFeatSlot(std::string_view slotId)
    {
        feats_.removeExtraSlot(slotId);
    }

    void CharacterSheet::selectFeat(std::string_view slotId, std::string_view featId, FeatChoices choices)
    {
        feats_.selectFeat(slotId, featId, std::move(choices));
    }

    void CharacterSheet::removeFeat(std::string_view slotId)
    {
        feats_.removeFeat(slotId);
    }

    void CharacterSheet::recheckFeatPrerequisites()
    {
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::addLanguage(std::string_view languageId, LanguageOrigin origin)
    {
        languages_.add(languageId, origin);
    }

    void CharacterSheet::removeLanguage(std::string_view languageId)
    {
        languages_.remove(languageId);
    }

    void CharacterSheet::selectAlternateRacialFeature(std::string_view featureId)
    {
        race_.selectAlternateFeature(featureId);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::removeAlternateRacialFeature(std::string_view featureId)
    {
        race_.removeAlternateFeature(featureId);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::setRacialChoice(std::string_view elementId, std::string_view choiceId, std::vector<std::string> optionIds)
    {
        race_.setChoice(elementId, choiceId, std::move(optionIds));
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::setAbilityBaseValue(AbilityType type, int baseValue)
    {
        ability(type).setBaseValue(baseValue);
        if (type == AbilityType::Constitution)
        {
            hitPoints_.initializeConditionEntries();
        }
        feats_.recheckPrerequisites();
        if (type == AbilityType::Intelligence) languages_.refreshReminders();
    }

    void CharacterSheet::heal(int amount)
    {
        hitPoints_.heal(amount);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::damage(int amount, DamageLethality lethality)
    {
        hitPoints_.damage(amount, lethality);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::setSkillRanks(SkillType type, int ranks)
    {
        skills_.setRanks(type, ranks);
        languages_.refreshReminders();
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::addSkillSpecialization(SkillType type, const SkillSpecializationDefinition &definition)
    {
        skills_.addSpecialization(type, definition);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::removeSkillSpecialization(SkillType type, const std::string &specializationId)
    {
        skills_.removeSpecialization(type, specializationId);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::setSkillSpecializationRanks(SkillType type, const std::string &specializationId, int ranks)
    {
        skills_.setSpecializationRanks(type, specializationId, ranks);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::createAttack(const AttackDefinition &definition)
    {
        attacks_.create(definition);
    }

    void CharacterSheet::removeAttack(std::string_view attackId)
    {
        attacks_.remove(attackId);
    }

    void CharacterSheet::assignStrikeToAttack(std::string_view attackId, std::string_view slotId, std::string_view strikeGrantId)
    {
        attacks_.assignStrike(attackId, slotId, strikeGrantId);
    }

    void CharacterSheet::unassignStrikeFromAttack(std::string_view attackId, std::string_view slotId)
    {
        attacks_.unassignStrike(attackId, slotId);
    }

    void CharacterSheet::addCondition(ConditionEntry entry)
    {
        conditionManager_.addManualEntry(std::move(entry));
        hitPoints_.reconcileConditionEntries();
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::removeCondition(std::string_view entryId)
    {
        conditionManager_.removeManualEntry(entryId);
        hitPoints_.reconcileConditionEntries();
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::addMoney(CoinDenomination denomination, int quantity, std::string_view containerId)
    {
        inventory_->addMoney(denomination, quantity, containerId);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::removeMoney(CoinDenomination denomination, int quantity, std::string_view containerId)
    {
        inventory_->removeMoney(denomination, quantity, containerId);
        feats_.recheckPrerequisites();
    }

    CharacterSheetView CharacterSheet::toView()
    {
        return toView(StrikeCalculationContext{});
    }

    CharacterSheetView CharacterSheet::toView(const StrikeCalculationContext &strikeContext)
    {
        languages_.refreshReminders();
        hitPoints_.reconcileConditionEntries();
        feats_.recheckPrerequisites();
        EncumbranceView encumbranceView = encumbrance_.toView();
        std::vector<AbilityView> abilityViews;
        abilityViews.reserve(abilities_.size());

        for (const AbilityScore &abilityScore : abilities_)
        {
            abilityViews.push_back(abilityScore.toView(resourceManager_));
        }

        return CharacterSheetView{
            .identity = identity_.toView(),
            .race = race_.toView(),
            .feats = feats_.toView(),
            .languages = languages_.toView(),
            .grantedSpells = grantedSpells_.toView(),
            .actions = actionManager_.toView(),
            .reminders = reminderManager_.toView(),
            .abilities = std::move(abilityViews),
            .baseAttackBonus = baseAttackBonus_.toView(),
            .strikes = strikes_.toView(strikeContext),
            .attackRoutines = attackRoutines_.toView(),
            .attacks = attacks_.toView(strikeContext),
            .combatManeuvers = combatManeuvers_.toView(),
            .hitPoints = hitPoints_.toView(),
            .initiative = initiative_.toView(),
            .armorClass = armorClass_.toView(),
            .savingThrows = savingThrows_.toView(),
            .specialDefenses = specialDefenses_.toView(),
            .skills = skills_.toView(),
            .movement = movement_.toView(),
            .carryingCapacity = carryingCapacity_.toView(),
            .encumbrance = std::move(encumbranceView),
            .inventory = inventory_->toView(),
            .size = sizeManager_.toView(),
            .conditions = conditionManager_.toView(),
            .classes = classes_.toView(),
            .legacyFirstLevelFeatSlot = legacyFirstLevelFeatSlot_,
            .spellcastings = spellcastings_.toView(),
            .enhancements = enhancements_.toView(),
            .customItemDefinitions = itemCatalog_.customDefinitions(),
            .customSpellDefinitions = spellCatalog_.customDefinitions(),
            .customFeatDefinitions = featCatalog_.customDefinitions(),
            .notes = notes_
        };
    }

    CharacterSheetSaveData CharacterSheet::toSaveData() const
    {
        std::vector<AbilitySaveData> abilityData;
        abilityData.reserve(abilities_.size());

        for (const AbilityScore &abilityScore : abilities_)
        {
            abilityData.push_back(abilityScore.toSaveData());
        }

        return CharacterSheetSaveData{
            .formatVersion = 41,
            .identity = identity_.toSaveData(),
            .race = race_.toSaveData(),
            .abilities = std::move(abilityData),
            .hitPoints = hitPoints_.toSaveData(),
            .skills = skills_.toSaveData(),
            .attacks = attacks_.toData(),
            .conditions = conditionManager_.toSaveData(),
            .inventory = inventory_->toSaveData(),
            .feats = feats_.toData(),
            .languages = languages_.toData(),
            .spellcastings = spellcastings_.toData(),
            .grantedSpells = grantedSpells_.toData(),
            .classes = classes_.toData(),
            .legacyFirstLevelFeatSlot = legacyFirstLevelFeatSlot_,
            .enhancements = enhancements_.toData(),
            .customItemDefinitions = itemCatalog_.customDefinitions(),
            .customSpellDefinitions = spellCatalog_.customDefinitions(),
            .customFeatDefinitions = featCatalog_.customDefinitions(),
            .notes = notes_
        };
    }

    void CharacterSheet::save(const std::filesystem::path &path) const
    {
        persistence::saveToFile(toSaveData(), path);
    }

    AbilityScore &CharacterSheet::ability(AbilityType type)
    {
        return abilities_[abilityIndex(type)];
    }

    int CharacterSheet::level() const
    {
        return classes_.level();
    }

    void CharacterSheet::addClassArchetype(CharacterClassType type, const std::string &archetypeId)
    {
        classes_.addArchetype(type, archetypeId);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::removeClassArchetype(CharacterClassType type, const std::string &archetypeId)
    {
        classes_.removeArchetype(type, archetypeId);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::addClassLevel(CharacterClassType type, std::optional<int> hitDieResult)
    {
        if (std::ranges::any_of(pendingChoices(), [](const PendingChoice &choice) { return choice.blocksAdvancement; }))
            throw std::invalid_argument("cannot advance while required choices are incomplete");
        classes_.addLevel(type, hitDieResult);
        legacyFirstLevelFeatSlot_ = false;
        feats_.setLevel(level());
        if (hitDieResult) hitPoints_.initializeConditionEntries();
        else hitPoints_.reconcileConditionEntries();
    }

    void CharacterSheet::removeClassLevel(CharacterClassType type)
    {
        classes_.removeLevel(type);
        feats_.setLevel(level());
        hitPoints_.reconcileConditionEntries();
    }

    void CharacterSheet::setFavoredClass(CharacterClassType type) { setFavoredClasses({type}); }

    void CharacterSheet::setFavoredClasses(std::set<CharacterClassType> types)
    {
        classes_.setFavoredClasses(std::move(types));
        feats_.recheckPrerequisites();
        hitPoints_.reconcileConditionEntries();
    }

    void CharacterSheet::selectFavoredClassBonus(CharacterClassType type, int classLevel, std::string optionId)
    {
        classes_.selectFavoredClassBonus(type, classLevel, std::move(optionId));
        feats_.recheckPrerequisites();
        hitPoints_.initializeConditionEntries();
    }

    void CharacterSheet::clearFavoredClassBonus(CharacterClassType type, int classLevel)
    {
        classes_.clearFavoredClassBonus(type, classLevel);
        feats_.recheckPrerequisites();
        hitPoints_.reconcileConditionEntries();
    }

    void CharacterSheet::setClassHitDieResult(CharacterClassType type, int classLevel, int result)
    {
        classes_.setHitDieResult(type, classLevel, result);
        feats_.recheckPrerequisites();
        hitPoints_.initializeConditionEntries();
    }

    void CharacterSheet::selectClassOption(CharacterClassType type, const ClassOptionDefinition &definition)
    {
        classes_.selectOption(type, definition);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::unselectClassOption(CharacterClassType type, const ClassOptionDefinition &definition)
    {
        classes_.unselectOption(type, definition);
        feats_.recheckPrerequisites();
    }
    std::vector<PendingChoice> CharacterSheet::pendingChoices()
    {
        auto result = identity_.pendingChoices();
        const auto append = [&](std::vector<PendingChoice> choices)
        {
            result.insert(result.end(), std::make_move_iterator(choices.begin()), std::make_move_iterator(choices.end()));
        };
        append(race_.pendingChoices());
        append(classes_.pendingChoices());
        append(feats_.pendingChoices());
        append(skills_.pendingChoices());
        append(languages_.pendingChoices());
        append(spellcastings_.pendingChoices());
        return result;
    }

}
