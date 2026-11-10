#include "golarion/character/character_sheet.hpp"

#include "conditions/condition_factories.hpp"
#include "golarion/persistence/json.hpp"
#include <algorithm>
#include <stdexcept>
#include <utility>

namespace
{
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
          spellcastings_(resourceManager_),
          grantedSpells_(resourceManager_),
          favoredClasses_(resourceManager_),
          feats_(resourceManager_),
          conditionManager_(resourceManager_),
          race_(resourceManager_),
          classes_(resourceManager_, favoredClasses_, classDefinitions)
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
        inventory_.emplace(resourceManager_);
    }

    CharacterSheet::CharacterSheet(const CharacterSheetSaveData &data, const ClassDefinitionManager &classDefinitions) : CharacterSheet(classDefinitions)
    {
        if (data.formatVersion != 8 && data.formatVersion != 10 && data.formatVersion != 11 && data.formatVersion != 12 && data.formatVersion != 13 && data.formatVersion != 14 && data.formatVersion != 15 && data.formatVersion != 16 && data.formatVersion != 17 && data.formatVersion != 18 && data.formatVersion != 19 && data.formatVersion != 20 && data.formatVersion != 21 && data.formatVersion != 22 && data.formatVersion != 23 && data.formatVersion != 24 && data.formatVersion != 25 && data.formatVersion != 26 && data.formatVersion != 27 && data.formatVersion != 28 && data.formatVersion != 29 && data.formatVersion != 30 && data.formatVersion != 31 && data.formatVersion != 32 && data.formatVersion != 33)
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

        identity_ = CharacterIdentity(data.identity);
        if (data.formatVersion >= 30) classes_.load(data.classes);
        legacyFirstLevelFeatSlot_ = data.formatVersion >= 30 ? data.legacyFirstLevelFeatSlot : std::ranges::any_of(data.feats.selections, [](const auto &selection) { return selection.slotId == "base.feat.level1"; });
        if (legacyFirstLevelFeatSlot_ && level() != 0) throw std::invalid_argument("legacy feat slot cannot coexist with class levels");
        feats_.setLevel(legacyFirstLevelFeatSlot_ ? 1 : level());
        race_.load(data.race);
        skills_.load(data.skills);
        attacks_.load(data.attacks);
        conditionManager_.load(data.conditions);
        inventory_->load(data.inventory);
        classes_.loadFavoredClasses(data.classes.favoredClasses);
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

    void CharacterSheet::setLanguageIntelligenceBonus(int bonus)
    {
        languages_.setIntelligenceBonus(bonus);
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

    void CharacterSheet::addSkillSpecialization(SkillType type, const std::string &specializationId, const std::string &specialization)
    {
        skills_.addSpecialization(type, specializationId, specialization);
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

    void CharacterSheet::createAttack(std::string_view id, std::string_view name, std::string_view routineId)
    {
        attacks_.create(id, name, routineId);
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
            .spellcastings = spellcastings_.toView()
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
            .formatVersion = 33,
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
            .legacyFirstLevelFeatSlot = legacyFirstLevelFeatSlot_
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
        classes_.addLevel(type, hitDieResult);
        legacyFirstLevelFeatSlot_ = false;
        feats_.setLevel(level());
        if (hitDieResult) hitPoints_.initializeConditionEntries();
        else hitPoints_.reconcileConditionEntries();
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

    void CharacterSheet::selectClassOption(CharacterClassType type, const std::string &acquisitionId, const std::string &choiceId, const std::string &optionId)
    {
        classes_.selectOption(type, acquisitionId, choiceId, optionId);
        feats_.recheckPrerequisites();
    }

    void CharacterSheet::unselectClassOption(CharacterClassType type, const std::string &acquisitionId, const std::string &choiceId, const std::string &optionId)
    {
        classes_.unselectOption(type, acquisitionId, choiceId, optionId);
        feats_.recheckPrerequisites();
    }
}
