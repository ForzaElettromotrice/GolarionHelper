#include "golarion/character/character_sheet.hpp"

#include "golarion/persistence/json.hpp"
#include "golarion/resource/contribution.hpp"
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
        : baseAttackBonus_(resourceManager_),
          abilities_{
              AbilityScore(AbilityType::Strength),
              AbilityScore(AbilityType::Dexterity),
              AbilityScore(AbilityType::Constitution),
              AbilityScore(AbilityType::Intelligence),
              AbilityScore(AbilityType::Wisdom),
              AbilityScore(AbilityType::Charisma)
          },
          attacks_(resourceManager_),
          combatManeuvers_(resourceManager_),
          hitPoints_(resourceManager_),
          initiative_(resourceManager_),
          armorClass_(resourceManager_),
          skills_(resourceManager_),
          savingThrows_(resourceManager_),
          movement_(resourceManager_)
    {
        for (AbilityScore &abilityScore : abilities_)
        {
            abilityScore.registerResources(resourceManager_);
        }
        resourceManager_.registerTarget("level", [this]
        {
            return level();
        });
        resourceManager_.addContribution("hp.max", Contribution("hitPoints.constitution", "@conMod * @level"));
        resourceManager_.addToCollection(MovementGrantsResource, MovementGrant(MovementGrantDefinition{
            .id = "racial",
            .source = "Velocità razziale",
            .type = MovementType::Land,
            .baseSpeedExpression = "6",
            .maneuverability = std::nullopt,
            .affectedByArmor = true,
            .affectedByLoad = true
        }));
    }

    CharacterSheet::CharacterSheet(const CharacterSheetSaveData &data) : CharacterSheet()
    {
        if (data.formatVersion != 8 && data.formatVersion != 10 && data.formatVersion != 11 && data.formatVersion != 12 && data.formatVersion != 13 && data.formatVersion != 14)
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

        skills_.load(data.skills);
        hitPoints_.load(data.hitPoints);
    }

    CharacterSheet CharacterSheet::load(const std::filesystem::path &path)
    {
        return CharacterSheet(persistence::loadFromFile(path));
    }

    void CharacterSheet::setAbilityBaseValue(AbilityType type, int baseValue)
    {
        ability(type).setBaseValue(baseValue);
    }

    void CharacterSheet::advanceTime(GameDuration duration)
    {
        hitPoints_.advanceTime(duration);
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

    CharacterSheetView CharacterSheet::toView()
    {
        std::vector<AbilityView> abilityViews;
        abilityViews.reserve(abilities_.size());

        for (const AbilityScore &abilityScore : abilities_)
        {
            abilityViews.push_back(abilityScore.toView(resourceManager_));
        }

        return CharacterSheetView{
            .abilities = std::move(abilityViews),
            .baseAttackBonus = baseAttackBonus_.toView(),
            .attacks = attacks_.toView(),
            .combatManeuvers = combatManeuvers_.toView(),
            .hitPoints = hitPoints_.toView(),
            .initiative = initiative_.toView(),
            .armorClass = armorClass_.toView(),
            .savingThrows = savingThrows_.toView(),
            .skills = skills_.toView(),
            .movement = movement_.toView()
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
            .formatVersion = 14,
            .abilities = std::move(abilityData),
            .hitPoints = hitPoints_.toSaveData(),
            .skills = skills_.toSaveData()
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
