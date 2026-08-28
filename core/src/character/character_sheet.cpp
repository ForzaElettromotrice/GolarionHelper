#include "golarion/character/character_sheet.hpp"

#include "conditions/condition_factories.hpp"
#include "golarion/persistence/json.hpp"
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
    CharacterSheet::CharacterSheet()
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
          conditionManager_(resourceManager_)
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
        hitPoints_.connectConditionEntries();
        inventory_.emplace(resourceManager_);
    }

    CharacterSheet::CharacterSheet(const CharacterSheetSaveData &data) : CharacterSheet()
    {
        if (data.formatVersion != 8 && data.formatVersion != 10 && data.formatVersion != 11 && data.formatVersion != 12 && data.formatVersion != 13 && data.formatVersion != 14 && data.formatVersion != 15 && data.formatVersion != 16 && data.formatVersion != 17 && data.formatVersion != 18 && data.formatVersion != 19 && data.formatVersion != 20 && data.formatVersion != 21 && data.formatVersion != 22)
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
        skills_.load(data.skills);
        hitPoints_.load(data.hitPoints);
        attacks_.load(data.attacks);
        conditionManager_.load(data.conditions);
        inventory_->load(data.inventory);
        hitPoints_.reconcileConditionEntries();
    }

    CharacterSheet CharacterSheet::load(const std::filesystem::path &path)
    {
        return CharacterSheet(persistence::loadFromFile(path));
    }

    void CharacterSheet::setName(std::string_view name)
    {
        identity_.setName(name);
    }

    void CharacterSheet::setPlayerName(std::string_view playerName)
    {
        identity_.setPlayerName(playerName);
    }

    void CharacterSheet::setAlignment(std::optional<Alignment> alignment)
    {
        identity_.setAlignment(alignment);
    }

    void CharacterSheet::setDeity(std::optional<std::string> deity)
    {
        identity_.setDeity(std::move(deity));
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
    }

    void CharacterSheet::setHeightCentimeters(std::optional<int> heightCentimeters)
    {
        identity_.setHeightCentimeters(heightCentimeters);
    }

    void CharacterSheet::setWeightGrams(std::optional<std::int64_t> weightGrams)
    {
        identity_.setWeightGrams(weightGrams);
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

    void CharacterSheet::setAbilityBaseValue(AbilityType type, int baseValue)
    {
        ability(type).setBaseValue(baseValue);
        if (type == AbilityType::Constitution)
        {
            hitPoints_.initializeConditionEntries();
        }
    }

    void CharacterSheet::heal(int amount)
    {
        hitPoints_.heal(amount);
    }

    void CharacterSheet::damage(int amount, DamageLethality lethality)
    {
        hitPoints_.damage(amount, lethality);
    }

    void CharacterSheet::setSkillRanks(SkillType type, int ranks)
    {
        skills_.setRanks(type, ranks);
    }

    void CharacterSheet::addSkillSpecialization(SkillType type, const std::string &specializationId, const std::string &specialization)
    {
        skills_.addSpecialization(type, specializationId, specialization);
    }

    void CharacterSheet::removeSkillSpecialization(SkillType type, const std::string &specializationId)
    {
        skills_.removeSpecialization(type, specializationId);
    }

    void CharacterSheet::setSkillSpecializationRanks(SkillType type, const std::string &specializationId, int ranks)
    {
        skills_.setSpecializationRanks(type, specializationId, ranks);
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
    }

    void CharacterSheet::removeCondition(std::string_view entryId)
    {
        conditionManager_.removeManualEntry(entryId);
        hitPoints_.reconcileConditionEntries();
    }

    void CharacterSheet::addMoney(CoinDenomination denomination, int quantity, std::string_view containerId)
    {
        inventory_->addMoney(denomination, quantity, containerId);
    }

    void CharacterSheet::removeMoney(CoinDenomination denomination, int quantity, std::string_view containerId)
    {
        inventory_->removeMoney(denomination, quantity, containerId);
    }

    CharacterSheetView CharacterSheet::toView()
    {
        return toView(StrikeCalculationContext{});
    }

    CharacterSheetView CharacterSheet::toView(const StrikeCalculationContext &strikeContext)
    {
        hitPoints_.reconcileConditionEntries();
        EncumbranceView encumbranceView = encumbrance_.toView();
        std::vector<AbilityView> abilityViews;
        abilityViews.reserve(abilities_.size());

        for (const AbilityScore &abilityScore : abilities_)
        {
            abilityViews.push_back(abilityScore.toView(resourceManager_));
        }

        return CharacterSheetView{
            .identity = identity_.toView(),
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
            .conditions = conditionManager_.toView()
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
            .formatVersion = 22,
            .identity = identity_.toSaveData(),
            .abilities = std::move(abilityData),
            .hitPoints = hitPoints_.toSaveData(),
            .skills = skills_.toSaveData(),
            .attacks = attacks_.toData(),
            .conditions = conditionManager_.toSaveData(),
            .inventory = inventory_->toSaveData()
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
        return 1;
    }
}
