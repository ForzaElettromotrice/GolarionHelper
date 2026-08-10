#pragma once

#include "golarion/character/ability.hpp"

#include <string_view>
#include <vector>

namespace golarion
{
    class ResourceManager;
    struct SavingThrowSaveData;
    struct SavingThrowView;

    enum class SavingThrowType
    {
        Fortitude,
        Reflex,
        Will
    };

    std::string_view displayName(SavingThrowType type);
    std::string_view resourceName(SavingThrowType type);
    AbilityType defaultAbility(SavingThrowType type);

    class SavingThrow final
    {
    public:
        explicit SavingThrow(SavingThrowType type);

        void setBaseValue(int baseValue);
        void setAbilityType(AbilityType abilityType);
        void registerResources(ResourceManager &resourceManager, std::vector<std::string> parentResources) const;
        SavingThrowView toView(ResourceManager &resourceManager) const;
        SavingThrowSaveData toSaveData() const;
        int totalValue(ResourceManager &resourceManager) const;

    private:
        SavingThrowType type_;
        int baseValue_;
        AbilityType abilityType_;
    };
}
