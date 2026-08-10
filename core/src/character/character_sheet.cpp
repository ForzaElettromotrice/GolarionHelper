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
        : modifierGroupManager_(resourceManager_),
          contributionGroupManager_(resourceManager_),
          abilities_{
              AbilityScore(AbilityType::Strength),
              AbilityScore(AbilityType::Dexterity),
              AbilityScore(AbilityType::Constitution),
              AbilityScore(AbilityType::Intelligence),
              AbilityScore(AbilityType::Wisdom),
              AbilityScore(AbilityType::Charisma)
          },
          hitPoints_(resourceManager_),
          initiative_(resourceManager_),
          skills_(resourceManager_),
          savingThrows_(resourceManager_),
          movement_(resourceManager_),
          movementGroupManager_(resourceManager_)
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
        if (data.formatVersion != 8 && data.formatVersion != 10)
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

        initiative_.load(data.initiative);
        savingThrows_.load(data.savingThrows);
        skills_.load(data.skills);
        if (data.formatVersion >= 10)
        {
            for (const MovementGroupManagerSaveData::GroupSaveData &groupData : data.movementGroups.groups)
            {
                movementGroupManager_.addGroup(groupData.id, MovementGroup(groupData.group), groupData.enabled);
            }
        }

        for (const ModifierGroupManagerSaveData::GroupSaveData &groupData : data.modifierGroups.groups)
        {
            modifierGroupManager_.addGroup(groupData.id, ModifierGroup(groupData.group), groupData.enabled);
        }
        for (const ContributionGroupManagerSaveData::GroupSaveData &groupData : data.contributionGroups.groups)
        {
            contributionGroupManager_.addGroup(groupData.id, ContributionGroup(groupData.group), groupData.enabled);
        }
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

    void CharacterSheet::setMaxHitPoints(int value)
    {
        hitPoints_.setMax(value);
    }

    void CharacterSheet::setCurrentHitPoints(int value)
    {
        hitPoints_.setCurrent(value);
    }

    void CharacterSheet::setNonLethalDamage(int value)
    {
        hitPoints_.setNonLethal(value);
    }

    void CharacterSheet::addTemporaryHitPoints(std::string id, int amount, std::optional<GameDuration> duration)
    {
        resourceManager_.addToCollection(TemporaryHitPointsResource, TemporaryHitPointGrant{
            .id = std::move(id),
            .amountExpression = std::to_string(amount),
            .duration = duration
        });
    }

    void CharacterSheet::removeTemporaryHitPoints(std::string_view id)
    {
        resourceManager_.removeFromCollection(TemporaryHitPointsResource, id);
    }

    void CharacterSheet::advanceTime(GameDuration duration)
    {
        hitPoints_.advanceTime(duration);
    }

    void CharacterSheet::heal(int amount)
    {
        hitPoints_.heal(amount);
    }

    void CharacterSheet::damage(int amount, DamageType type)
    {
        hitPoints_.damage(amount, type);
    }

    void CharacterSheet::setInitiativeAbilityType(AbilityType abilityType)
    {
        initiative_.setAbilityType(abilityType);
    }

    void CharacterSheet::setSavingThrowBaseValue(SavingThrowType type, int baseValue)
    {
        savingThrows_.setBaseValue(type, baseValue);
    }

    void CharacterSheet::setSavingThrowAbilityType(SavingThrowType type, AbilityType abilityType)
    {
        savingThrows_.setAbilityType(type, abilityType);
    }

    void CharacterSheet::setSkillRanks(SkillType type, int ranks)
    {
        skills_.setRanks(type, ranks);
    }

    void CharacterSheet::setSkillClassSkill(SkillType type, bool classSkill)
    {
        skills_.setClassSkill(type, classSkill);
    }

    void CharacterSheet::setSkillAbilityType(SkillType type, AbilityType abilityType)
    {
        skills_.setAbilityType(type, abilityType);
    }

    void CharacterSheet::addSkillSpecialization(SkillType type, const std::string &specializationId, const std::string &specialization)
    {
        skills_.addSpecialization(type, specializationId, specialization);
    }

    void CharacterSheet::removeSkillSpecialization(SkillType type, const std::string &specializationId)
    {
        const std::string resourceName = skills_.removableSpecializationResourceName(type, specializationId);
        modifierGroupManager_.removeModifiersForResource(resourceName);
        skills_.removeSpecialization(type, specializationId);
    }

    void CharacterSheet::setSkillSpecializationRanks(SkillType type, const std::string &specializationId, int ranks)
    {
        skills_.setSpecializationRanks(type, specializationId, ranks);
    }

    void CharacterSheet::createMovementGroup(const std::string &groupId, MovementGroup group)
    {
        movementGroupManager_.addGroup(groupId, std::move(group), false);
    }

    void CharacterSheet::destroyMovementGroup(std::string_view groupId)
    {
        movementGroupManager_.removeGroup(groupId);
    }

    void CharacterSheet::setMovementGroupEnabled(std::string_view groupId, bool enabled)
    {
        movementGroupManager_.setGroupEnabled(groupId, enabled);
    }

    void CharacterSheet::createModifierGroup(const std::string &groupId, ModifierGroup group)
    {
        modifierGroupManager_.addGroup(groupId, std::move(group), false);
    }

    void CharacterSheet::destroyModifierGroup(std::string_view groupId)
    {
        modifierGroupManager_.removeGroup(groupId);
    }

    void CharacterSheet::setModifierGroupEnabled(std::string_view groupId, bool enabled)
    {
        modifierGroupManager_.setGroupEnabled(groupId, enabled);
    }

    void CharacterSheet::createContributionGroup(const std::string &groupId, ContributionGroup group)
    {
        contributionGroupManager_.addGroup(groupId, std::move(group), false);
    }

    void CharacterSheet::destroyContributionGroup(std::string_view groupId)
    {
        contributionGroupManager_.removeGroup(groupId);
    }

    void CharacterSheet::setContributionGroupEnabled(std::string_view groupId, bool enabled)
    {
        contributionGroupManager_.setGroupEnabled(groupId, enabled);
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
            .hitPoints = hitPoints_.toView(),
            .initiative = initiative_.toView(),
            .savingThrows = savingThrows_.toView(),
            .skills = skills_.toView(),
            .movement = movement_.toView(),
            .movementGroups = movementGroupManager_.toView(),
            .resources = resourceManager_.toView(),
            .modifierGroups = modifierGroupManager_.toView(),
            .contributionGroups = contributionGroupManager_.toView()
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
            .formatVersion = 10,
            .abilities = std::move(abilityData),
            .hitPoints = hitPoints_.toSaveData(),
            .initiative = initiative_.toSaveData(),
            .savingThrows = savingThrows_.toSaveData(),
            .skills = skills_.toSaveData(),
            .movementGroups = movementGroupManager_.toSaveData(),
            .modifierGroups = modifierGroupManager_.toSaveData(),
            .contributionGroups = contributionGroupManager_.toSaveData()
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
