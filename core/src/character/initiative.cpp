#include "golarion/character/initiative.hpp"

#include "golarion/data/initiative_save_data.hpp"
#include "golarion/resource/resource_manager.hpp"
#include "golarion/view/initiative_view.hpp"

#include <limits>
#include <stdexcept>
#include <string>
#include <utility>

namespace
{
    constexpr std::string_view InitiativeResource = "initiative";

    int checkedInitiativeValue(long long value)
    {
        if (value < std::numeric_limits<int>::min() || value > std::numeric_limits<int>::max())
        {
            throw std::invalid_argument("initiative value is out of range");
        }
        return static_cast<int>(value);
    }
}

namespace golarion
{
    Initiative::Initiative(ResourceManager &resourceManager) : resourceManager_(resourceManager), abilityType_(AbilityType::Dexterity)
    {
        resourceManager_.registerEnhanceableResource(InitiativeResource);
    }

    void Initiative::setAbilityType(AbilityType abilityType)
    {
        abilityType_ = abilityType;
    }

    InitiativeView Initiative::toView()
    {
        ModifierSetView modifiers = resourceManager_.modifierSetView(InitiativeResource);
        const int abilityModifier = resourceManager_.targetValue(std::string(resourceName(abilityType_)) + "Mod");
        const int total = checkedInitiativeValue(static_cast<long long>(abilityModifier) + modifiers.total);

        return InitiativeView{
            .abilityType = abilityType_,
            .abilityModifier = abilityModifier,
            .totalValue = total,
            .modifiers = std::move(modifiers)
        };
    }

    InitiativeSaveData Initiative::toSaveData() const
    {
        return InitiativeSaveData{.abilityType = abilityType_};
    }

    void Initiative::load(const InitiativeSaveData &data)
    {
        setAbilityType(data.abilityType);
    }
}
