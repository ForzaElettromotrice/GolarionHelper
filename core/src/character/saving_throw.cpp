#include "golarion/character/saving_throw.hpp"

#include "golarion/data/saving_throw_save_data.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/saving_throw_view.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    int checkedSavingThrowValue(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("saving throw value is out of range");
        }
        return static_cast<int>(value);
    }
}

namespace golarion
{
    std::string_view displayName(SavingThrowType type)
    {
        switch (type)
        {
            case SavingThrowType::Fortitude:
                return "Tempra";
            case SavingThrowType::Reflex:
                return "Riflessi";
            case SavingThrowType::Will:
                return "Volontà";
        }
        throw std::invalid_argument("unknown saving throw type");
    }

    std::string_view resourceName(SavingThrowType type)
    {
        switch (type)
        {
            case SavingThrowType::Fortitude:
                return "savingThrow.fortitude";
            case SavingThrowType::Reflex:
                return "savingThrow.reflex";
            case SavingThrowType::Will:
                return "savingThrow.will";
        }
        throw std::invalid_argument("unknown saving throw type");
    }

    AbilityType defaultAbility(SavingThrowType type)
    {
        switch (type)
        {
            case SavingThrowType::Fortitude:
                return AbilityType::Constitution;
            case SavingThrowType::Reflex:
                return AbilityType::Dexterity;
            case SavingThrowType::Will:
                return AbilityType::Wisdom;
        }
        throw std::invalid_argument("unknown saving throw type");
    }

    SavingThrow::SavingThrow(SavingThrowType type) : type_(type), baseValue_(0), abilityType_(defaultAbility(type))
    {
    }

    void SavingThrow::setBaseValue(int baseValue)
    {
        if (baseValue < 0)
        {
            throw std::invalid_argument("saving throw base value must not be negative");
        }
        baseValue_ = baseValue;
    }

    void SavingThrow::setAbilityType(AbilityType abilityType)
    {
        abilityType_ = abilityType;
    }

    void SavingThrow::registerResources(ResourceManager &resourceManager, std::vector<std::string> parentResources) const
    {
        resourceManager.registerEnhanceableResource(resourceName(type_), std::move(parentResources));
    }

    SavingThrowView SavingThrow::toView(ResourceManager &resourceManager) const
    {
        ModifierSetView modifiers = resourceManager.modifierSetView(resourceName(type_));
        const int abilityModifier = resourceManager.targetValue(std::string(resourceName(abilityType_)) + "Mod");
        const int total = checkedSavingThrowValue(static_cast<long long>(baseValue_) + abilityModifier + modifiers.total);

        return SavingThrowView{
            .type = type_,
            .baseValue = baseValue_,
            .abilityType = abilityType_,
            .abilityModifier = abilityModifier,
            .totalValue = total,
            .modifiers = std::move(modifiers)
        };
    }

    SavingThrowSaveData SavingThrow::toSaveData() const
    {
        return SavingThrowSaveData{
            .type = type_,
            .baseValue = baseValue_,
            .abilityType = abilityType_
        };
    }

    int SavingThrow::totalValue(ResourceManager &resourceManager) const
    {
        const int abilityModifier = resourceManager.targetValue(std::string(resourceName(abilityType_)) + "Mod");
        return checkedSavingThrowValue(static_cast<long long>(baseValue_) + abilityModifier + resourceManager.modifierTotal(resourceName(type_)));
    }
}
