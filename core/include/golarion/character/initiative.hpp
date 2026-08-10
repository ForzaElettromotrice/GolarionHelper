#pragma once

#include "golarion/character/ability.hpp"

namespace golarion
{
    class ResourceManager;
    struct InitiativeSaveData;
    struct InitiativeView;

    class Initiative final
    {
    public:
        explicit Initiative(ResourceManager &resourceManager);

        void setAbilityType(AbilityType abilityType);
        InitiativeView toView();
        InitiativeSaveData toSaveData() const;

    private:
        friend class CharacterSheet;

        void load(const InitiativeSaveData &data);

        ResourceManager &resourceManager_;
        AbilityType abilityType_;
    };
}
