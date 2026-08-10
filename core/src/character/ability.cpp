#include "golarion/character/ability.hpp"

#include "golarion/data/ability_save_data.hpp"
#include "golarion/view/ability_view.hpp"
#include "golarion/resource/resource_manager.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    int checkedInt(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("ability value is out of range");
        }
        return static_cast<int>(value);
    }

    int floorDivideByTwo(long long value)
    {
        long long quotient = value / 2;
        if (value < 0 && value % 2 != 0)
        {
            --quotient;
        }
        return checkedInt(quotient);
    }
}

namespace golarion
{
    std::string_view displayName(AbilityType type)
    {
        switch (type)
        {
            case AbilityType::Strength:
                return "Forza";
            case AbilityType::Dexterity:
                return "Destrezza";
            case AbilityType::Constitution:
                return "Costituzione";
            case AbilityType::Intelligence:
                return "Intelligenza";
            case AbilityType::Wisdom:
                return "Saggezza";
            case AbilityType::Charisma:
                return "Carisma";
        }

        throw std::invalid_argument("unknown ability type");
    }

    std::string_view resourceName(AbilityType type)
    {
        switch (type)
        {
            case AbilityType::Strength:
                return "str";
            case AbilityType::Dexterity:
                return "dex";
            case AbilityType::Constitution:
                return "con";
            case AbilityType::Intelligence:
                return "int";
            case AbilityType::Wisdom:
                return "wis";
            case AbilityType::Charisma:
                return "cha";
        }

        throw std::invalid_argument("unknown ability type");
    }

    AbilityScore::AbilityScore(AbilityType type, int baseValue) : type_(type), baseValue_(10)
    {
        setBaseValue(baseValue);
    }

    void AbilityScore::setBaseValue(int baseValue)
    {
        if (baseValue <= 0)
        {
            throw std::invalid_argument("base value must be greater than 0");
        }
        baseValue_ = baseValue;
    }

    void AbilityScore::registerResources(ResourceManager &resourceManager) const
    {
        const std::string resource(resourceName(type_));
        ResourceManager *manager = &resourceManager;

        resourceManager.registerEnhanceableResource(resource);
        resourceManager.registerTarget(resource, [this, manager]
        {
            return totalValue(*manager);
        });
        resourceManager.registerTarget(resource + "Mod", [this, manager]
        {
            return modifier(*manager);
        });
    }

    AbilityView AbilityScore::toView(ResourceManager &resourceManager) const
    {
        ModifierSetView modifiers = resourceManager.modifierSetView(resourceName(type_));
        const int total = checkedInt(static_cast<long long>(baseValue_) + modifiers.total);

        return AbilityView{
            type_,
            baseValue_,
            total,
            floorDivideByTwo(static_cast<long long>(total) - 10),
            std::move(modifiers)
        };
    }

    AbilitySaveData AbilityScore::toSaveData() const
    {
        return AbilitySaveData{
            .type = type_,
            .baseValue = baseValue_
        };
    }

    int AbilityScore::baseValue() const
    {
        return baseValue_;
    }

    int AbilityScore::totalValue(ResourceManager &resourceManager) const
    {
        return checkedInt(static_cast<long long>(baseValue_) + resourceManager.modifierTotal(resourceName(type_)));
    }

    int AbilityScore::modifier(ResourceManager &resourceManager) const
    {
        return floorDivideByTwo(static_cast<long long>(totalValue(resourceManager)) - 10);
    }
}
