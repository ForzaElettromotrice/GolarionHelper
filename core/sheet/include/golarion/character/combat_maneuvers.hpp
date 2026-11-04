#pragma once

#include "golarion/character/ability.hpp"

#include <map>
#include <string>
#include <string_view>

namespace golarion
{
    inline constexpr std::string_view CombatManeuverBonusAllResource = "combatManeuver.bonus.all";
    inline constexpr std::string_view CombatManeuverDefenseAllResource = "combatManeuver.defense.all";
    inline constexpr std::string_view CombatManeuverAbilityReplacementsResource = "combatManeuver.abilityReplacements";
    inline constexpr std::string_view CombatManeuverDefenseDexteritySuppressionsResource = "combatManeuver.defense.dexteritySuppressions";

    class ResourceManager;
    struct CombatManeuversView;

    enum class CombatManeuverType
    {
        BullRush,
        DirtyTrick,
        Disarm,
        Drag,
        Grapple,
        Overrun,
        Reposition,
        Steal,
        Sunder,
        Trip
    };

    std::string_view displayName(CombatManeuverType type);
    std::string combatManeuverBonusResourceName(CombatManeuverType type);
    std::string combatManeuverDefenseResourceName(CombatManeuverType type);

    struct CombatManeuverAbilityReplacementDefinition
    {
        std::string id;
        std::string source;
        std::string targetResourceName;
        AbilityType abilityType;
    };

    struct CombatManeuverDefenseDexteritySuppressionDefinition
    {
        std::string id;
        std::string source;
    };

    class CombatManeuverAbilityReplacement final
    {
    public:
        explicit CombatManeuverAbilityReplacement(CombatManeuverAbilityReplacementDefinition definition);

    private:
        friend class CombatManeuvers;

        std::string id_;
        std::string source_;
        std::string targetResourceName_;
        AbilityType abilityType_;
    };

    class CombatManeuverDefenseDexteritySuppression final
    {
    public:
        explicit CombatManeuverDefenseDexteritySuppression(CombatManeuverDefenseDexteritySuppressionDefinition definition);

    private:
        friend class CombatManeuvers;

        std::string id_;
        std::string source_;
    };

    class CombatManeuvers final
    {
    public:
        explicit CombatManeuvers(ResourceManager &resourceManager);

        CombatManeuvers(const CombatManeuvers &) = delete;
        CombatManeuvers &operator=(const CombatManeuvers &) = delete;
        CombatManeuvers(CombatManeuvers &&) = delete;
        CombatManeuvers &operator=(CombatManeuvers &&) = delete;

        CombatManeuversView toView();

    private:
        void addAbilityReplacement(CombatManeuverAbilityReplacement replacement);
        void removeAbilityReplacement(std::string_view replacementId);
        void addDexteritySuppression(CombatManeuverDefenseDexteritySuppression suppression);
        void removeDexteritySuppression(std::string_view suppressionId);

        ResourceManager &resourceManager_;
        std::map<std::string, CombatManeuverAbilityReplacement> abilityReplacements_;
        std::map<std::string, CombatManeuverDefenseDexteritySuppression> dexteritySuppressions_;
    };
}
