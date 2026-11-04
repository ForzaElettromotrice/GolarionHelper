#pragma once

#include "golarion/character/ability.hpp"

#include <map>
#include <string>
#include <string_view>

namespace golarion
{
    class ResourceManager;
    struct InitiativeView;

    inline constexpr std::string_view InitiativeResource = "initiative";
    inline constexpr std::string_view InitiativeAbilityReplacementsResource = "initiative.abilityReplacements";

    struct InitiativeAbilityReplacementDefinition
    {
        std::string id;
        std::string source;
        AbilityType abilityType;
    };

    class InitiativeAbilityReplacement final
    {
    public:
        explicit InitiativeAbilityReplacement(InitiativeAbilityReplacementDefinition definition);

    private:
        friend class Initiative;

        std::string id_;
        std::string source_;
        AbilityType abilityType_;
    };

    class Initiative final
    {
    public:
        explicit Initiative(ResourceManager &resourceManager);

        InitiativeView toView();

    private:
        void addAbilityReplacement(InitiativeAbilityReplacement replacement);
        void removeAbilityReplacement(std::string_view replacementId);

        ResourceManager &resourceManager_;
        std::map<std::string, InitiativeAbilityReplacement> abilityReplacements_;
    };
}
