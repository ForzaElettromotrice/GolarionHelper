#pragma once

#include "golarion/character/ability.hpp"

#include <optional>
#include <map>
#include <string>
#include <string_view>

namespace golarion
{
    inline constexpr std::string_view ArmorClassAllResource = "armorClass.all";
    inline constexpr std::string_view ArmorClassReflexiveResource = "armorClass.reflexive";
    inline constexpr std::string_view ArmorClassSolidResource = "armorClass.solid";
    inline constexpr std::string_view ArmorClassAbilityReplacementsResource = "armorClass.abilityReplacements";
    inline constexpr std::string_view ArmorClassAbilitySuppressionsResource = "armorClass.abilitySuppressions";
    inline constexpr std::string_view MaximumDexterityLimitsResource = "armorClass.maxDexLimits";
    inline constexpr std::string_view MaximumDexterityAllResource = "armorClass.maxDex.all";

    class ResourceManager;
    struct ArmorClassView;

    enum class ArmorClassType
    {
        Normal,
        Touch,
        FlatFooted
    };

    enum class MaximumDexterityLimitType
    {
        Armor,
        Shield,
        Load,
        Other
    };

    std::string_view displayName(ArmorClassType type);
    std::string_view resourceName(ArmorClassType type);
    std::string_view displayName(MaximumDexterityLimitType type);
    std::string_view maximumDexterityResourceName(MaximumDexterityLimitType type);
    std::string maximumDexterityResourceName(std::string_view limitId);

    struct MaximumDexterityLimitDefinition
    {
        std::string id;
        std::string source;
        MaximumDexterityLimitType type;
        std::string expression;
    };

    struct ArmorClassAbilityReplacementDefinition
    {
        std::string id;
        std::string source;
        AbilityType abilityType;
    };

    struct ArmorClassAbilitySuppressionDefinition
    {
        std::string id;
        std::string source;
    };

    class ArmorClassAbilityReplacement final
    {
    public:
        explicit ArmorClassAbilityReplacement(ArmorClassAbilityReplacementDefinition definition);

    private:
        friend class ArmorClass;

        std::string id_;
        std::string source_;
        AbilityType abilityType_;
    };

    class MaximumDexterityLimit final
    {
    public:
        explicit MaximumDexterityLimit(MaximumDexterityLimitDefinition definition);

    private:
        friend class ArmorClass;

        std::string id_;
        std::string source_;
        MaximumDexterityLimitType type_;
        std::string expression_;
    };

    class ArmorClassAbilitySuppression final
    {
    public:
        explicit ArmorClassAbilitySuppression(ArmorClassAbilitySuppressionDefinition definition);

    private:
        friend class ArmorClass;

        std::string id_;
        std::string source_;
    };

    class ArmorClass final
    {
    public:
        explicit ArmorClass(ResourceManager &resourceManager);

        std::optional<int> maximumDexterityBonus();
        ArmorClassView toView();

    private:
        void addAbilityReplacement(ArmorClassAbilityReplacement replacement);
        void removeAbilityReplacement(std::string_view replacementId);
        void addAbilitySuppression(ArmorClassAbilitySuppression suppression);
        void removeAbilitySuppression(std::string_view suppressionId);
        void addMaximumDexterityLimit(MaximumDexterityLimit limit);
        void removeMaximumDexterityLimit(std::string_view limitId);

        ResourceManager &resourceManager_;
        std::map<std::string, ArmorClassAbilityReplacement> abilityReplacements_;
        std::map<std::string, ArmorClassAbilitySuppression> abilitySuppressions_;
        std::map<std::string, MaximumDexterityLimit> maximumDexterityLimits_;
    };
}
