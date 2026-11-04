#pragma once

#include "golarion/character/ability.hpp"

#include <map>
#include <string>
#include <string_view>
#include <vector>

namespace golarion
{
    class ResourceManager;
    class SavingThrows;
    struct SavingThrowView;

    inline constexpr std::string_view SavingThrowAbilityReplacementsResource = "savingThrow.abilityReplacements";

    enum class SavingThrowType
    {
        Fortitude,
        Reflex,
        Will
    };

    std::string_view displayName(SavingThrowType type);
    std::string_view resourceName(SavingThrowType type);
    std::string baseResourceName(SavingThrowType type);
    AbilityType defaultAbility(SavingThrowType type);

    struct SavingThrowAbilityReplacementDefinition
    {
        std::string id;
        std::string source;
        SavingThrowType savingThrowType;
        AbilityType abilityType;
    };

    class SavingThrowAbilityReplacement final
    {
    public:
        explicit SavingThrowAbilityReplacement(SavingThrowAbilityReplacementDefinition definition);

    private:
        friend class SavingThrow;
        friend class SavingThrows;

        std::string id_;
        std::string source_;
        SavingThrowType savingThrowType_;
        AbilityType abilityType_;
    };

    class SavingThrow final
    {
    public:
        explicit SavingThrow(SavingThrowType type);

        void registerResources(ResourceManager &resourceManager, std::vector<std::string> parentResources) const;
        SavingThrowView toView(ResourceManager &resourceManager, const std::map<std::string, SavingThrowAbilityReplacement> &abilityReplacements = {}) const;
        int totalValue(ResourceManager &resourceManager) const;

    private:
        SavingThrowType type_;
    };
}
